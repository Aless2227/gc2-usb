"""Build the optional Windows USB probe with an existing host Clang."""
import argparse,pathlib,subprocess,sys
p=argparse.ArgumentParser();p.add_argument('--clang',required=True);a=p.parse_args()
if sys.platform!='win32':raise SystemExit('Native WinUSB/HID probe requires Windows')
root=pathlib.Path(__file__).resolve().parents[1]
out=root/'pc-probe';out.mkdir(exist_ok=True)
cmd=[a.clang,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-D_CRT_SECURE_NO_WARNINGS',
     '-fms-runtime-lib=static','-I'+str(root/'include'),'-I'+str(root/'vendor/syscon'),
     root/'tools/pc_probe.cpp',root/'source/gc2_protocol.cpp',root/'source/gc2_transport.cpp',
     '-lsetupapi','-lwinusb','-lhid','-o',out/'gc2-pc-probe-0.1.2.exe']
subprocess.run([str(x) for x in cmd],check=True)
