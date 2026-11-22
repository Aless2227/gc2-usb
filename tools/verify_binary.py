"""Structural ELF/NSO/NPDM verification. Does not execute the sysmodule."""
import hashlib,json,pathlib,struct,sys
root=pathlib.Path(__file__).resolve().parents[1]
build=root/'build'
elf=(build/'gc2-usb.elf').read_bytes()
if elf[:6]!=b'\x7fELF\x02\x01':raise SystemExit('Expected ELF64 little endian')
h=struct.unpack_from('<16sHHIQQQIHHHHHH',elf)
if h[1:3]!=(3,183) or h[4]!=0:raise SystemExit('Expected AArch64 ET_DYN with entry at 0')
loads=[];dynamic=None
for i in range(h[10]):
    p=struct.unpack_from('<IIQQQQQQ',elf,h[5]+i*h[9])
    if p[0]==1:loads.append(p)
    if p[0]==2:dynamic=p
if len(loads)!=3 or [p[1] for p in loads]!=[5,4,6]:raise SystemExit('Expected RX/R/RW segments')
if any(p[3]%4096 or p[5]>p[6] or p[2]+p[5]>len(elf) for p in loads):raise SystemExit('Invalid load bounds')
if any(a[3]+a[6]>b[3] for a,b in zip(loads,loads[1:])):raise SystemExit('Overlapping segments')
def at_va(address,n):
    for p in loads:
        if p[3]<=address and address+n<=p[3]+p[5]:
            return elf[p[2]+address-p[3]:p[2]+address-p[3]+n]
    raise ValueError('Virtual address outside file-backed segments')
dyn={}
for i in range(dynamic[5]//16):
    tag,value=struct.unpack_from('<QQ',elf,dynamic[2]+16*i)
    if tag==0:break
    dyn[tag]=value
if 1 in dyn:raise SystemExit('DT_NEEDED not permitted')
if dyn.get(9)!=24:raise SystemExit('Invalid RELA size')
rela=at_va(dyn[7],dyn[8])
for i in range(0,len(rela),24):
    address,info,addend=struct.unpack_from('<QQq',rela,i)
    if info!=1027:raise SystemExit(f'Unsupported dynamic relocation {info}')
    if not any(p[3]<=address and address+8<=p[3]+p[6] for p in loads):raise SystemExit('Bad relocation target')
nso=(build/'gc2-usb.nso').read_bytes()
if nso[:4]!=b'NSO0' or len(nso)<256:raise SystemExit('Invalid NSO')
flags=struct.unpack_from('<I',nso,12)[0]
# Optional LZ4 module for independent decompression; required for a full verification.
try:import lz4.block
except ImportError:raise SystemExit('Install lz4 to independently verify compressed NSO segments')
for i,p in enumerate(loads):
    off,va,size,extra=struct.unpack_from('<IIII',nso,16+16*i)
    compressed=struct.unpack_from('<I',nso,96+4*i)[0]
    data=nso[off:off+compressed]
    if flags&(1<<i):data=lz4.block.decompress(data,uncompressed_size=size)
    if va!=p[3] or size!=p[5] or data!=elf[p[2]:p[2]+p[5]]:raise SystemExit('ELF/NSO mismatch')
    if hashlib.sha256(data).digest()!=nso[160+32*i:192+32*i]:raise SystemExit('NSO SHA256 mismatch')
npdm=(build/'gc2-usb.npdm').read_bytes()
if npdm[:4]!=b'META':raise SystemExit('NPDM magic')
aci=npdm.index(b'ACI0');program=struct.unpack_from('<Q',npdm,aci+16)[0]
if program!=0x690000000000000d:raise SystemExit('NPDM title mismatch')
report={'elf':'AArch64 ET_DYN','entry':0,'segments':3,'relative_relocations':len(rela)//24,
        'nso_hashes_verified':True,'nso_matches_elf':True,'npdm_title_id':f'{program:016X}',
        'load_memory_bytes':loads[-1][3]+loads[-1][6],
        'sha256_nsp':hashlib.sha256((build/'gc2-usb.nsp').read_bytes()).hexdigest(),
        'hardware_tested':False,'smash_tested':False}
(build/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
