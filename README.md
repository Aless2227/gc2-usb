# gc2-usb

Experimental Atmosphere sysmodule for using the **official Nintendo Switch 2 wireless GameCube controller over USB on an original Nintendo Switch**. Derived from [sys-con](https://github.com/cathery/sys-con), with USB code from [o0Zz's fork](https://github.com/o0Zz/sys-con) and protocol logic adapted from [SDL](https://github.com/libsdl-org/SDL).

**USB only. One controller. Target device: `057e:2073`.** Bluetooth, older GameCube adapters, Joy-Con 2 and Switch 2 Pro Controllers are outside this project's scope. This is an independent project, not an official Nintendo product.

## Status

**Experimental; not a compatibility certification.** Version 0.1.3 adds configurable outer-range compensation, but that adjustment has not yet been tested..

| Area | Evidence |
| --- | --- |
| USB discovery and acquisition | Both target interfaces and calibration checks succeeded in a tester's 0.1.1 log on HOS 23.0.1. Initialization then stopped while waiting for replies. |
| Initialization change in 0.1.2 | Reply handling was aligned with SDL's policy. Working gameplay was subsequently reported; a complete control/stability checklist remains pending. |
| Stick adjustment in 0.1.3 | Implemented and checked with synthetic data; physical effectiveness remains unverified. |
| Host checks | 20,150 protocol checks, 81 simulated transport checks and 191,115 stick checks passed. Ten initialization commands match the pinned SDL reference. |
| Build | AArch64 ELF, NSO, NPDM and ExeFS generated with LLVM 22.1.3 and official devkitPro target libraries; structurally verified. |
| Standard devkitA64/Docker build | Provided, but not executed during preparation of this release. |

Build success and synthetic checks do not prove console compatibility, latency or a fix for the reported stick problem. See [validation](docs/VALIDATION.md) and [known limitations](docs/LIMITATIONS.md).

## Installation

Download `gc2-usb-0.1.3-experimental.zip` from the release assets. The GitHub source ZIP is **not** an SD installation package.

1. Fully power off the console and back up any existing `atmosphere/contents/690000000000000D/` folder.
2. Extract the installation ZIP to the SD root. It contains `atmosphere/contents/690000000000000D/exefs.nsp` and `config/gc2-usb/config.ini`.
3. To enable automatic startup, create an empty file named `atmosphere/contents/690000000000000D/flags/boot2.flag`. The ZIP deliberately does not include it.
4. Boot Atmosphere and connect the controller to a dock USB port with a USB **data** cable. USB host OTG is another test path; it is not yet validated by a complete hardware test matrix.
5. Check `config/gc2-usb/log.txt`, then test system input before trying Smash.

**Do not run this alongside sys-con.** It uses sys-con's program ID `690000000000000D` and PSC slot 126, and replaces that module. You do not need to install sys-con separately. Remove `boot2.flag` with the console powered off to disable gc2-usb. See [full installation and recovery instructions](docs/INSTALL.md).

## Input mapping

The controller is published as a **virtual Pro Controller**, so use the Pro Controller profile in Smash. A Switch 2 GameCube menu icon is not implemented.

| Physical control | Virtual input |
| --- | --- |
| A / B / X / Y and D-pad | Same Nintendo button labels and directions |
| Main stick / C-stick | Left / right analog stick |
| L / R analog triggers | Digital L / R: press at 25%, release at 20%; full trigger click also activates |
| Additional ZL / original Z | ZL / ZR |
| START / HOME / Capture | Plus / HOME / Capture |
| GameChat C button | Minus |

Trigger travel is decoded and calibrated, then converted to digital buttons for this output. Native GameCube analog trigger output, rumble, motion and player LED control are not implemented.

## Stick adjustment

`config/gc2-usb/config.ini` is read when the module starts:

```ini
[sticks]
left_stick_outer_percent=85
right_stick_outer_percent=85
stick_diagnostics_seconds=0
```

Each outer percentage accepts 60 through 100. With the default 85, full virtual radial travel is reached at 85% of the calibrated range. The inner 50% is unchanged. **100 bypasses the adjustment exactly.** The default is an output policy, not a measured calibration value or a Nintendo specification. Restart Atmosphere after editing. See [stick adjustment and measurement](docs/STICKS-0.1.3.md).

## Build and contribute

See [BUILD.md](docs/BUILD.md) for the tested LLVM path and the provided devkitA64 path. Portable host checks require Python 3.11+ and a C++17 host compiler:

```sh
python tools/test_host.py --cxx clang++
python tools/check_sdl_reference.py
```

Include firmware versions, module version, binary SHA256, reproduction steps and relevant sanitized logs in bug reports. Read [CONTRIBUTING.md](CONTRIBUTING.md). GitHub host-check automation tests synthetic inputs only; it does not test a Switch.

## Credits and license

Thanks to **cathery, o0Zz and sys-con contributors; Sam Lantinga, Valve and SDL contributors; ndeadly; switchbrew/libnx contributors; and devkitPro contributors**. This project incorporates and alters third-party code; it is not an implementation written entirely from scratch. See [CREDITS.md](CREDITS.md) and [THIRD_PARTY.md](THIRD_PARTY.md) for the exact scope and pinned sources.

The combined derived project is distributed under **GNU GPL version 3** ([LICENSE](LICENSE)). Permissive components retain their original notices and terms. Binary releases must be accompanied by their corresponding source and build scripts; the release tooling generates a source archive alongside the SD package. No proprietary Nintendo SDK code, controller firmware or Nintendo artwork is bundled.
