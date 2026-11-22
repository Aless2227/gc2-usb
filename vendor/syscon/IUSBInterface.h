/* gc2-usb provenance notice added 2026-11-22.
 * Copied code; provenance notice added from o0Zz/sys-con (GPL version 3).
 * Upstream: src/controllerlib/IUSBInterface.h
 * Revision: c0cd1ff355b4f04ca5cc62963f66572f11344553.
 * Original work remains credited to upstream authors/contributors.
 * Functional adaptations, where present, made during November 2026.
 * See PROVENANCE.md, ../../THIRD_PARTY.md and ../../LICENSE.
 */
#pragma once
#include "IUSBEndpoint.h"

namespace controllerlib
{
    class IUSBInterface
    {
    protected:
    public:
        struct InterfaceDescriptor
        {
            uint8_t bLength;
            uint8_t bDescriptorType;
            uint8_t bInterfaceNumber;
            uint8_t bAlternateSetting;
            uint8_t bNumEndpoints;
            uint8_t bInterfaceClass;
            uint8_t bInterfaceSubClass;
            uint8_t bInterfaceProtocol;
            uint8_t iInterface;
        };
        virtual ~IUSBInterface() = default;

        virtual Status Open() = 0;
        virtual void Close() = 0;

        virtual Status ControlTransferInput(uint8_t bmRequestType, uint8_t bmRequest, uint16_t wValue, uint16_t wIndex, void *buffer, uint16_t *wLength) = 0;
        virtual Status ControlTransferOutput(uint8_t bmRequestType, uint8_t bmRequest, uint16_t wValue, uint16_t wIndex, const void *buffer, uint16_t wLength) = 0;

        virtual IUSBEndpoint *GetEndpoint(IUSBEndpoint::Direction direction, uint8_t index) = 0;
        virtual InterfaceDescriptor *GetDescriptor() = 0;
    };
} // namespace controllerlib
