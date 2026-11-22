# Protocol research and evidence

Initial research: 2026-11-05. Status reconciled: 2026-11-22. This is community reverse engineering, not an official Nintendo protocol specification.

## USB feasibility

The official Switch 2 GameCube controller has a usable **USB data transport**, not merely USB charging. The primary implementation evidence is [SDL's pinned Switch 2 driver](https://github.com/libsdl-org/SDL/blob/b0ccb9ef0fb3ee86aee690535ed232b81af776f0/src/joystick/hidapi/SDL_hidapi_switch2.c), which identifies PID `2073`, exchanges bulk commands, reads calibration and handles GameCube input. Public descriptors and protocol documentation are available in [ndeadly's pinned research](https://github.com/ndeadly/switch2_controller_research/tree/a3306b473acff0d6844fb1e288883a3940df0baf).

Subsequently, a tester's 0.1.1 log on Switch V1/HOS 23.0.1 confirmed acquisition of both target interfaces and passing calibration checks. Initialization stopped later while waiting for command replies. After reply handling changed, working gameplay was reported, with intermittent stick issues; the exact gameplay build was not reconfirmed by a fresh log. See [VALIDATION.md](VALIDATION.md).

## Source basis

1. [cathery/sys-con](https://github.com/cathery/sys-con/tree/e79be831614952aa84f7feb48cea5b6329ff07dd): runtime, USB discovery, PSC and virtual-pad design.
2. [o0Zz/sys-con](https://github.com/o0Zz/sys-con/tree/c0cd1ff355b4f04ca5cc62963f66572f11344553): incorporated USB interface/endpoint/lock code, with local resource and transfer changes. Other controller drivers are not included.
3. [SDL](https://github.com/libsdl-org/SDL/blob/b0ccb9ef0fb3ee86aee690535ed232b81af776f0/src/joystick/hidapi/SDL_hidapi_switch2.c): exact ten-command USB initialization sequence, flash/calibration reads and common input format. The unmodified source is retained under `reference/`; the reference check compares initialization bytes and ordering.
4. [Descriptors](https://github.com/ndeadly/switch2_controller_research/blob/a3306b473acff0d6844fb1e288883a3940df0baf/descriptors.md#nso-gamecube-controller): `057e:2073`, HID interface 0 with interrupt IN/OUT and vendor interface 1 with bulk IN/OUT. The driver locates endpoints by direction/type rather than assuming another controller's topology.
5. [Reports](https://github.com/ndeadly/switch2_controller_research/blob/a3306b473acff0d6844fb1e288883a3940df0baf/hid_reports.md): common report `05` and native GameCube report `0a` differ. This driver explicitly selects `05` as SDL does and rejects `0a` rather than applying incompatible offsets. GATT descriptions can omit the USB report-ID byte; do not mix their indices.
6. [Commands](https://github.com/ndeadly/switch2_controller_research/blob/a3306b473acff0d6844fb1e288883a3940df0baf/commands.md): command headers and flash block read `02/01`. Calibration replies require matching command, subcommand, length and address. There is no invented universal acknowledgement value.

## Implemented report layout

These are **zero-based USB indices including report ID**, only for a complete 64-byte common report `05`:

| Field | Bytes |
| --- | --- |
| Report ID | 0 = `05` |
| Buttons | 5, 6, 7 |
| Main stick, two 12-bit axes | 11-13 |
| C-stick, two 12-bit axes | 14-16 |
| Analog L/R triggers | 61, 62 |

For each packed axis pair: `x = b0 | ((b1 & 0x0f) << 8)` and `y = (b1 >> 4) | (b2 << 4)`. Publication uses positive Y upward for libnx rather than copying SDL's output-axis inversion for its different convention.

Factory stick data is taken from blocks `13080/130c0` at payload offset hexadecimal `28`, trigger zeros from `13140`, and user stick data from `1fc040/1fc080` only when marked `b2 a1`. The read sequence also includes `13000`, `13040` and `13100`, following SDL. Centers and ranges are validated; invalid calibration rejects the session. Trigger normalization uses SDL's observed endpoint `232`; physical verification for this unit remains pending.

The trigger press/release thresholds (25%/20%), Pro Controller identity, C-to-Minus mapping, one-controller limit, retry times and outer-stick policy are **local implementation decisions**, not Nintendo protocol facts. The outer-stick default 85% is not measured calibration.

## Evidence limits

- Some initialization commands have unknown semantics even in the reference. Bytes and order are preserved without inventing meanings.
- No raw input packet capture from the tester's controller is bundled. Synthetic fixtures are not presented as captures.
- There is no measured input latency, noise, firmware-wide compatibility, full control matrix or endurance/suspend-reconnect result.
- The Windows probe enumerated the device but failed to open WinUSB; this is not evidence that USB input is unsupported on Switch.
- Only the USB transport is implemented. Public Bluetooth research does not mean this module supports Bluetooth.
- User feedback about working gameplay is meaningful but does not physically validate the current 0.1.3 stick adjustment.
