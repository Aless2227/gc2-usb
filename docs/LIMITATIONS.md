# Known limitations

Status: 2026-11-22, version 0.1.3.

- Only the official Switch 2 GameCube controller, USB VID/PID `057e:2073`, is accepted. One controller is supported at a time.
- USB only. No Bluetooth transport, BLE pairing, Bluetooth wake or MissionControl integration is included.
- It publishes a virtual Pro Controller, not a four-port GameCube adapter or the Switch 2 wireless GameCube identity. Use the Pro Controller profile in games. There is no custom GameCube menu icon.
- L/R trigger travel is decoded but published as digital L/R with hysteresis. This is not native analog GameCube trigger output.
- No runtime rumble, motion or player LED control. The exact SDL initialization sequence includes feature-enable commands; this is not a claim of implemented output features.
- It shares sys-con's program ID and PSC slot and cannot run alongside sys-con. Compatibility with other input sysmodules has not been established.
- Working input in Smash was reported by a tester. Intermittent dash/travel problems remain under investigation. The 0.1.3 outer-range adjustment is not yet physically validated; do not advertise it as a proven fix.
- There is no complete per-button, game, firmware, dock/OTG, suspension/reconnection or endurance test matrix, and no end-to-end latency measurement.
- Host checks use synthetic reports and simulated endpoints. Build/format verification does not exercise Horizon services or a physical controller.
- The native Windows probe could enumerate the target, but opening WinUSB returned access denied. No real input capture was obtained on that PC. This does not establish the cause of any Switch-side issue.
- The optional Python capture helper retains stricter historical initialization-reply checks than the module. A failure there does not demonstrate failure of the current module's initialization policy.
- The alternative LLVM build was executed. The standard devkitA64 Makefile/Docker path is supplied but was not executed during preparation of this release.

See [VALIDATION.md](VALIDATION.md) before expanding compatibility claims.
