"""Independent extraction: compare every init byte to the vendored SDL source."""
import pathlib,re
root=pathlib.Path(__file__).resolve().parents[1]
s=(root/'reference/SDL_hidapi_switch2.c').read_text()
s=s.split('const Uint8 *init_sequence[] = {',1)[1].split('NULL, // Sentinel',1)[0]
arrays=re.findall(r'\(Uint8\[\]\)\s*\{(.*?)\}',s,re.S)
def values(s):
    s=re.sub(r'//[^\n]*','',s)
    return [int(x,0) for x in re.findall(r'\b(?:0x[0-9a-fA-F]+|[0-9]+)\b',s)]
expected=[values(a) for a in arrays]
ours=(root/'source/gc2_protocol.cpp').read_text().split('InitCommands{{',1)[1].split('}};',1)[0]
actual=[values(a) for a in re.findall(r'\{\{\{(.*?)\}\}',ours,re.S)]
if actual!=expected: raise SystemExit(f'Init differs: {actual!r} != {expected!r}')
print(f'{len(actual)} USB initialization commands match SDL byte-for-byte')
