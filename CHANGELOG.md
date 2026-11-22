# Changelog

## 0.1.3 — experimental

- Add independent, configurable outer radial range adjustment for both sticks; default 85%, exact bypass at 100%.
- Preserve the inner 50% of travel and direction. Keep existing above-unit diagonal vectors unchanged.
- Add an optional bounded measurement window summarizing calibrated input extrema before adjustment and the largest observed input-processing gap.
- Add 191,115 synthetic/mathematical stick checks.
- A tester reported intermittent dash/travel issues before this adjustment. Its physical effectiveness is pending validation.

### English distribution preparation — 2026-11-22

- Translate and reconcile documentation, clearly separating historical results from current unverified behavior.
- Add prominent credits, file provenance/date notices and the complete SDL notice to binary documentation.
- Provide a clean source archive, release notes, checksums and host-check workflow. Exclude private logs, captures, local paths and generated host tools from repository/release sources.
- Runtime behavior is unchanged from the previously built 0.1.3 binary. No new hardware validation is claimed.

## 0.1.2 — experimental

- Keep initialization writes mandatory while allowing missing init replies, matching SDL's policy.
- Bound reads and skip mismatching replies without treating them as acknowledgements.
- Keep calibration reply validation mandatory; attach a virtual pad only after a valid report 05.
- Add 81 simulated transport checks.
- Gameplay was subsequently reported; the exact installed build was not reconfirmed with a fresh log.

## 0.1.1 — diagnostic

- Add target-specific USB availability events, interface/endpoint discovery logging and power-state diagnostics.
- Tester log on HOS 23.0.1 confirmed acquisition of HID and bulk interfaces and passing calibration checks; initialization reply handling then failed.

## 0.1.0 — initial experimental build

- Implement the single-device USB driver, SDL-based initialization/calibration, report 05 decoding and virtual Pro Controller mapping.
- Add synthetic protocol checks and an AArch64 build. Initial tester feedback reported no working input.
