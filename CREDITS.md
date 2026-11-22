# Credits

gc2-usb is based on the work of the following projects and contributors. Credit does not imply that they endorse, maintain or have tested this derivative.

| Project / contributors | Contribution to gc2-usb |
| --- | --- |
| [cathery/sys-con](https://github.com/cathery/sys-con) and contributors | Original USB sysmodule architecture, runtime/power-management patterns and virtual controller design. |
| [o0Zz/sys-con](https://github.com/o0Zz/sys-con) and contributors | Source of the vendored USB interface, endpoint and locking code, supporting interfaces, logger declarations and status types. Several files are modified. |
| [SDL](https://github.com/libsdl-org/SDL), Sam Lantinga, Valve Corporation and contributors | Switch 2 USB command sequence, flash/calibration reads and input-processing logic adapted for the GameCube-only driver. The original driver credits Valve for code and logic. |
| [ndeadly/switch2_controller_research](https://github.com/ndeadly/switch2_controller_research) | Public reverse-engineering documentation used to cross-check descriptors, reports and commands. MissionControl code is not included. |
| [switchbrew/libnx](https://github.com/switchbrew/libnx) and contributors | Switch runtime and service interfaces. |
| [devkitPro](https://devkitpro.org/) and contributors | Target libraries, headers and the standard cross-compilation environment. |
| [switchbrew/switch-tools](https://github.com/switchbrew/switch-tools) and [LZ4](https://github.com/lz4/lz4) contributors | Host conversion tools used when building the release. Their executables are not bundled in the source archive. |
| LLVM contributors | Existing Clang/LLD toolchain used for the alternative build. LLVM binaries are not redistributed. |

Project-specific work includes the single-device integration, validation and bounded transport policy, input mapping, configurable stick-range policy, diagnostics, tests, packaging and documentation. These changes build on the credited work above. No individual author's legal name is inferred or assigned by this package.

See [THIRD_PARTY.md](THIRD_PARTY.md) for commits, licenses and modification details. Original copyright and permission notices remain applicable.
