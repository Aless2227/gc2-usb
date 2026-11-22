/* gc2-usb provenance notice added 2026-11-22.
 * Copied code; provenance notice added from o0Zz/sys-con (GPL version 3).
 * Upstream: src/platform/SwitchLogger.h
 * Revision: c0cd1ff355b4f04ca5cc62963f66572f11344553.
 * Original work remains credited to upstream authors/contributors.
 * Functional adaptations, where present, made during November 2026.
 * See PROVENANCE.md, ../../THIRD_PARTY.md and ../../LICENSE.
 */
#pragma once

/*
    Declares the subset of syscon::logger that ControllerSwitch needs, without dragging in
    logger.h (and with it IFileManager.h). Log levels come from ILogger.h so that there is
    exactly one definition of them.
*/
#include "ILogger.h"

namespace syscon::logger
{
    void LogTrace(const char *fmt, ...);
    void LogDebug(const char *fmt, ...);
    void LogPerf(const char *fmt, ...);
    void LogInfo(const char *fmt, ...);
    void LogWarning(const char *fmt, ...);
    void LogError(const char *fmt, ...);
    void LogBuffer(controllerlib::LogLevel lvl, const uint8_t *buffer, size_t size);
} // namespace syscon::logger
