/**
 * InstanceLabs: a quick-stop (M410, host watchdog trip) during G28 ends the
 * homing cleanly instead of killing the controller.
 *
 * Stock Marlin: M410 during a homing move aborts the move short of the
 * endstop, validate_homing_move() then calls kill("Homing Failed"), and the
 * MCU halts with USB dead until a reset. OpenPnP's Stop sends M410 first
 * (DISABLE_COMMAND), so Stop pressed while homing needed a board reset.
 *
 * With this: while G28 runs, a quick-stop sets il_homing_aborted. Every
 * following homing step is skipped (no move is planned, no endstop check,
 * no axis is marked homed), G28 reports "echo:G28 aborted by quick-stop"
 * and leaves all axes unhomed. Outside G28 the flag stays clear.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#pragma once

#include "../../inc/MarlinConfigPre.h"

extern volatile bool il_homing_active;   // G28 is running
extern volatile bool il_homing_aborted;  // a quick-stop arrived during this G28

// Called by every quick-stop path (quickstop_stepper(), host watchdog trip).
inline void il_note_quickstop() { if (il_homing_active) il_homing_aborted = true; }
