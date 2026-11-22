"""Build/run real C++ decoder tests; requires a host C++ compiler, never devkitA64."""
import argparse, pathlib, subprocess, sys
p=argparse.ArgumentParser()
p.add_argument('--cxx',default='g++')
a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1]
out=root/'build-host';out.mkdir(exist_ok=True)
exe=out/('protocol_tests.exe' if sys.platform=='win32' else 'protocol_tests')
subprocess.run([a.cxx,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'include'),str(root/'source/gc2_protocol.cpp'),str(root/'tests/protocol_tests.cpp'),'-o',str(exe)],check=True)
subprocess.run([str(exe)],check=True)
transport=out/('transport_tests.exe' if sys.platform=='win32' else 'transport_tests')
subprocess.run([a.cxx,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'include'),'-I'+str(root/'vendor/syscon'),str(root/'source/gc2_protocol.cpp'),str(root/'source/gc2_transport.cpp'),str(root/'tests/transport_tests.cpp'),'-o',str(transport)],check=True)
subprocess.run([str(transport)],check=True)
sticks=out/('stick_tests.exe' if sys.platform=='win32' else 'stick_tests')
subprocess.run([a.cxx,'-std=c++17','-O2','-Wall','-Wextra','-Werror','-I'+str(root/'include'),str(root/'source/gc2_sticks.cpp'),str(root/'tests/stick_tests.cpp'),'-o',str(sticks)],check=True)
subprocess.run([str(sticks)],check=True)
