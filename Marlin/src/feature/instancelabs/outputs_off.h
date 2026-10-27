/**
 * InstanceLabs safety helpers for the LumenPnP REV05 + auger paste dispenser.
 *
 * The four MOSFET outputs are Marlin "fans":
 *   FAN0 (PB8)  left pump        FAN1 (PB9)  left valve
 *   FAN2 (PE5)  right pump (12 V buck, on/off only)
 *   FAN3 (PE6)  right valve (Clippard EV, normally closed -> vents when off)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */
#pragma once

#include "../../inc/MarlinConfigPre.h"

/**
 * Drive every MOSFET output low now, in this order:
 *   FAN3 (right valve) -> FAN2 (right pump) -> FAN1 -> FAN0.
 *
 * Valve first, so the barrel is vented before the pump stops.
 *
 * - Zeroes the commanded speeds (thermalManager.fan_speed[]), so the
 *   planner's fan sync (Planner::check_axes_activity) cannot switch an
 *   output back on. Callers outside kill() must flush the planner
 *   first (blocks carry their own fan speeds).
 * - Forces each pin to a GPIO push-pull LOW (detaches it from its PWM
 *   timer). Register writes only: no allocation, no delays, no
 *   interrupts, so it is safe in kill()/minkill() after cli().
 * - A later M106 re-attaches the pin to its timer
 *   (MarlinHAL::set_pwm_duty calls pinmap_pinout every time), so the
 *   outputs keep working after a non-fatal call.
 */
void il_outputs_off();
