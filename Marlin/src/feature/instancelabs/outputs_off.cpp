/**
 * InstanceLabs safety helpers - see outputs_off.h
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "../../inc/MarlinConfig.h"

#include "outputs_off.h"

#if HAS_FAN
  #include "../../module/temperature.h"
#endif

// Write LOW first, so switching the pin from timer AF to GPIO output
// cannot produce a high glitch, then switch the mode, then write again.
#define _IL_PIN_LOW(P) do{ WRITE(P, LOW); OUT_WRITE(P, LOW); }while(0)

#define _IL_FAN_OFF(N) do{ thermalManager.fan_speed[N] = 0; _IL_PIN_LOW(FAN##N##_PIN); }while(0)

void il_right_outputs_off() {
  #if HAS_FAN3
    _IL_FAN_OFF(3);   // right valve first: vent the barrel
  #endif
  #if HAS_FAN2
    _IL_FAN_OFF(2);   // right pump
  #endif
}

void il_outputs_off() {
  #if HAS_FAN3
    _IL_FAN_OFF(3);   // right valve first: vent the barrel
  #endif
  #if HAS_FAN2
    _IL_FAN_OFF(2);   // right pump
  #endif
  #if HAS_FAN1
    _IL_FAN_OFF(1);   // left valve
  #endif
  #if HAS_FAN0
    _IL_FAN_OFF(0);   // left pump
  #endif
}
