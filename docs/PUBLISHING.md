# Publishing this prepared repository

The outer GitHub-ready ZIP contains a `gc2-usb/` directory. Extract it, then put **the contents of that directory** at the repository root. Its `README.md`, `LICENSE`, `source/`, `include/`, `tools/` and other source folders should be directly visible on GitHub.

`release-assets/` is a prepared upload folder, ignored by Git. It contains the SD installation ZIP, the corresponding source ZIP, verification results, release notes and checksums. Do not copy the GitHub-ready ZIP or source ZIP to the SD as an installation method.

## Suggested first publication

1. Create an empty GitHub repository named `gc2-usb` (or your preferred name). Keep the existing GPLv3 `LICENSE`; do not replace it with a conflicting license.
2. Upload/commit the prepared source tree. With Git, `git add .` respects `.gitignore`, so local build files and `release-assets/` are omitted. If using the website, omit those folders manually and include hidden `.github/`, `.gitignore` and `.gitattributes` files.
3. Check that CREDITS, THIRD_PARTY, component notices and all source/build files are present. Inspect the diff before committing. No credentials, raw tester logs or toolchain archives belong in the repository.
4. Tag the source commit as `v0.1.3-experimental` and create a GitHub **pre-release** at that tag. Use `release-assets/RELEASE_NOTES.md` as the description.
5. Attach `gc2-usb-0.1.3-experimental.zip`, `gc2-usb-0.1.3-source.zip`, `verification.json`, `host-checks.json`, `RELEASE_NOTES.md` and `SHA256SUMS.txt` from `release-assets/`. Publishing the matching explicit source ZIP alongside the binary makes the source/build scripts easy to obtain. Keep them available while the binary is available.
6. Check the README's installation filename against the attached asset. The prepared README uses no guessed repository owner or placeholder download URL.

The package has not been pushed, uploaded or released automatically. GitHub Actions will run only after publication; the workflow itself has not been run on GitHub during preparation. Its synthetic host tests are not hardware compatibility tests.

## Rebuilding assets

After building the target and installing Python LZ4:

```sh
python tools/test_host.py --cxx clang++
python tools/check_sdl_reference.py
python tools/verify_binary.py
python tools/package.py
python tools/assemble_delivery.py
```

The assembler packages allowlisted source/documentation files only. It excludes `.git`, toolchains, captures, logs, host executables and generated build objects. It requires a passing independent binary-verification report matching the NSP SHA256. A host-check result is included only if it has been explicitly recorded; do not fabricate physical test results in metadata.

English distribution preparation changes documentation, provenance comments and release tooling only. The initial release assets retain the existing 0.1.3 module binary; the new stick policy is still pending physical validation. For future functional changes, update version metadata, tests, validation status and changelog, rebuild, and publish matching sources for the actual binary.
