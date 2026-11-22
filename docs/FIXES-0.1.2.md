# Initialization reply handling: 0.1.2

## Evidence from 0.1.1

A tester log on HOS 23.0.1 showed two available `057e:2073` interfaces on the same USB device: HID 0 and vendor bulk 1. Both were acquired and endpoints opened. Calibration checks passed before initialization stopped: first a timeout on `0c/02`, then an eight-byte mismatching reply on `0a/08` in later attempts.

Rejected reply headers and calibration values were not printed by that version. Delayed or unrelated replies were hypotheses, not captured facts. A separately connected Bluetooth controller mentioned by the tester was a different device; gc2-usb has no Bluetooth transport.

## Difference from SDL

The pinned `HIDAPI_DriverSwitch2_InitUSB` checks command writes but does not require `RecvBulkData` to succeed for each of the ten initialization commands. Earlier gc2-usb versions imposed correlated replies on all of them. That extra restriction stopped the tester's initialization sequence.

## Changes

- Writes remain mandatory; write failures stop initialization.
- Replies to the ten initialization commands become optional, matching SDL's policy. Logs distinguish sent commands from matching replies.
- Read at most six packets per exchange with 100 ms per-read waits. These are bounded local policies, not protocol constants.
- Skip mismatching headers within the bound and log them. Never treat them as acknowledgements of the current command.
- Keep calibration replies, address/length checks and value validation mandatory. No guessed calibration fallback is introduced.
- Attach a virtual pad only after decoding a complete 64-byte input report `05`. A sent initialization sequence is not proof of working input.
- Keep initialization bytes/order, VID/PID, report offsets and mapping unchanged. No speculative commands, Bluetooth or flash writes are added.

The portable command channel in `gc2_transport.cpp` is shared with the native PC probe. Its 81 synthetic checks cover optional/mandatory timeouts, foreign replies, later matching replies, multi-packet flash reads, bounds and I/O errors. Protocol checks are retained.

## Current interpretation

After this version was delivered, a tester reported gameplay input in Smash with intermittent stick/dash problems. The exact installed build was not reconfirmed by a fresh log, and the full hardware checklist remains incomplete. Version 0.1.3 addresses an output-range hypothesis; it has not been proven to resolve the issue. See [VALIDATION.md](VALIDATION.md).
