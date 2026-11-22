# Vendored sys-con provenance

Source: [o0Zz/sys-con at c0cd1ff355b4f04ca5cc62963f66572f11344553](https://github.com/o0Zz/sys-con/tree/c0cd1ff355b4f04ca5cc62963f66572f11344553), derived from cathery/sys-con. GPL version 3; see [LICENSE](../../LICENSE) and [THIRD_PARTY](../../THIRD_PARTY.md). Credit belongs to the upstream authors and contributors, not solely the names in the repository URLs.

The table compares the preparation input against the pinned original files before adding gc2-usb's prominent provenance notices. Original hashes identify upstream bytes, not the modified local files. Notices were added on 2026-11-22; functional adaptations were made during November 2026.

| Local file | Upstream path | Classification | Original SHA256 |
| --- | --- | --- | --- |
| `ILogger.h` | `src/controllerlib/ILogger.h` | Copied code; provenance notice added | `d79ea2a81316cd87b872bee861eaa24d945b2bf4c2753fda9bc152d73e3f05c6` |
| `IUSBEndpoint.h` | `src/controllerlib/IUSBEndpoint.h` | Copied code; provenance notice added | `6b68dc444caecfff8f2df410540818c0f6671804ab265da3443bd9ac8a1d68fd` |
| `IUSBInterface.h` | `src/controllerlib/IUSBInterface.h` | Copied code; provenance notice added | `30e8ddfd01ed44ae0fc381c66bcdeb5855408c8453de4e30f86ed79253d02539` |
| `Status.cpp` | `src/controllerlib/Status.cpp` | Copied code; provenance notice added | `f8c4794f8000bad9cfe0a66683b640e34f569cdc1d3c64c28cb0797fd4a7dac9` |
| `Status.h` | `src/controllerlib/Status.h` | Copied code; provenance notice added | `d904a3e6be95db6c431d0704da5fe28181f28e91565f368b416c6c9522f7a8d8` |
| `SwitchLogger.h` | `src/platform/SwitchLogger.h` | Copied code; provenance notice added | `cab61c9a7baf376e2b8f7b82d745fdceb8892b767aee023ad3b09f4a1b73eaff` |
| `SwitchUSBEndpoint.cpp` | `src/platform/SwitchUSBEndpoint.cpp` | Altered adaptation | `4c1c12f9ec58edbe6346575d2532d1d41bd92962836cbdaa8e3e1ef214a98164` |
| `SwitchUSBEndpoint.h` | `src/platform/SwitchUSBEndpoint.h` | Altered adaptation | `7e70dd1faa4a45e0de8b7a2783e2f703cbd76f64db457ac24163ffad457a7580` |
| `SwitchUSBInterface.cpp` | `src/platform/SwitchUSBInterface.cpp` | Altered adaptation | `728e167034546ce2872bf7789b47b3ab5fd9270f1eb0cb4af0bcb4e7916c4246` |
| `SwitchUSBInterface.h` | `src/platform/SwitchUSBInterface.h` | Altered adaptation | `c7b1985d7da423f69356ddb96fd167d66b2094662df65d72c049945e6a33ca7d` |
| `SwitchUSBLock.cpp` | `src/platform/SwitchUSBLock.cpp` | Copied code; provenance notice added | `c557f35802a90d507cce36f676ff656594d59594bfee2ccaf1f17d49e0ed5c93` |
| `SwitchUSBLock.h` | `src/platform/SwitchUSBLock.h` | Copied code; provenance notice added | `18bbc84cd9d13e11504bf485da2582769b9af115c2f6733939af3564cae1e4d2` |

Local functional changes include idempotent cleanup of partial initialization, owned-resource release in destructors, distinguishing pending transfer state from a zero transfer ID, bounded asynchronous writes, transfer-size validation, buffer limits, and interrupt-only interval handling. Supporting interface/logger/status files are retained to build the reduced USB layer; full upstream controller support is not incorporated.

The reduced module runtime in `source/main.cpp` and the process configuration also adapt upstream design. They are outside this vendored directory. gc2-usb's own transport, decoding and stick policy are documented separately; SDL-derived portions keep SDL notices.
