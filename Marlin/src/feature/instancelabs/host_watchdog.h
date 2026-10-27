/**
 * InstanceLabs host-silence watchdog (M990).
 *
 * If the host stops talking while the watchdog is armed, the firmware
 * makes the dispenser safe on its own:
 *   planner quick-stop -> FAN3, FAN2, FAN1, FAN0 off -> J (B, auger) driver off
 *   -> "Error:host watchdog" -> wait out the abort and resync position (as M410)
 *
 * Default: disarmed, so stock host behaviour is unchanged.
 *
 * Timer rules
 *  - Every non-empty line received from a host serial port restarts the timer
 *    (GCodeQueue::get_serial_commands).
 *  - The timer is also held while Marlin is executing a command
 *    (gcode.busy_state != NOT_BUSY, e.g. M400, G4, G28) or still has commands
 *    queued: a host waiting for "ok" is not silent. Consequence: the worst-case
 *    reaction time is (remaining run time of the command in progress) + S.
 *  - Moves already in the planner do NOT hold the timer. That is the case the
 *    watchdog exists for: a queued auger move that keeps running after the host
 *    has gone.
 *  - One-shot: a trip disarms the watchdog and sets a "tripped" flag that
 *    M990 reports until the host arms it again.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#pragma once

#include "../../inc/MarlinConfigPre.h"

#if ENABLED(IL_HOST_WATCHDOG)

#if DISABLED(HOST_KEEPALIVE_FEATURE)
  #error "IL_HOST_WATCHDOG requires HOST_KEEPALIVE_FEATURE (it uses gcode.busy_state)."
#endif

#ifndef IL_HOST_WATCHDOG_MIN_MS
  #define IL_HOST_WATCHDOG_MIN_MS   100UL       // Shorter values would trip between normal host lines
#endif
#ifndef IL_HOST_WATCHDOG_MAX_MS
  #define IL_HOST_WATCHDOG_MAX_MS   3600000UL   // 1 hour
#endif

#include <stdint.h>

class HostWatchdog {
public:
  static uint32_t timeout_ms;   // 0 = disarmed
  static uint32_t last_ms;      // millis() of the last host activity
  static bool tripped;          // Set by a trip, cleared by arming

  static bool armed() { return timeout_ms != 0; }

  // Arm with a timeout (ms), or disarm with 0. Arming clears 'tripped'.
  static void set_timeout(const uint32_t ms);

  // A line arrived from the host (main context only).
  static void host_activity();

  // Called from manage_inactivity(), i.e. on every idle() pass.
  static void idle();

  // Make safe now. Public so it can be exercised by a test build.
  static void trip();

  static void report();
};

extern HostWatchdog host_watchdog;

#endif // IL_HOST_WATCHDOG
