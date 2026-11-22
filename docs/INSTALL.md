# Installation and removal

## Requirements and status

An original Nintendo Switch already running Atmosphere, an accessible microSD, a USB host connection (normally a dock), a USB data cable and the official Switch 2 wireless GameCube controller (`057e:2073`) are required. gc2-usb contains no Bluetooth support.

USB acquisition and calibration checks were confirmed in a 0.1.1 log on HOS 23.0.1. Gameplay was subsequently reported, with intermittent stick/dash issues. The 0.1.3 adjustment is not physically validated. Exact Atmosphere/controller firmware versions and a complete compatibility matrix have not been established.

## First installation

1. Fully power off the console and remove the SD card.
2. Back up any existing `atmosphere/contents/690000000000000D/` folder elsewhere, then remove the old content folder from `contents`. This prevents an old `boot2.flag` from enabling the module before you choose to do so. gc2-usb replaces sys-con: both use program ID `690000000000000D` and PSC slot 126. Keep any old `config/sys-con` folder if you want to restore sys-con later. Installing sys-con separately is not required.
3. Extract `gc2-usb-0.1.3-experimental.zip` to the SD root. The executable must be at:

   ```text
   atmosphere/contents/690000000000000D/exefs.nsp
   ```

   Settings go at `config/gc2-usb/config.ini`. `gc2-usb-docs/` contains documentation and notices; it is not needed for execution. Retain those notices with the package when redistributing it.
4. Enable automatic startup by creating an empty file at exactly:

   ```text
   atmosphere/contents/690000000000000D/flags/boot2.flag
   ```

   The ZIP does not contain this file. On Windows, check that it is not accidentally named `boot2.flag.txt`. The `.keep` file is harmless and does not enable the module.
5. Put the SD back, boot Atmosphere and connect the controller to a dock USB port using the data cable. No wireless pairing is needed. OTG can be investigated separately; a complete OTG validation is pending.
6. Inspect `config/gc2-usb/log.txt`. `First valid USB input report 05 received` followed by `HDLS virtual Pro pad attached` identifies successful input decoding and virtual-pad attachment. Startup or initialization messages alone do not prove working input.
7. Test controls in the system menu, then follow [VALIDATION.md](VALIDATION.md). In Smash, select the **Pro Controller** profile because that is the identity gc2-usb publishes.

## Updating an enabled installation

With the console fully powered off, back up the existing executable and settings. Replace `exefs.nsp` and merge the new settings into `config.ini`. Keep your existing `boot2.flag` if you want automatic startup to remain enabled. Do not overwrite a carefully adjusted configuration without saving it.

Settings are read at module startup; restart Atmosphere after editing. gc2-usb does not read sys-con INI files. See [STICKS-0.1.3.md](STICKS-0.1.3.md) for the optional adjustment and measurement window.

## Disable, remove or recover

Fully power off, access the SD from a PC and remove only `atmosphere/contents/690000000000000D/flags/boot2.flag` to disable startup. To remove gc2-usb, remove its content folder, or restore the backed-up sys-con folder. This also works if the console cannot reach HOME. The configuration/log folder may be kept for diagnosis.

The module does not write controller flash or update controller firmware. On input loss it attempts to neutralize input after 100 ms and release the session after 500 ms. These are implementation policies, not measured latency or verified recovery guarantees.

For failures, record version, binary SHA256, HOS/Atmosphere versions, cable/dock, controller firmware if known and a sanitized log. Do not alter device IDs or packet offsets just to accept an unknown device.
