/**
 * InstanceLabs: M997 -> STM32F4 system-memory DFU bootloader.
 *
 * Lets a host put the LumenPnP REV05 board into USB DFU mode
 * (0483:df11) without pressing BOOT+RESET.
 *
 * How it works (two stages, so the bootloader always starts from a real
 * hardware reset state):
 *
 *  1. flashFirmware() (called by M997; this strong definition replaces
 *     the WEAK one in HAL/STM32/HAL.cpp, which only rebooted):
 *       planner.quick_stop(), il_outputs_off() (FAN3, FAN2, FAN1, FAN0),
 *       disable all steppers, report, let USB drain, write a two-word
 *       magic into the top of the CCM data RAM, NVIC_SystemReset().
 *
 *  2. il_dfu_preinit() runs from .preinit_array, i.e. inside
 *     __libc_init_array, after Reset_Handler has copied .data, zeroed .bss
 *     and run SystemInit (FPU access + VTOR only), and before every C++
 *     constructor (STM32duino premain() -> init() -> HAL_Init, clock tree,
 *     SysTick, USB) and before main(). At that point the chip is exactly as
 *     the reset left it: HSI clock, no PLL, peripheral clocks at reset
 *     values, no interrupt enabled in the NVIC, SysTick off, and the IWDG
 *     stopped (a system reset stops the software-started IWDG).
 *     If, and only if, the magic is present AND the reset was a software
 *     reset (RCC_CSR.SFTRSTF), it clears the magic and the reset flags,
 *     maps system memory at 0x00000000 (SYSCFG_MEMRMP = 01, what BOOT0=1
 *     does), points VTOR at 0x1FFF0000, loads MSP from 0x1FFF0000 and
 *     branches to the reset vector at 0x1FFF0004 (AN2606, STM32F40x/41x).
 *
 * Why not jump directly from the running firmware: Marlin starts the IWDG
 * (USE_WATCHDOG), which cannot be stopped by software and would reset the
 * chip out of DFU a few seconds later; and every running peripheral (USB,
 * timers, DMA, PLL) would have to be torn down by hand. A reset does all
 * of that in hardware.
 *
 * Failure modes: if the magic is absent or the reset was not a software
 * reset, the hook returns and the firmware boots normally. The magic is
 * cleared before the jump, so leaving DFU (dfu-util ":leave" or RESET)
 * boots the application; there is no boot loop. CCM RAM is not retained
 * across power loss, and its power-on content would also need SFTRSTF.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "../../inc/MarlinConfig.h"

#if ENABLED(IL_M997_DFU)

#if !defined(HAL_STM32) || !defined(STM32F4xx)
  #error "IL_M997_DFU supports STM32F4 (HAL/STM32) only."
#endif
#if DISABLED(PLATFORM_M997_SUPPORT)
  #error "IL_M997_DFU requires PLATFORM_M997_SUPPORT (HAL/STM32/HAL.h)."
#endif

#include "outputs_off.h"
#include "../../module/planner.h"
#include "../../module/stepper.h"

// Last 8 bytes of the 64 KB CCM data RAM (0x10000000-0x1000FFFF). Nothing
// in this link uses CCM (.ccmram is empty, see firmware.map); the startup
// code neither copies nor clears it, and SRAM keeps its content across a
// system reset.
#define IL_DFU_FLAG_ADDR  0x1000FFF8UL
#define IL_DFU_MAGIC      0xDF00B007UL
#define IL_SYSMEM_BASE    0x1FFF0000UL     // STM32F40x/41x system memory (AN2606)

#define IL_DFU_FLAG ((volatile uint32_t *)IL_DFU_FLAG_ADDR)

extern "C" void il_dfu_preinit() {
  if (IL_DFU_FLAG[0] != IL_DFU_MAGIC || IL_DFU_FLAG[1] != ~IL_DFU_MAGIC) return;
  IL_DFU_FLAG[0] = 0;                               // One-shot
  IL_DFU_FLAG[1] = 0;
  if (!(RCC->CSR & RCC_CSR_SFTRSTF)) return;        // Only after our own NVIC_SystemReset()
  RCC->CSR |= RCC_CSR_RMVF;                         // Clear reset flags so the next app boot reports correctly

  RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;             // SYSCFG clock (for MEMRMP)
  (void)RCC->APB2ENR;
  SYSCFG->MEMRMP = SYSCFG_MEMRMP_MEM_MODE_0;        // 0x00000000 -> system memory, as with BOOT0 = 1
  SCB->VTOR = IL_SYSMEM_BASE;

  const uint32_t sp = ((volatile uint32_t *)IL_SYSMEM_BASE)[0],
                 pc = ((volatile uint32_t *)IL_SYSMEM_BASE)[1];
  __DSB();
  __ISB();
  // Set MSP and branch in one asm block, so nothing touches the stack in between.
  __asm volatile ("msr msp, %0 \n\t"
                  "bx  %1      \n\t"
                  : : "r" (sp), "r" (pc) : "memory");
  for (;;) { /* not reached */ }
}

// Run il_dfu_preinit() before any constructor (see the stage 2 notes above).
__attribute__((used, section(".preinit_array")))
static void (* const il_dfu_preinit_entry)(void) = il_dfu_preinit;

void flashFirmware(const int16_t) {
  planner.quick_stop();                             // Drop planned moves, abort the current one
  il_outputs_off();                                 // FAN3 valve, FAN2 pump, FAN1, FAN0
  stepper.disable_all_steppers();                   // Same end state as any reset (drivers' ENN pull-ups)

  SERIAL_ECHO_MSG("M997: entering STM32 DFU bootloader (0483:df11)");
  for (uint8_t i = 0; i < 20; ++i) {                // ~100 ms for USB CDC to send the message
    hal.watchdog_refresh();
    delay(5);
  }

  __disable_irq();
  IL_DFU_FLAG[0] = IL_DFU_MAGIC;
  IL_DFU_FLAG[1] = ~IL_DFU_MAGIC;
  __DSB();
  NVIC_SystemReset();
}

#endif // IL_M997_DFU
