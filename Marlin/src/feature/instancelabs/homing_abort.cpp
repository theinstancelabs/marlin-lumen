/**
 * InstanceLabs: quick-stop during G28 - see homing_abort.h
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "../../inc/MarlinConfig.h"

#include "homing_abort.h"

volatile bool il_homing_active;   // = false
volatile bool il_homing_aborted;  // = false
