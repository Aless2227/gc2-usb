/* gc2-usb provenance notice added 2026-11-22.
 * Copied code; provenance notice added from o0Zz/sys-con (GPL version 3).
 * Upstream: src/platform/SwitchUSBLock.h
 * Revision: c0cd1ff355b4f04ca5cc62963f66572f11344553.
 * Original work remains credited to upstream authors/contributors.
 * Functional adaptations, where present, made during November 2026.
 * See PROVENANCE.md, ../../THIRD_PARTY.md and ../../LICENSE.
 */
#pragma once

/*
 * Global Lock for USBHS API Calls
 *
 * This lock is required for the entire application when using any usbHsXXXX API.
 *
 * Reason:
 * The usbHsXXXX APIs cannot be called in parallel for an unknown reason. They must
 * be executed sequentially to avoid unexpected behavior (e.g. Initialization failure, ...).
 *
 * Usage:
 * Always ensure you acquire this lock before calling any usbHsXXXX API.
 */

class SwitchUSBLock
{
public:
    SwitchUSBLock(bool scoped = true);
    ~SwitchUSBLock();

    void lock();
    void unlock();

private:
    bool m_scoped;
};