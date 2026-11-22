/* gc2-usb provenance notice added 2026-11-22.
 * Copied code; provenance notice added from o0Zz/sys-con (GPL version 3).
 * Upstream: src/controllerlib/Status.cpp
 * Revision: c0cd1ff355b4f04ca5cc62963f66572f11344553.
 * Original work remains credited to upstream authors/contributors.
 * Functional adaptations, where present, made during November 2026.
 * See PROVENANCE.md, ../../THIRD_PARTY.md and ../../LICENSE.
 */
#include "Status.h"

namespace controllerlib
{
    const char *ToString(Status status)
    {
        switch (status)
        {
            case Status::Success:
                return "Success";
            case Status::InvalidEndpoint:
                return "InvalidEndpoint";
            case Status::NothingTodo:
                return "NothingTodo";
            case Status::NotImplemented:
                return "NotImplemented";
            case Status::UnexpectedData:
                return "UnexpectedData";
            case Status::InvalidArgument:
                return "InvalidArgument";
            case Status::HidIsNotJoystick:
                return "HidIsNotJoystick";
            case Status::NoInterfaces:
                return "NoInterfaces";
            case Status::NoDataAvailable:
                return "NoDataAvailable";
            case Status::OutOfMemory:
                return "OutOfMemory";
            case Status::UsbInterfaceAcquire:
                return "UsbInterfaceAcquire";
            case Status::OpenFailed:
                return "OpenFailed";
            case Status::WriteFailed:
                return "WriteFailed";
            case Status::ReadFailed:
                return "ReadFailed";
            case Status::Timeout:
                return "Timeout";
            case Status::UsbEndpointOpen:
                return "UsbEndpointOpen";
            case Status::InvalidIndex:
                return "InvalidIndex";
            case Status::HidIsNotKeyboard:
                return "HidIsNotKeyboard";
            case Status::HidIsNotMouse:
                return "HidIsNotMouse";
            case Status::UnknownError:
                return "UnknownError";
        }

        return "Status(?)";
    }
} // namespace controllerlib
