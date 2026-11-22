"""Prepare allowlisted source/release assets locally. Does not publish to GitHub."""
import hashlib
import json
import pathlib
import shutil
import zipfile

root = pathlib.Path(__file__).resolve().parents[1]
release = json.loads((root / 'version.json').read_text())
version = release['version']
assets = root / 'release-assets'
assets.mkdir(exist_ok=True)
binary = root / 'dist' / f"gc2-usb-{version}-{release['package_kind']}.zip"
nsp = root / 'build/gc2-usb.nsp'
verification = json.loads((root / 'build/verification.json').read_text())
if (verification.get('sha256_nsp') != hashlib.sha256(nsp.read_bytes()).hexdigest()
        or not verification.get('nso_hashes_verified')
        or not verification.get('nso_matches_elf')):
    raise SystemExit('Run tools/verify_binary.py on this binary before assembling releases')
with zipfile.ZipFile(binary) as z:
    payload = z.read('atmosphere/contents/690000000000000D/exefs.nsp')
    if payload != nsp.read_bytes():
        raise SystemExit('SD package does not match verified binary; run tools/package.py')

source_dirs = {'source', 'include', 'vendor', 'reference', 'tests', 'tools', 'docs', 'licenses', '.github'}
source_files = {'.gitignore', '.gitattributes', 'README.md', 'CREDITS.md', 'THIRD_PARTY.md',
                'LICENSE', 'CHANGELOG.md', 'CONTRIBUTING.md', 'Makefile', 'Dockerfile',
                'gc2-usb.json', 'config.ini', 'toolchain-lock.json', 'version.json'}
suffixes = {'.cpp', '.c', '.h', '.hpp', '.py', '.ps1', '.md', '.json', '.yml', '.yaml'}
paths = []
for p in sorted(root.rglob('*')):
    if not p.is_file() or p.is_symlink():
        continue
    rel = p.relative_to(root)
    if rel.parts[0] not in source_dirs and rel.as_posix() not in source_files:
        continue
    if any(x in rel.parts for x in ('__pycache__', '.git')):
        continue
    if rel.parts[0] in source_dirs and rel.parts[0] != 'licenses' and p.suffix not in suffixes:
        continue
    paths.append(p)
archive = assets / f'gc2-usb-{version}-source.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for p in paths:
        z.write(p, 'gc2-usb/' + p.relative_to(root).as_posix())
shutil.copyfile(binary, assets / binary.name)
shutil.copyfile(root / 'build/verification.json', assets / 'verification.json')
shutil.copyfile(root / 'docs/RELEASE_NOTES.md', assets / 'RELEASE_NOTES.md')
names = [archive.name, binary.name, 'verification.json', 'RELEASE_NOTES.md']
host = root / 'build-host/host-checks.json'
if host.exists():
    shutil.copyfile(host, assets / 'host-checks.json')
    names.append('host-checks.json')
(assets / 'SHA256SUMS.txt').write_text(''.join(
    hashlib.sha256((assets / name).read_bytes()).hexdigest() + '  ' + name + '\n'
    for name in sorted(names)), encoding='utf-8')
print(f'Prepared {len(paths)} source files and release assets in {assets.name}/')
print('Local packaging only; no upload or hardware compatibility claim.')
