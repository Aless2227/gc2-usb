# Historical diagnostic: 0.1.0 to 0.1.1

This is a historical summary, not current installation guidance. Install the current package using [INSTALL.md](INSTALL.md). Private tester logs are not redistributed.

## Initial report

The tester reported no working input. The initial log identified gc2-usb 0.1.0 and HOS 23.0.1 and recorded a power transition. That confirmed module startup only, not USB discovery. Without timestamps or discovery snapshots, it could not identify the cause. A power transition alone is not an error.

## PC evidence

SetupAPI identified the real device `057e:2073`, revision `0101`, with HID interface 0 using HidUsb and interface 1 using WinUSB. The native probe could not open WinUSB (`ERROR_ACCESS_DENIED`, 5), even when launched through Windows UI. No real input was captured. No driver installation, elevation or controller firmware change was performed. The reason was not shown to be exclusively sandboxing, nor does it explain the Horizon-side issue.

## 0.1.1 changes

- Add a target-specific USB availability event and retain periodic discovery. There was no proof that absence of this event caused the original failure.
- Log monotonic timestamps and available/all interface snapshots. Diagnostic `QueryAllInterfaces` IDs are never used for acquisition.
- Add topology, endpoint, calibration, command and report diagnostics.
- Resume discovery on both `ReadyAwaken` and `Awake`, following sys-con's power-state design. Physical effectiveness of this change was not independently isolated.
- Keep initialization bytes, VID/PID, report layout and button mapping unchanged.

## Subsequent evidence

The 0.1.1 tester log confirmed that both interfaces were found and acquired and calibration checks passed. Initialization then failed waiting for replies. This supersedes the initial absence of discovery evidence and motivated [FIXES-0.1.2.md](FIXES-0.1.2.md). It did not yet establish working gameplay or full compatibility.
