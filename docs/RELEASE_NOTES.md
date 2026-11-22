# gc2-usb 0.1.3 — experimental USB support

Use the official Nintendo Switch 2 wireless GameCube controller (`057e:2073`) over USB on an original Nintendo Switch running Atmosphere. One controller, USB only; published as a virtual Pro Controller.

## What's included

- Buttons, both analog sticks and decoded/calibrated triggers mapped to digital Pro Controller L/R.
- Configurable outer-range adjustment for each stick; default 85%, exact bypass at 100%.
- Optional summarized stick diagnostics, complete source/build scripts, licensing notices and English documentation.

## Evidence and limitations

A tester confirmed target interface acquisition/calibration in a 0.1.1 log on HOS 23.0.1. After the initialization-reply change, working Smash gameplay was reported, with intermittent stick/dash issues; the exact installed gameplay build was not independently reconfirmed. The 0.1.3 adjustment is **not yet physically validated** and is not advertised as a proven fix. There is no complete game/control/stability matrix or measured latency.

20,150 protocol, 81 simulated transport and 191,115 stick checks pass; ten initialization commands match the pinned SDL source. The AArch64 binary was built with LLVM and official devkitPro target libraries and structurally verified. The provided standard devkitA64/Docker build path has not been executed here. Host checks and successful compilation do not prove hardware compatibility.

English publication preparation changes documentation, notices and packaging only; the SD asset retains the previously built 0.1.3 module binary. There is no Bluetooth, native analog GameCube trigger output, rumble, motion, player LED control or GameCube menu icon.

## Installation

Download the **experimental SD ZIP**, not the source ZIP. Fully power off, back up any existing content folder and extract to the SD root. Create empty `atmosphere/contents/690000000000000D/flags/boot2.flag` to enable startup; it is deliberately absent from the ZIP. Connect using a USB data cable at the dock. Use the Pro Controller profile in Smash.

This module replaces sys-con and must not run alongside it: both use program ID `690000000000000D` and PSC slot 126. Installing sys-con separately is unnecessary. Remove `boot2.flag` with the console off to disable the module. Read `docs/INSTALL.md` and `docs/VALIDATION.md` before testing.

## Credits and source

Derived from cathery/sys-con and o0Zz/sys-con, with protocol logic adapted from SDL (Sam Lantinga, Valve and contributors) and research by ndeadly. Uses libnx and devkitPro target libraries. See CREDITS, THIRD_PARTY and component license notices.

The combined derivative is GPLv3. The corresponding source ZIP and build instructions accompany the binary. SHA256SUMS identifies the attached archives/reports. This is an independent community project, not an official Nintendo release.
