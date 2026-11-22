/* gc2-usb provenance notice added 2026-11-22.
 * Copied code; provenance notice added from o0Zz/sys-con (GPL version 3).
 * Upstream: src/controllerlib/IUSBEndpoint.h
 * Revision: c0cd1ff355b4f04ca5cc62963f66572f11344553.
 * Original work remains credited to upstream authors/contributors.
 * Functional adaptations, where present, made during November 2026.
 * See PROVENANCE.md, ../../THIRD_PARTY.md and ../../LICENSE.
 */
#pragma once
#include "Status.h"
#include <cstdint>
#include <cstddef>

namespace controllerlib
{
    class IUSBEndpoint
    {
    public:
        enum Direction : uint8_t
        {
            USB_ENDPOINT_IN = 0x80,
            USB_ENDPOINT_OUT = 0x00,
        };

        struct EndpointDescriptor
        {
            uint8_t bLength;
            uint8_t bDescriptorType;
            uint8_t bEndpointAddress;
            uint8_t bmAttributes;
            uint16_t wMaxPacketSize;
            uint8_t bInterval;
        };

        virtual ~IUSBEndpoint() = default;

        // Open and close the endpoint. if maxPacketSize is not set, it uses wMaxPacketSize from the descriptor.
        virtual Status Open(int maxPacketSize = 0) = 0;
        virtual void Close() = 0;

        virtual Status Write(const uint8_t *inBuffer, size_t bufferSize) = 0;
        virtual Status Read(uint8_t *outBuffer, size_t *bufferSizeInOut, uint64_t aTimeoutUs) = 0;

        virtual IUSBEndpoint::Direction GetDirection() = 0;
        virtual EndpointDescriptor *GetDescriptor() = 0;
    };
} // namespace controllerlib
