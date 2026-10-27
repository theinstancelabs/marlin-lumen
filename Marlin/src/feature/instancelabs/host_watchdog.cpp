/**
 * InstanceLabs host-silence watchdog (M990) - see host_watchdog.h
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "../../inc/MarlinConfig.h"

#if ENABLED(IL_HOST_WATCHDOG)

#include "host_watchdog.h"
#include "outputs_off.h"

#include "../../gcode/gcode.h"
#include "../../gcode/queue.h"
#include "../../module/motion.h"
#include "../../module/planner.h"
#include "../../module/stepper.h"

HostWatchdog host_watchdog;

uint32_t HostWatchdog::timeout_ms; // = 0 (disarmed)
uint32_t HostWatchdog::last_ms;
bool HostWatchdog::tripped;

void HostWatchdog::set_timeout(const uint32_t ms) {
  last_ms = millis();
  timeout_ms = ms;
  if (ms) tripped = false;
}

void HostWatchdog::host_activity() { last_ms = millis(); }

void HostWatchdog::idle() {
  if (!timeout_ms) return;

  const uint32_t ms = millis();

  // A host waiting for "ok" is not silent: hold the timer while a command
  // runs or is waiting in the command queue.
  if (gcode.busy_state != GcodeSuite::NOT_BUSY || queue.has_commands_queued()) {
    last_ms = ms;
    return;
  }

  if (uint32_t(ms - last_ms) >= timeout_ms) trip();
}

void HostWatchdog::trip() {
  // Disarm first: planner.synchronize() below calls idle(), which would
  // otherwise re-enter here.
  timeout_ms = 0;
  tripped = true;

  // Same steps as quickstop_stepper() (M410), but the outputs go off right
  // after the planner is flushed: planner.synchronize() then waits about
  // 1 s (Planner::busy() includes cleaning_buffer_counter = TEMP_TIMER_FREQUENCY).
  planner.quick_stop();               // Drop planned moves and abort the current one (immediate)
  il_outputs_off();                   // FAN3 valve, FAN2 pump, FAN1, FAN0
  #if HAS_J_AXIS
    stepper.disable_axis(J_AXIS);     // Auger (B). Z stays energised so the head does not drop.
  #endif

  SERIAL_ERROR_MSG("host watchdog");

  // Rest of quickstop_stepper(): wait out the abort, then resync position.
  // B may count a few steps sent to the now-disabled driver: re-zero B after a trip.
  planner.synchronize();
  set_current_from_steppers_for_axis(ALL_AXES_ENUM);
  sync_plan_position();
}

void HostWatchdog::report() {
  SERIAL_ECHO_START();
  SERIAL_ECHOPGM("Host watchdog: ");
  if (timeout_ms)
    SERIAL_ECHOPGM("armed S", timeout_ms);
  else
    SERIAL_ECHOPGM("off");
  if (tripped) SERIAL_ECHOPGM(" tripped");
  SERIAL_EOL();
}

#endif // IL_HOST_WATCHDOG
