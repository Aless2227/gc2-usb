"""Validate NSP/PFS0 and NSO metadata before generating a manual-install ZIP."""
import hashlib,json,pathlib,struct,zipfile
root=pathlib.Path(__file__).resolve().parents[1]
release=json.loads((root/'version.json').read_text())
source=root/'build/gc2-usb.nsp'
b=source.read_bytes()
if b[:4]!=b'PFS0': raise SystemExit('NSP is not PFS0; not packaging')
_,count,strsize,_=struct.unpack_from('<4sIII',b)
if count>16: raise SystemExit('Invalid PFS0 count')
strings_at=16+24*count
data_at=strings_at+strsize
files={}
for i in range(count):
    offset,size,name,_=struct.unpack_from('<QQII',b,16+24*i)
    end=b.index(b'\0',strings_at+name,data_at)
    label=b[strings_at+name:end].decode()
    payload=b[data_at+offset:data_at+offset+size]
    if len(payload)!=size: raise SystemExit('Truncated PFS0')
    files[label]=payload
if set(files)!={'main','main.npdm'} or files['main'][:4]!=b'NSO0' or files['main.npdm'][:4]!=b'META':
    raise SystemExit('Invalid ExeFS contents')
title=json.loads((root/'gc2-usb.json').read_text())['program_id'].removeprefix('0x')
out=root/'dist';out.mkdir(exist_ok=True)
zip_path=out/f"gc2-usb-{release['version']}-{release['package_kind']}.zip"
prefix=f'atmosphere/contents/{title}'
with zipfile.ZipFile(zip_path,'w',zipfile.ZIP_DEFLATED) as z:
    z.writestr(prefix+'/exefs.nsp',b)
    # No boot2.flag: explicit activation is an installation step for experimental builds.
    z.writestr(prefix+'/flags/.keep','')
    z.write(root/'config.ini','config/gc2-usb/config.ini')
    for p in ('README.md','CREDITS.md','CHANGELOG.md','THIRD_PARTY.md','LICENSE'):
        z.write(root/p,'gc2-usb-docs/'+p)
    for p in sorted((root/'docs').glob('*.md')):
        z.write(p,'gc2-usb-docs/docs/'+p.name)
    for p in sorted((root/'licenses').iterdir()):
        if p.is_file():z.write(p,'gc2-usb-docs/licenses/'+p.name)
report={'version':release['version'],'format_checked':True,'sha256_nsp':hashlib.sha256(b).hexdigest(),
        'nsp_bytes':len(b),'title_id':title,'hardware_tested':False,'smash_tested':False}
(out/'binary-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
print(zip_path)
