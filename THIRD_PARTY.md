# Third-party sources and licensing

The combined sys-con-derived project is distributed under GNU GPL version 3; see [LICENSE](LICENSE). Component-specific permissive notices are retained. This document identifies actual incorporated code separately from research and build dependencies.

## Incorporated sys-con code — GPL version 3

- Original project: [cathery/sys-con, e79be831614952aa84f7feb48cea5b6329ff07dd](https://github.com/cathery/sys-con/tree/e79be831614952aa84f7feb48cea5b6329ff07dd).
- Vendored source: [o0Zz/sys-con, c0cd1ff355b4f04ca5cc62963f66572f11344553](https://github.com/o0Zz/sys-con/tree/c0cd1ff355b4f04ca5cc62963f66572f11344553).
- `vendor/syscon/` includes `SwitchUSBInterface`, `SwitchUSBEndpoint`, `SwitchUSBLock`, abstract USB/logger interfaces, logger declarations and status definitions. Original owners retain their rights; added provenance notices do not replace them.
- `source/main.cpp` adapts runtime, USB discovery, PSC power transitions and HDLS virtual-pad patterns. `gc2-usb.json` derives from the upstream process configuration.
- Changes are documented in [vendor/syscon/PROVENANCE.md](vendor/syscon/PROVENANCE.md). Prominent notices identify copied/altered files. Distribution preparation notices were added on 2026-11-22; functional adaptations were made during November 2026.
- The GPL text is included as `LICENSE`. The source and build scripts for the distributed module are included in the matching source release.

## Incorporated SDL logic — zlib license

Pinned driver: [SDL_hidapi_switch2.c at b0ccb9ef0fb3ee86aee690535ed232b81af776f0](https://github.com/libsdl-org/SDL/blob/b0ccb9ef0fb3ee86aee690535ed232b81af776f0/src/joystick/hidapi/SDL_hidapi_switch2.c), blob `bf0e22f68ecbb05053ab25862a171894934ec5f2`.

The unmodified reference is in `reference/SDL_hidapi_switch2.c`; it is not compiled. Its complete original license and contribution notice are also in `licenses/SDL-LICENSE`. It credits Copyright (C) 1997-2026 Sam Lantinga and code/logic contributed by Valve Corporation.

`source/gc2_protocol.cpp` and `source/gc2_device.cpp` are **altered adaptations**, not original SDL files. They extract GameCube USB initialization bytes, flash/calibration reads and decoding logic, remove SDL integration, add validation and publish through libnx. Supporting transport/capture tools reuse this extracted sequence. The GameCube-only scope and local input/output policies are gc2-usb decisions. These adaptations are plainly identified and retain the full SDL notice.

## Research, not incorporated MissionControl code

[ndeadly/switch2_controller_research at a3306b473acff0d6844fb1e288883a3940df0baf](https://github.com/ndeadly/switch2_controller_research/tree/a3306b473acff0d6844fb1e288883a3940df0baf) was consulted for descriptors, report layouts, headers and flash replies. This package links to that work; it does not redistribute its captures or documents. It is community reverse engineering, not an official Nintendo specification. MissionControl is credited as related research by the same author; none of its code or experimental binaries is incorporated here.

## Linked dependencies and build tools

| Component | Use / notice |
| --- | --- |
| [libnx 4.12.0](https://github.com/switchbrew/libnx/tree/v4.12.0) | Linked runtime and service wrappers; ISC. Full notice in `licenses/libnx-LICENSE`. |
| devkitA64 GCC 15.2.0 runtime / libstdc++ | Official devkitPro target libraries; GPLv3 with applicable GCC Runtime Library Exception 3.1. See `LICENSE`, `licenses/GCC-RUNTIME-LIBRARY-EXCEPTION` and [upstream runtime licensing](https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html). Compiler binaries are not redistributed. |
| newlib | Target C runtime; collection of permissive notices in `licenses/NEWLIB-LICENSE`. |
| [switch-tools 22756068dd0ed6ff9734c59cb4f99ebd3f62555b](https://github.com/switchbrew/switch-tools/tree/22756068dd0ed6ff9734c59cb4f99ebd3f62555b) | Host ELF-to-NSO and NPDM tools; ISC, `licenses/switch-tools-COPYING`. Not linked into the module or bundled as executables. |
| [LZ4 ebb370ca83af193212df4dcbadcc5d87bc0de2f0](https://github.com/lz4/lz4/tree/ebb370ca83af193212df4dcbadcc5d87bc0de2f0) | Host converter dependency; BSD 2-Clause, `licenses/lz4-LICENSE`. |
| LLVM / Clang / LLD 22.1.3 | Existing host tools used for the alternative build; not redistributed. |
| Python, optional PyUSB and Python LZ4 | User-installed host tooling; not bundled. |

Exact image digest, target layer and source commits are recorded in `toolchain-lock.json`. The source ZIP does not contain the toolchain, generated host executables, Nintendo SDK code, controller firmware, keys or Nintendo artwork.

## Distribution

Keep `LICENSE`, this file, `CREDITS.md`, component notices and source-file notices intact. Identify modifications and their dates. When publishing binaries, publish the corresponding source archive and build instructions alongside them under the GPL terms. Do not describe this derivative as the original sys-con or SDL release. See [GPLv3](https://www.gnu.org/licenses/gpl-3.0.html) and [SDL licensing](https://libsdl.org/license.php).
