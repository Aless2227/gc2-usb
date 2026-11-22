"""Prepare target libraries and build switch-tools locally using an existing LLVM.
Only files in --cache are written. Does not install anything globally.
"""
import argparse,hashlib,json,pathlib,shutil,subprocess,tarfile,urllib.request
p=argparse.ArgumentParser()
p.add_argument('--clang',required=True,help='Existing host clang.exe (C compiler)')
p.add_argument('--cache',type=pathlib.Path,required=True)
p.add_argument('--layer-file',type=pathlib.Path,help='Optional already downloaded official OCI layer')
a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]
lock=json.loads((root/'toolchain-lock.json').read_text())
cache=a.cache.resolve();cache.mkdir(parents=True,exist_ok=True)
def download(url,target,headers=None):
    if target.exists():return
    request=urllib.request.Request(url,headers=headers or {})
    with urllib.request.urlopen(request,timeout=60) as r,target.open('wb') as f:
        shutil.copyfileobj(r,f)
def contained(path):
    if not path.resolve().is_relative_to(cache):raise RuntimeError('Unsafe archive path')
def unpack(archive,destination,predicate):
    with tarfile.open(archive,'r:*') as t:
        for m in t:
            if not m.isfile():continue
            name=m.name.removeprefix('./')
            if not predicate(name):continue
            path=destination/name;contained(path)
            path.parent.mkdir(parents=True,exist_ok=True)
            with t.extractfile(m) as src,path.open('wb') as dst:shutil.copyfileobj(src,dst)
archive=a.layer_file.resolve() if a.layer_file else cache/'devkita64-layer.tar.gz'
if not archive.exists():
    url='https://auth.docker.io/token?service=registry.docker.io&scope=repository:devkitpro/devkita64:pull'
    token=json.load(urllib.request.urlopen(url))['token']
    download('https://registry-1.docker.io/v2/devkitpro/devkita64/blobs/'+lock['target_layer'],archive,{'Authorization':'Bearer '+token})
with archive.open('rb') as f:actual=hashlib.file_digest(f,'sha256').hexdigest()
if actual!=lock['target_layer'].removeprefix('sha256:'):raise SystemExit('Official OCI layer digest mismatch')
dkp=cache/'root/opt/devkitpro'
if not (dkp/'libnx/lib/libnx.a').exists():
    unpack(archive,cache/'root',lambda n:n.startswith(('opt/devkitpro/devkitA64/','opt/devkitpro/libnx/')) and any(x in n for x in ('/include/','/lib/','/libnx/')))
sources={}
for repo,key in [('switchbrew/switch-tools','switch_tools_commit'),('lz4/lz4','lz4_commit')]:
    sha=lock[key];archive=cache/(key+'.tar.gz')
    download(f'https://codeload.github.com/{repo}/tar.gz/{sha}',archive)
    destination=cache/key
    if not destination.exists():unpack(archive,destination,lambda n:True)
    sources[key]=next(destination.iterdir())
tools=cache/'bin';tools.mkdir(exist_ok=True)
suffix='.exe' if __import__('sys').platform=='win32' else ''
st=sources['switch_tools_commit']/'src';lz4=sources['lz4_commit']/'lib'
commands=[
    [a.clang,'-O2','-D_CRT_SECURE_NO_WARNINGS',st/'npdmtool.c',st/'cJSON.c','-o',tools/('npdmtool'+suffix)],
    [a.clang,'-O2','-D_CRT_SECURE_NO_WARNINGS','-I'+str(lz4),st/'elf2nso.c',st/'sha256.c',lz4/'lz4.c','-o',tools/('elf2nso'+suffix)]
]
for cmd in commands:subprocess.run([str(x) for x in cmd],check=True)
env={'devkitpro':str(dkp),'npdmtool':str(tools/('npdmtool'+suffix)),'elf2nso':str(tools/('elf2nso'+suffix)),
     'layer_sha256':actual,'libraries':'official devkitPro image; not executed on host'}
(cache/'environment.json').write_text(json.dumps(env,indent=2)+'\n')
print(json.dumps(env,indent=2))
