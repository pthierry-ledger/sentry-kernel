// SPDX-FileCopyrightText: 2023 Ledger SAS
// SPDX-License-Identifier: Apache-2.0

#ifndef DRV_PWR_H
#define DRV_PWR_H

#include <inttypes.h>
#include <stdbool.h>

#include <sentry/ktypes.h>
#include <sentry/arch/asm-cortex-m/buses.h>

#if defined(CONFIG_ARCH_MCU_STM32F439) || defined(CONFIG_ARCH_MCU_STM32F429)
/* Two-bits encoded value on STM32F4(2|3)x */
typedef enum clk_vos_scale {
    POWER_VOS_SCALE_3   = 0x1UL,
    POWER_VOS_SCALE_2   = 0x2UL,
    POWER_VOS_SCALE_1   = 0x3UL,
} clk_vos_scale_t;
/*@
  prediate scale_is_valid(uint8_t s) =
    s == POWER_VOS_SCALE_3 ||
    s == POWER_VOS_SCALE_2 ||
    s == POWER_VOS_SCALE_1;
*/
#else
/* One-bit encoded value on STM32F4(0|1)x */
typedef enum clk_vos_scale {
    POWER_VOS_SCALE_1   = 0x0UL,
    POWER_VOS_SCALE_2   = 0x1UL,
} clk_vos_scale_t;
/*@
  prediate scale_is_valid(uint8_t s) =
    s == POWER_VOS_SCALE_2 ||
    s == POWER_VOS_SCALE_1;
*/
#endif

kstatus_t pwr_probe(void);

kstatus_t pwr_set_voltage_regulator_scaling(uint8_t scale);

#endif/*DRV_PWR_H*/
