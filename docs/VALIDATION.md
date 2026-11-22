# Staged validation

Status as of **2026-11-22**: synthetic host checks and the binary structure have passed. A 0.1.1 tester log on HOS 23.0.1 confirmed target interface acquisition and passing calibration checks, followed by initialization reply failures. After 0.1.2 was delivered, the tester reported input in Smash, with intermittent travel/dash issues, mostly on the main stick and possibly the C-stick. The exact gameplay build was not independently reconfirmed by a new log. The 0.1.3 adjustment has **not** been physically validated. No complete Smash compatibility matrix exists.

Private logs and local build transcripts are intentionally excluded from publication. The historical summaries below are reports, not fabricated replacement logs.

## A. Host and format checks — passed

`tools/test_host.py` compiles the actual portable implementations with a host C++ compiler:

- **20,150 protocol checks:** button bits/combinations, centers/extrema of all axes, trigger limits/hysteresis, invalid/truncated calibration, report ID/length rejection, flash reply correlation and 10,000 random synthetic reports.
- **81 transport checks:** simulated endpoints, mandatory/optional timeouts, foreign headers, later matching replies, multi-packet flash responses, size limits and hard I/O failures.
- **191,115 stick checks:** inner-zone identity, cardinal maxima, angle/rounding, symmetry, monotonicity, no range reduction, diagonal preservation, exact bypass, independent configuration, parser and extrema statistics.

`tools/check_sdl_reference.py` checks all ten initialization commands against the unmodified pinned SDL file. The fixtures are constructed, not real captures. These checks do not execute `usb:hs`, PSC or HDLS.

`tools/verify_binary.py` checks AArch64 ELF/NSO/NPDM structure, relative relocations, decompressed segment identity/hashes and program ID. Release assets include its verification JSON. The standard devkitA64/Docker route remains unexecuted here.

## B. PC with real controller — input capture pending

SetupAPI enumeration identified `057e:2073`, HID interface 0 and WinUSB interface 1. The native Windows probe's open attempt returned `ERROR_ACCESS_DENIED` (5), including when launched through Windows UI. No calibration/input capture was obtained there. No driver replacement or elevation was performed. An exclusively sandbox-related cause was not established.

Linux with libusb/PyUSB and access to both target interfaces is the preferred independent capture path:

```sh
python -m pip install pyusb
python tools/capture_usb.py --out captures/descriptors.json
python tools/capture_usb.py --capture --seconds 10 --label neutral --out captures/neutral.json
```

Default mode reads descriptors only. `--capture` claims target interfaces, reads calibration and runs the SDL initialization sequence, releasing interfaces and reattaching any detached kernel drivers on completion. It is not a firmware updater. Capture separately: neutral, each button, D-pad, triggers gradually/full click, main stick and C-stick at center/cardinals/diagonals. Record action timing and verify 64-byte report `05` changes accordingly. Never parse native `0a` using the common layout. Review captures before sharing them. Do not replace unrelated Windows drivers just to obtain a capture.

**Legacy-tool limitation:** the Python capture helper still requires matching initialization replies, unlike the module/native C++ probe since 0.1.2. It can therefore stop on a controller the module accepts. It is not an equivalent test of current initialization behavior; do not use that failure as evidence against module compatibility. The native probe shares the updated C++ transport but has the Windows access limitation described above.

## C. Switch system controls — partial historical evidence; full pass pending

Record model, HOS, exact Atmosphere version, controller firmware if known and binary SHA256. Start with one controller and a dock; treat OTG as a separate test.

1. Boot without the controller: no freeze, phantom pad or acquisition of unrelated USB devices.
2. Connect: verify both interfaces, successful calibration, first valid report `05` and HDLS attachment in the log.
3. Test all mapped buttons in the system input test, including HOME/Capture where supported.
4. Check centered stability, direction of all axes, cardinal/diagonal extremes and no phantom movement.
5. Verify L/R threshold activation, full-click activation and release. Check Z and additional ZL independently.
6. Change order/grip, enter/exit applets and return to the game: no lost or duplicate virtual pad.

Pass only if each input is correct and stable without USB/HDLS errors. Initial discovery or calibration alone is insufficient.

## D. Super Smash Bros. Ultimate — gameplay reported; complete pass pending

Use offline training and the **Pro Controller** profile. Record game version, in-game bindings and stick settings. Change one variable at a time.

| Test | Pass criterion |
| --- | --- |
| Face buttons, START, D-pad | Expected action for each press, no swapped/stuck inputs |
| Main stick | Consistent walking, dashing, direction changes, crouching and enabled tap jump |
| C-stick | Correct configured attacks/tilts in cardinal and diagonal directions |
| L/R travel and full click | Consistent mapped action, no required full click, no stuck button |
| Z and ZL | Independent configured actions |
| Combinations | Shield/direction, attack/stick and jump/attack without missing input |
| Menus and match transitions | Player selection, start, pause, return without duplicate pad |
| Endurance | At least 30 minutes, recording disconnects, apparent delay and errors |

Trigger output here is digital Pro Controller L/R. Do not claim native analog GameCube shield sensitivity. No end-to-end latency measurement is available; the USB timeout is not a latency measurement. See [STICKS-0.1.3.md](STICKS-0.1.3.md) for comparing compensation enabled and bypassed.

## E. Recovery and power — pending

Disconnect while holding a button/stick and verify input releases. Repeat connect/disconnect ten times, change USB port and disconnect during initialization and reading. Suspend/resume with the cable connected/disconnected; check actual sleep and reconnection after wake. Test cold boot with the controller attached and closing/reopening the game. Keep sanitized crash reports if a failure occurs.

## Recording results

For each stage record date, environment, installed `exefs.nsp` SHA256, steps, expected/actual behavior, evidence and repetitions. Use `PASS`, `FAIL` or `NOT RUN`. Never turn `NOT RUN` into `PASS` because it compiles. Compatibility claims apply only to the combinations actually tested. Build manifests' false hardware flags describe the current exact binary's unverified physical status; they do not erase the historical tester feedback described above.
