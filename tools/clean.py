import pathlib, shutil
root=pathlib.Path(__file__).resolve().parents[1]
for name in ('build','build-host','dist'):
    p=(root/name).resolve()
    if p.parent!=root or p.name!=name: raise SystemExit('Invalid output path')
    if p.exists(): shutil.rmtree(p)
