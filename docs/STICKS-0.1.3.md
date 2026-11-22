# Stick range adjustment: 0.1.3

Status: 2026-11-22. A tester reports normal behavior on Switch 2, but intermittent incomplete travel through this module on Switch V1: a dash attempt may walk or fail to move. The main stick is most noticeable; C-stick involvement is uncertain. No corresponding raw movement capture or physical extrema measurement is available.

## Hypothesis and implementation

Earlier output normalized each axis by the range read from controller calibration. If mechanical extrema produce smaller or varying values, output may not reach the expected extreme. This is a hypothesis, not a proven root cause. Missing movement can also involve input continuity; range correction does not automatically explain or fix it.

The adjustment is applied immediately before virtual Pro Controller publication. It does not change USB report offsets, factory/user calibration, initialization bytes or button/trigger mapping.

For each calibrated stick vector with default outer percentage 85:

- Up to radial magnitude 0.5, preserve previous values exactly.
- Between 0.5 and 0.85, expand magnitude linearly to 1, preserving direction.
- From 0.85 to 1, publish unit magnitude in the same direction.
- Preserve original above-unit vectors so existing diagonal travel is not reduced.
- Do not learn from previous motion, add a deadzone, synthesize button presses or generate a dash.

The 85% default is an adjustable output policy, not a Nintendo specification or a measurement of this controller. A setting of **100 bypasses the adjustment exactly**, including diagonals.

## Configuration

SD path: `config/gc2-usb/config.ini`.

```ini
[sticks]
left_stick_outer_percent=85
right_stick_outer_percent=85
stick_diagnostics_seconds=0
```

Outer percentages are independent integers from 60 through 100. Lower values reach full output sooner. Set `right_stick_outer_percent=100` to bypass only C-stick adjustment; set both to 100 to restore previous output. Invalid values are logged and ignored. Restart Atmosphere after changing settings.

## Required physical comparison

In offline Smash training with the same Pro Controller profile, compare light-tilt walking, left/right dashes from rest, quick reversals, vertical/diagonal motion and C-stick actions. Repeat each direction. Check that walking remains possible with the desired tilt. Compare default settings with both percentages at 100, changing no other settings. **The delivered adjustment has not yet been shown to resolve the tester's issue.**

## Optional measurement

1. Set `stick_diagnostics_seconds=60` and restart Atmosphere.
2. After connecting, rotate each stick fully and briefly hold all four cardinal extremes during the first minute. Include neutral.
3. The module accumulates pre-adjustment extrema in RAM and writes one summary at completion or disconnection, not a line per input report.
4. Record LX/LY/CX/CY extrema, percentages and which directions fail. Extrema are representative only if all extremes were actually exercised.
5. The largest observed gap between processed inputs is also summarized. It is not end-to-end latency or a direct USB polling-frequency measurement.
6. Restore `stick_diagnostics_seconds=0` and restart after measurement. Review/sanitize logs before sharing.

## Checks

191,115 synthetic/mathematical checks cover neutral/inner-zone identity, cardinal extrema, angular preservation with rounding, symmetry, monotonicity, no magnitude reduction, diagonals, bypass, independent options, parsing and range statistics. The protocol, simulated transport and pinned SDL checks remain in place. These checks do not measure a physical stick or Smash's internal thresholds.
