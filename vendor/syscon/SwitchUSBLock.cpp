/* gc2-usb provenance notice added 2026-11-22.
 * Copied code; provenance notice added from o0Zz/sys-con (GPL version 3).
 * Upstream: src/platform/SwitchUSBLock.cpp
 * Revision: c0cd1ff355b4f04ca5cc62963f66572f11344553.
 * Original work remains credited to upstream authors/contributors.
 * Functional adaptations, where present, made during November 2026.
 * See PROVENANCE.md, ../../THIRD_PARTY.md and ../../LICENSE.
 */
#include "SwitchUSBLock.h"
#include <mutex>

static std::recursive_mutex usbMutex;

SwitchUSBLock::SwitchUSBLock(bool scoped) : m_scoped(scoped)
{
    if (m_scoped)
        lock();
}

SwitchUSBLock::~SwitchUSBLock()
{
    if (m_scoped)
        unlock();
}

void SwitchUSBLock::lock()
{
    usbMutex.lock();
}

void SwitchUSBLock::unlock()
{
    usbMutex.unlock();
}