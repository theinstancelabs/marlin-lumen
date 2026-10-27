/**
 * InstanceLabs: M990 - host-silence watchdog
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "../../../inc/MarlinConfig.h"

#if ENABLED(IL_HOST_WATCHDOG)

#include "../../gcode.h"
#include "../../../feature/instancelabs/host_watchdog.h"

/**
 * M990: Host-silence watchdog
 *
 *   M990          Report: "echo:Host watchdog: off|armed S<ms>[ tripped]"
 *   M990 S<ms>    Arm with a timeout of <ms> milliseconds (IL_HOST_WATCHDOG_MIN_MS..MAX_MS)
 *   M990 S0       Disarm
 *
 * While armed, if no line arrives from the host for <ms> while Marlin is
 * idle: quick-stop, FAN3/FAN2/FAN1/FAN0 off, J (B) driver off,
 * "Error:host watchdog", and the watchdog disarms itself (one-shot).
 * See feature/instancelabs/host_watchdog.h for the exact timer rules.
 */
void GcodeSuite::M990() {
  if (parser.seen('S')) {
    const uint32_t ms = parser.ulongval('S');
    if (ms && !WITHIN(ms, IL_HOST_WATCHDOG_MIN_MS, IL_HOST_WATCHDOG_MAX_MS)) {
      SERIAL_ERROR_MSG("M990 S must be 0 or ", IL_HOST_WATCHDOG_MIN_MS, "..", IL_HOST_WATCHDOG_MAX_MS);
      return;
    }
    host_watchdog.set_timeout(ms);
  }
  host_watchdog.report();
}

#endif // IL_HOST_WATCHDOG
