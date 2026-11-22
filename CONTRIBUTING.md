# Contributing

Keep support limited to the official Switch 2 GameCube controller over USB unless a scope change is explicitly agreed. Do not infer undocumented protocol semantics from a plausible packet, invent calibration constants, or claim hardware compatibility because a build succeeds.

Before proposing changes, run:

```sh
python tools/test_host.py --cxx clang++
python tools/check_sdl_reference.py
```

For target changes also build and run `tools/verify_binary.py` with Python LZ4 installed. Host tests cannot replace hardware validation. Document the environment, actual test procedure and remaining uncertainties. Mark synthetic data as synthetic and retain the origin/license of borrowed code. Follow the existing GPLv3 distribution terms and component-specific notices.

## Bug reports

Include:

- gc2-usb version and SHA256 of the installed `exefs.nsp`.
- Console model, HOS and Atmosphere versions; controller firmware if known.
- Dock/OTG and cable used, and other relevant input sysmodules.
- Exact reproduction steps, expected result, actual result and repeatability.
- Sanitized relevant log lines and whether the problem occurs in system input tests, Smash, or both.

For stick problems include the active settings and which stick/directions fail. See [stick diagnostics](docs/STICKS-0.1.3.md). Remove personal paths, identifiers and unrelated device data before uploading logs. Do not upload console keys, firmware, proprietary SDK files or unrequested raw captures.
