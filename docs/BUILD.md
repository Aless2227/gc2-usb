# Build environment

Python 3.11+ is used by the tooling. The target is AArch64 Horizon under Atmosphere, not a desktop executable. Source/image revisions are pinned in [toolchain-lock.json](../toolchain-lock.json).

## Executed build: LLVM with official devkitPro target libraries

The delivered 0.1.3 binary was built on Windows with existing Clang/LLD 22.1.3, targeting `aarch64-none-elf`. Headers and libraries came from the official `devkitpro/devkita64` image pinned by digest: libnx 4.12.0 and devkitA64 GCC 15.2.0 runtime/headers. The downloaded image layer was SHA256 checked before extraction. Linux executables from the image were not run on Windows. Host `elf2nso` and `npdmtool` were built from the pinned switch-tools/LZ4 sources.

`build_llvm.py` adds an explicit `PT_TLS` to a local copy of libnx's linker script for LLD, retaining its three load segments and TLS reservation. It also supplies an explicit module-name section instead of GNU ld's `--nx-module-name` extension. These adaptations require real console verification; structural checks alone cannot validate them.

### Prepare and build on Windows

Use an existing LLVM installation with AArch64 support and a working host C/C++ development environment. Replace the example tool paths with your own; run commands from the repository root:

```powershell
python tools/bootstrap_llvm.py --clang "C:/LLVM/bin/clang.exe" --cache .toolchain
python tools/build_llvm.py --clang "C:/LLVM/bin/clang++.exe" --lld "C:/LLVM/bin/ld.lld.exe" --devkitpro .toolchain/root/opt/devkitpro --elf2nso .toolchain/bin/elf2nso.exe --npdmtool .toolchain/bin/npdmtool.exe
python -m pip install lz4
python tools/verify_binary.py
python tools/package.py
python tools/assemble_delivery.py
```

The bootstrap downloads an approximately 383 MB image layer plus small source archives and requires additional extraction space. It writes inside `--cache`; it does not install drivers or alter the registry. Generated paths are recorded in `.toolchain/environment.json`. An already downloaded layer may be supplied using `--layer-file`; the pinned digest is still required. On Linux omit `.exe`; this alternative path has not been executed on Linux during release preparation.

A build may contain debug paths, so cross-host byte-for-byte identity is not promised. Record the SHA256 of the binary actually tested. The publication package preserves the previously built 0.1.3 executable; the English documentation/notice preparation does not claim a new hardware test.

## Standard devkitA64 path: provided, not executed here

With an existing devkitPro environment containing devkitA64, libnx, switch-tools, make and Python:

```sh
make package
```

On Windows with devkitPro/MSYS2:

```powershell
./tools/build.ps1 -DevkitPro C:/devkitPro
```

The Makefile uses libnx `switch_rules` and `switch.specs`. A pinned Dockerfile is also supplied:

```sh
docker build -t gc2-usb-build .
docker run --rm -v "$PWD:/project" gc2-usb-build
```

Neither this Makefile build nor this Docker build was executed during preparation of the delivered binary. Do not interpret their presence as a passed build result.

## Checks and outputs

```sh
python tools/test_host.py --cxx clang++
python tools/check_sdl_reference.py
python tools/verify_binary.py
python tools/package.py
python tools/assemble_delivery.py
```

Host checks need a C++17 **host** compiler; pass its full path on Windows if necessary. They use synthetic input, not USB hardware. GitHub's workflow runs only these host/reference checks, not the target build or console tests.

`verify_binary.py` requires Python `lz4`. It checks ELF64 AArch64 ET_DYN, entry zero, three non-overlapping RX/R/RW load segments, relative dynamic relocations, LZ4 decompression against ELF, NSO hashes and NPDM program ID. It writes `build/verification.json`. Its `hardware_tested=false` and `smash_tested=false` fields refer to physical validation of this exact release binary; historical tester feedback is recorded separately in [VALIDATION.md](VALIDATION.md).

`package.py` checks ExeFS structure and generates the SD ZIP and a format manifest under `dist/`. Run the independent verifier first; a format manifest is not a hardware result. `assemble_delivery.py` creates source and SD release assets, verification JSON, English release notes and SHA256 checksums under `release-assets/`. It does not publish anything or include local logs, captures, the toolchain or host executables. `tools/clean.py` removes generated `build/`, `build-host/` and `dist/` within this repository; release assets are preserved.

## Optional PC diagnostics

Linux/libusb is the preferred capture path. Install PyUSB separately and see [VALIDATION.md](VALIDATION.md). The default `capture_usb.py` mode only reads descriptors; `--capture` sends the documented initialization/read sequence.

The Python helper retains stricter historical initialization-reply checks and can fail where the module succeeds; see the explicit legacy-tool limitation in VALIDATION.md. It does not share the current C++ command channel.

The Windows probe source is included, but no probe executable is shipped in this publication package:

```powershell
python tools/build_pc_probe.py --clang "C:/LLVM/bin/clang++.exe"
```

The output is `pc-probe/gc2-pc-probe-0.1.2.exe`; its version refers to the diagnostic transport implementation. It is not the Atmosphere module. Previous attempts to open the tester's WinUSB interface returned access denied; no real input capture was obtained and no cause exclusive to sandboxing was established.
