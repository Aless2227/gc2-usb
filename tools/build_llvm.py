"""Alternative experimental cross-build with LLVM and official devkitPro target libraries.
No host OS binaries from the Linux image are executed. See docs/BUILD.md.
"""
import argparse,pathlib,subprocess,struct
p=argparse.ArgumentParser()
p.add_argument('--clang',required=True)
p.add_argument('--lld',required=True)
p.add_argument('--devkitpro',type=pathlib.Path,required=True)
p.add_argument('--elf2nso',required=True)
p.add_argument('--npdmtool',required=True)
a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]
dkp=a.devkitpro.resolve();target=dkp/'devkitA64/aarch64-none-elf'
gcc=next((dkp/'devkitA64/lib/gcc/aarch64-none-elf').iterdir())
cxx=next((target/'include/c++').iterdir())
out=root/'build';out.mkdir(exist_ok=True)
def run(args):
    print('RUN',pathlib.Path(args[0]).name, ' '.join(str(x) for x in args[1:3]),flush=True)
    subprocess.run([str(x) for x in args],check=True)
flags=['--target=aarch64-none-elf','-march=armv8-a+crc+crypto','-mtune=cortex-a57',
       '-fPIC','-ftls-model=local-exec','-D__SWITCH__','-std=c++17','-O2','-g',
       '-fno-exceptions','-fno-rtti','-ffunction-sections','-fdata-sections',
       '-nostdinc++','-Wall','-Wextra','-Wno-unused-parameter',
       '-isystem',str(cxx),'-isystem',str(cxx/'aarch64-none-elf'),
       '-isystem',str(target/'include'),'-isystem',str(dkp/'libnx/include'),
       '-I'+str(root/'include'),'-I'+str(root/'vendor/syscon')]
sources=[*sorted((root/'source').glob('*.cpp')),
         *(root/'vendor/syscon'/s for s in ['SwitchUSBInterface.cpp','SwitchUSBEndpoint.cpp','SwitchUSBLock.cpp'])]
objects=[]
for src in sources:
    obj=out/(src.stem+'.o');run([a.clang,*flags,'-c',src,'-o',obj]);objects.append(obj)
# Match libnx switch.specs with LLD equivalents. --nx-module-name is a devkitPro
# GNU ld extension, so an explicit section is supplied below instead.
module=out/'module_name.S'
module.write_text('.section .nx-module-name, "a"\n.balign 4\n.long 0\n.long 7\n.asciz "gc2-usb"\n.balign 4\n')
run([a.clang,'--target=aarch64-none-elf','-c',module,'-o',out/'module_name.o'])
objects.append(out/'module_name.o')
elf=out/'gc2-usb.elf'
# LLD requires an explicit PT_TLS for the GCC-built library's STT_TLS symbols.
# NSO conversion keeps the three PT_LOAD segments and libnx's reserved TLS storage.
linker=(dkp/'libnx/switch.ld').read_text()
linker=linker.replace('dyn    PT_DYNAMIC;', 'dyn    PT_DYNAMIC;\n\ttls PT_TLS;')
for section in ('.tdata :','.tbss :'):
    start=linker.index(section)
    end=linker.index('} :data',start)
    linker=linker[:end]+linker[end:].replace('} :data','} :data :tls',1)
script=out/'switch-llvm.ld';script.write_text(linker)
run([a.lld,'-m','aarch64elf','-T',script,'-pie','--no-dynamic-linker',
     '--gc-sections','-z','text','-z','now','--build-id=sha1','-u','main',
     '-Map='+str(out/'gc2-usb.map'),'-o',elf,
     gcc/'pic/crti.o',gcc/'pic/crtbegin.o',*objects,
     '-L'+str(dkp/'libnx/lib'),'-L'+str(target/'lib/pic'),'-L'+str(gcc/'pic'),
     '--start-group','-lstdc++','-lnx','-lpthread','-lm','-lc','-lsysbase','-lgcc','--end-group',
     gcc/'pic/crtend.o',gcc/'pic/crtn.o'])
run([a.elf2nso,elf,out/'gc2-usb.nso'])
run([a.npdmtool,root/'gc2-usb.json',out/'gc2-usb.npdm'])
# PFS0 ExeFS, same format built by switch-tools build_pfs0.
entries=[('main',(out/'gc2-usb.nso').read_bytes()),('main.npdm',(out/'gc2-usb.npdm').read_bytes())]
names=b'';records=b'';data=b''
for name,payload in entries:
    records+=struct.pack('<QQII',len(data),len(payload),len(names),0)
    names+=name.encode()+b'\0';data+=payload
header=struct.pack('<4sIII',b'PFS0',len(entries),len(names),0)
(out/'gc2-usb.nsp').write_bytes(header+records+names+data)
print('Built experimental AArch64 ExeFS; NO hardware compatibility claim.')
