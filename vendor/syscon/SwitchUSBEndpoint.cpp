/* gc2-usb provenance notice added 2026-11-22.
 * Altered adaptation from o0Zz/sys-con (GPL version 3).
 * Upstream: src/platform/SwitchUSBEndpoint.cpp
 * Revision: c0cd1ff355b4f04ca5cc62963f66572f11344553.
 * Original work remains credited to upstream authors/contributors.
 * Functional adaptations, where present, made during November 2026.
 * See PROVENANCE.md, ../../THIRD_PARTY.md and ../../LICENSE.
 */
#include "SwitchUSBEndpoint.h"
#include "SwitchUSBLock.h"
#include "SwitchLogger.h"
#include <algorithm>
#include <cstring>

using namespace controllerlib;

SwitchUSBEndpoint::SwitchUSBEndpoint(UsbHsClientIfSession &if_session, usb_endpoint_descriptor &desc)
    : m_ifSession(&if_session),
      m_descriptor(&desc)
{
}

SwitchUSBEndpoint::~SwitchUSBEndpoint()
{
    Close();
}

Status SwitchUSBEndpoint::Open(int maxPacketSize)
{
    maxPacketSize = maxPacketSize != 0 ? maxPacketSize : m_descriptor->wMaxPacketSize;

    ::syscon::logger::LogDebug("SwitchUSBEndpoint[0x%02X] Opening (Pkt size: %d)...", m_descriptor->bEndpointAddress, maxPacketSize);

    Result rc;
    {
        SwitchUSBLock usbLock;
        rc = usbHsIfOpenUsbEp(m_ifSession, &m_epSession, 1, maxPacketSize, m_descriptor);
    }

    if (R_FAILED(rc))
    {
        ::syscon::logger::LogError("SwitchUSBEndpoint[0x%02X] Failed to open: 0x%08X (Module: 0x%X, Desc: 0x%X)", m_descriptor->bEndpointAddress, rc, R_MODULE(rc), R_DESCRIPTION(rc));
        return Status::UsbEndpointOpen;
    }

    m_open = true;
    ::syscon::logger::LogDebug("SwitchUSBEndpoint[0x%02X] Successfully opened !", m_descriptor->bEndpointAddress);

    if (GetDirection() == USB_ENDPOINT_OUT)
        return Status::Success;

    m_readSize = std::min<u32>(maxPacketSize, sizeof(m_usb_buffer_in));

    /* usb:hs must already hold a transfer on an interrupt IN endpoint when the device first
       talks on it, otherwise its first report (an Xbox 360 LED ack, the GC adapter's first
       frame, a pad already streaming at enumeration) wedges the endpoint for 30-60 s. */
    return PostRead();
}

Status SwitchUSBEndpoint::PostRead()
{
    Result rc;
    {
        SwitchUSBLock usbLock;
        rc = usbHsEpPostBufferAsync(&m_epSession, m_usb_buffer_in, m_readSize, 0, &m_xferIdRead);
    }

    if (R_FAILED(rc))
    {
        ::syscon::logger::LogError("SwitchUSBEndpoint[0x%02X] Read post failed: 0x%08X", m_descriptor->bEndpointAddress, rc);
        return Status::ReadFailed;
    }

    m_pending = true;
    return Status::Success;
}

void SwitchUSBEndpoint::Close()
{
    if (!m_open) return;
    m_open = false;
    m_pending = false;
    m_xferIdRead = 0;
    SwitchUSBLock usbLock;

    usbHsEpClose(&m_epSession);
}

Status SwitchUSBEndpoint::Write(const uint8_t *inBuffer, size_t bufferSize)
{
    if (!m_open || GetDirection() != USB_ENDPOINT_OUT || !inBuffer ||
        bufferSize > sizeof(m_usb_buffer_out)) return Status::InvalidArgument;
    if (GetDirection() == USB_ENDPOINT_IN)
        ::syscon::logger::LogError("SwitchUSBEndpoint[0x%02X] Trying to write an INPUT endpoint!", m_descriptor->bEndpointAddress);

    ::syscon::logger::LogTrace("SwitchUSBEndpoint[0x%02X] Write %d bytes", m_descriptor->bEndpointAddress, bufferSize);
    ::syscon::logger::LogBuffer(LogLevel::Trace, inBuffer, bufferSize);

    Result rc;
    {
        SwitchUSBLock usbLock;
        u32 transferredSize = 0;

        memcpy(m_usb_buffer_out, inBuffer, bufferSize);
        // gc2-usb: bounded asynchronous write; never block forever on a lost device.
        u32 id = 0, count = 0;
        UsbHsXferReport report{};
        rc = usbHsEpPostBufferAsync(&m_epSession, m_usb_buffer_out, bufferSize, 0, &id);
        if (R_SUCCEEDED(rc)) rc = eventWait(usbHsEpGetXferEvent(&m_epSession), 1000000000);
        if (R_SUCCEEDED(rc)) {
            eventClear(usbHsEpGetXferEvent(&m_epSession));
            rc = usbHsEpGetXferReport(&m_epSession, &report, 1, &count);
            if (R_SUCCEEDED(rc) && count == 1 && report.xferId == id) {
                rc = report.res;
                transferredSize = report.transferredSize;
            } else rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
        }
        if (R_SUCCEEDED(rc) && transferredSize != bufferSize)
            rc = MAKERESULT(Module_Libnx, LibnxError_BadInput);
    }

    if (R_FAILED(rc))
    {
        ::syscon::logger::LogError("SwitchUSBEndpoint[0x%02X] Write failed: 0x%08X (Module: 0x%X, Desc: 0x%X)", m_descriptor->bEndpointAddress, rc, R_MODULE(rc), R_DESCRIPTION(rc));
        return Status::WriteFailed;
    }

    /* bInterval pacing belongs to this endpoint: sleeping under the global USB lock stalls every
       other controller's polling thread for the whole interval. */
    if ((m_descriptor->bmAttributes & 3) == 3)
        svcSleepThread(m_descriptor->bInterval * 1000000);

    return Status::Success;
}

Status SwitchUSBEndpoint::Read(uint8_t *outBuffer, size_t *bufferSizeInOut, u64 aTimeoutUs)
{
    if (!m_open || GetDirection() != USB_ENDPOINT_IN || !outBuffer || !bufferSizeInOut)
        return Status::InvalidArgument;
    if (GetDirection() == USB_ENDPOINT_OUT)
        ::syscon::logger::LogError("SwitchUSBEndpoint[0x%02X] Trying to read an OUTPUT endpoint!", m_descriptor->bEndpointAddress);

    u32 count = 0;
    UsbHsXferReport report;
    u32 tmpXcferId = 0;
    Result rc;

    if (!m_pending)
    {
        Status postStatus = PostRead();
        if (postStatus != Status::Success)
            return postStatus;
    }

    /* eventWait/eventClear are syscalls on the endpoint's own event, not usbHs IPC. Holding the
       global USB lock across the wait would pin it for the whole polling timeout and starve every
       other controller and the discovery thread. */
    if (R_FAILED(eventWait(usbHsEpGetXferEvent(&m_epSession), aTimeoutUs * 1000)))
        return Status::Timeout;

    eventClear(usbHsEpGetXferEvent(&m_epSession));

    tmpXcferId = m_xferIdRead;
    m_xferIdRead = 0;
    m_pending = false;

    memset(&report, 0, sizeof(report));
    size_t copySize = 0;
    {
        SwitchUSBLock usbLock;
        rc = usbHsEpGetXferReport(&m_epSession, &report, 1, &count);
        if (R_SUCCEEDED(rc) && (count > 0) && (tmpXcferId == report.xferId) && R_SUCCEEDED(report.res))
        {
            if (report.transferredSize > *bufferSizeInOut || report.transferredSize > sizeof(m_usb_buffer_in))
                return Status::UnexpectedData;
            copySize = report.transferredSize;
            memcpy(outBuffer, m_usb_buffer_in, copySize);
        }
    }

    if (R_FAILED(rc))
    {
        ::syscon::logger::LogError("SwitchUSBEndpoint[0x%02X] Read report failed: 0x%08X", m_descriptor->bEndpointAddress, rc);
        return Status::ReadFailed;
    }

    if ((count <= 0) || (tmpXcferId != report.xferId))
    {
        ::syscon::logger::LogTrace("SwitchUSBEndpoint[0x%02X] Read no report (Count: %d, xferId %d/%d)", m_descriptor->bEndpointAddress, count, tmpXcferId, report.xferId);
        return Status::NoDataAvailable;
    }

    if (R_FAILED(report.res))
    {
        ::syscon::logger::LogError("SwitchUSBEndpoint[0x%02X] Read transfer failed: 0x%08X", m_descriptor->bEndpointAddress, report.res);
        return Status::ReadFailed;
    }

    *bufferSizeInOut = copySize;

    if (copySize == 0)
        return Status::NoDataAvailable;

    ::syscon::logger::LogTrace("SwitchUSBEndpoint[0x%02X] Read %d bytes", m_descriptor->bEndpointAddress, *bufferSizeInOut);
    ::syscon::logger::LogBuffer(LogLevel::Trace, outBuffer, *bufferSizeInOut);

    return Status::Success;
}

IUSBEndpoint::Direction SwitchUSBEndpoint::GetDirection()
{
    return ((m_descriptor->bEndpointAddress & USB_ENDPOINT_IN) ? USB_ENDPOINT_IN : USB_ENDPOINT_OUT);
}

IUSBEndpoint::EndpointDescriptor *SwitchUSBEndpoint::GetDescriptor()
{
    return reinterpret_cast<IUSBEndpoint::EndpointDescriptor *>(m_descriptor);
}
