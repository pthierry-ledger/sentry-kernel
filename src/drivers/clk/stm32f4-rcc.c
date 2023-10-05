// SPDX-FileCopyrightText: 2023 Ledger SAS
// SPDX-License-Identifier: Apache-2.0

/**
 * \file STM32F3xx and F4xx PLL & clock driver (see ST RM0090 datasheet)
 */
#include <assert.h>

#include <sentry/arch/asm-cortex-m/soc.h>
#include <sentry/arch/asm-cortex-m/layout.h>
#include <sentry/arch/asm-cortex-m/core.h>
#include <sentry/arch/asm-cortex-m/buses.h>
#include <sentry/arch/asm-generic/membarriers.h>
#include <sentry/io.h>
#include <sentry/bits.h>
#include <sentry/ktypes.h>

/* local includes, only manipulated by the driver itself */
#include <bsp/drivers/clk/clk.h>

/* RCC generated header for current platform */
#include "rcc_defs.h"
#include "pwr_defs.h"


#define RCC_OSCILLATOR_STABLE	(0)

#define HSE_STARTUP_TIMEOUT	(0x0500UL)
#define HSI_STARTUP_TIMEOUT	(0x0500UL)
#define PLL_STARTUP_TIMEOUT	(0x0500UL)


#define PROD_CLOCK_APB1  42000000 // Hz
#define PROD_CLOCK_APB2  84000000 // Hz

#define PROD_CORE_FREQUENCY 168000 // KHz


//#include "soc-flash.h"
//#include "m4-cpu.h"

uint64_t clk_get_core_frequency(void)
{
    return (PROD_CORE_FREQUENCY*1000);
}

/*
 * TODO: some of the bellowing code should be M4 generic. Yet, check if all
 * these registers are M4 generic or STM32F4 core specific
 */
void clk_reset(void)
{
    size_t reg;
    /* Reset the RCC clock configuration to the default reset state */
    /* Set HSION bit */
    reg = ioread32(RCC_BASE_ADDR + RCC_CR_REG);
    reg |= 0x1UL;
    iowrite32(RCC_BASE_ADDR + RCC_CR_REG, reg);

    /* Reset CFGR register */
    iowrite32(RCC_BASE_ADDR + RCC_CFGR_REG, 0x0UL);

    /* Reset HSEON, CSSON and PLLON bits */
    reg = ioread32(RCC_BASE_ADDR + RCC_CR_REG);
    reg &= ~ (RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON);
    iowrite32(RCC_BASE_ADDR + RCC_CR_REG, reg);

    /* Reset PLLCFGR register, 0x24.00.30.10 being the reset value */
    iowrite32(RCC_PLLCFGR_REG, 0x24003010UL);

    /* Reset HSEBYP bit */
    reg = ioread32(RCC_BASE_ADDR + RCC_CR_REG);
    reg &= ~(RCC_CR_HSEBYP);
    iowrite32(RCC_BASE_ADDR + RCC_CR_REG, reg);

    /* Reset all interrupts */
    iowrite32(RCC_BASE_ADDR + RCC_CIR_REG, 0x0UL);
}

kstatus_t clk_set_system_clk(bool enable_hse, bool enable_pll)
{
    kstatus_t status = K_STATUS_OKAY;
    bool timeouted = false;
    bool not_ready = true;
    union _reg {
        uint32_t raw;
        rcc_cfgr_t cfgr;
        rcc_pllcfgr_t pllcfgr;
    };
    union _reg reg;
    static_assert(sizeof(reg) == sizeof(uint32_t), "invalid register type length");

    uint32_t StartUpCounter = 0;

    /*
     * PLL (clocked by HSE/HSI) used as System clock source
     */

    if (enable_hse) {
        /* Enable HSE */
        reg.raw = ioread32(RCC_BASE_ADDR + RCC_CR_REG);
        reg.raw |= RCC_CR_HSEON;
        iowrite32(RCC_BASE_ADDR + RCC_CR_REG, reg.raw);
        do {
            reg.raw = ioread32(RCC_BASE_ADDR + RCC_CR_REG);
            reg.raw &= RCC_CR_HSERDY;
            StartUpCounter++;
        } while ((reg.raw == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));
    } else {
        /* Enable HSI */
        reg.raw = ioread32(RCC_BASE_ADDR + RCC_CR_REG);
        reg.raw |= RCC_CR_HSION;
        iowrite32(RCC_BASE_ADDR + RCC_CR_REG, reg.raw);
        do {
            reg.raw = ioread32(RCC_BASE_ADDR + RCC_CR_REG);
            reg.raw &= RCC_CR_HSIRDY;
            StartUpCounter++;
        } while ((reg.raw == 0) && (StartUpCounter != HSI_STARTUP_TIMEOUT));
    }

    if (reg.raw == 0) {
        /* HSE or HSI oscillator is not stable at the end of the timeout window,
         * we should do something
         */
        status = K_ERROR_NOTREADY;
        goto err;
    }

    /* Enable high performance mode at bootup, System frequency up to 168 MHz */
    reg.raw = ioread32(RCC_BASE_ADDR + RCC_APB1ENR_REG);
    reg.raw |= RCC_APB1ENR_PWREN;
    iowrite32(RCC_BASE_ADDR + RCC_APB1ENR_REG, reg.raw);
    /*
     * This bit controls the main internal voltage regulator output
     * voltage to achieve a trade-off between performance and power
     * consumption when the device does not operate at the maximum
     * frequency. (DocID018909 Rev 15 - page 141)
     * PWR_CR_VOS = 1 => Scale 1 mode (default value at reset)
     */
    reg.raw = ioread32(PWR_BASE_ADDR + PWR_CR_REG);
    reg.raw |= PWR_CR_VOS_MASK;
    iowrite32(RCC_BASE_ADDR + PWR_CR_REG, reg.raw);


    /* Set clock dividers */

    reg.raw = ioread32(RCC_BASE_ADDR + RCC_CFGR_REG);
    reg.cfgr.hpre = 0x0UL; /* not divide */
    reg.cfgr.ppre1 = 0x5UL; /* div 4 */
    reg.cfgr.ppre2 = 0x4UL; /* div 2 */
    iowrite32(RCC_BASE_ADDR + RCC_CFGR_REG, reg.raw);

    reg.raw = 0;
    if (enable_pll) {
        /* Configure the main PLL */
        /**
         * FIXME: this is the configuration valuses used for Wookey project that
         * are hardcoded, allowing correct (but maybe not optimal) calculation for
         * for various AHB/APB devices. To be checked and properly calculated
         */
        reg.pllcfgr.pllm4 = 1; /* PROD_PLL_M = 16 */
        reg.pllcfgr.pllp1 = 1; /* PROD_PLL_P = 2 */
        reg.pllcfgr.pllq0 = 1; /* PROD_PLL_Q = 7*/
        reg.pllcfgr.pllq1 = 1;
        reg.pllcfgr.pllq2 = 1;
        if (enable_hse) {
            reg.pllcfgr.pllsrc = 1;
        }
        iowrite32(RCC_BASE_ADDR + RCC_PLLCFGR_REG, reg.raw);

        reg.raw = ioread32(RCC_BASE_ADDR + RCC_CR_REG);
        reg.raw |= RCC_CR_PLLON;
        /* Enable the main PLL */
        iowrite32(RCC_BASE_ADDR + RCC_CR_REG, reg.raw);
        /* Wait till the main PLL is ready */
        StartUpCounter = 0;
        do {
            reg.raw = (ioread32(RCC_BASE_ADDR + RCC_CR_REG) & RCC_CR_PLLRDY);
            if (reg.raw != 0) {
                break;
            }
            StartUpCounter++;
        } while (StartUpCounter < PLL_STARTUP_TIMEOUT);
        /* check timeout */
        if (StartUpCounter == PLL_STARTUP_TIMEOUT) {
            status = K_ERROR_NOTREADY;
            goto err;
        }


        /* Select the main PLL as system clock source */
        reg.raw = ioread32(RCC_BASE_ADDR + RCC_CFGR_REG);
        reg.cfgr.sw0 = 0UL;
        reg.cfgr.sw1 = 1UL; /* 0b10 -> PLL as system clock */
        iowrite32(RCC_BASE_ADDR + RCC_CFGR_REG, reg.raw);

        /* Wait till the main PLL is used as system clock source */
        StartUpCounter = 0;
        do {
            reg.raw = ioread32(RCC_BASE_ADDR + RCC_CFGR_REG);
            if ((reg.raw & (RCC_CFGR_SWS0 | RCC_CFGR_SWS1)) == RCC_CFGR_SWS0) {
                break;
            }
            StartUpCounter++;
        } while (StartUpCounter < PLL_STARTUP_TIMEOUT);
        /* check timeout */
        if (StartUpCounter == PLL_STARTUP_TIMEOUT) {
            status = K_ERROR_NOTREADY;
            goto err;
        }
    }

#if 0
    /* Configure Flash prefetch, Instruction cache, Data cache and wait state */
    write_reg_value(FLASH_ACR, FLASH_ACR_ICEN
                    | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_5WS);
#endif

err:
    return status;

    //panic();
}

__STATIC_INLINE size_t rcc_get_register(bus_id_t busid, rcc_opts_t flags)
{
    size_t reg_base;
    if (flags & RCC_LPCONFIG) {
        reg_base = RCC_BASE_ADDR + RCC_AHB1LPENR_REG;
    } else {
        reg_base = RCC_BASE_ADDR + RCC_AHB1ENR_REG;
    }
    /*
     * Here, instead of a switch/case, we calculate the offset using the
     * fact that, for both nominal and low power enable registers:
     * 1. AHBxENR are concatenated
     * 2. APBxENR are concatenated
     * 3. a space may exist between AHBx & APBx for future AHB buses
     * We used basic calculation (additions and increment), the compiler
     * can highly optimize, avoiding branches and tests of a huge
     * switch/case
     */
    if (busid < BUS_APB1) {
        /* AHB buses registers are concatenated in memory */
        reg_base += (busid*sizeof(uint32_t));
    } else {
        /*
          ABP regs may start with empty slots for future buses.
          Empty slotting is the same for nominal and low power
        */
        /* 1. increment to APB1 reg base address */
        reg_base += (RCC_APB1ENR_REG - RCC_AHB1ENR_REG);
        /* 2. increment to APBx busid, starting at APB1 */
        reg_base += ((busid - BUS_APB1)*sizeof(uint32_t));
    }
    return reg_base;
}

/**
 * @brief enable given clock identifier for the given bus identifier
 *
 *
 * @param busid bus identifier, generated from SVD file, see buses.h
 * @param clk_msk clock mask, which correspond to the mask to apply on the
 *    bus enable register so that the corresponding device is enabled. This is
 *    a 32bit value that is directly used. On STM32, this value mostly hold a single
 *    bit set to 1 (except for ETH).
 *
 *
 * @return K_STATUS_OKAY of the clock is properly enabled, or an error
 *  status otherwise
 */
kstatus_t rcc_enable(bus_id_t busid, uint32_t clk_msk, rcc_opts_t flags)
{
    kstatus_t status = K_STATUS_OKAY;
    size_t reg;
    size_t reg_base = rcc_get_register(busid, flags);

    reg = ioread32(reg_base);
    reg |= clk_msk;
    iowrite32(reg_base, reg);
    // Stall the pipeline to work around erratum 2.1.13 (DM00037591)
    arch_data_sync_barrier();

    return status;
}

/**
 * @brief disable given clock identifier for the given bus identifier
 *
 *
 * @param busid bus identifier, generated from SVD file, see buses.h
 * @param clk_msk clock mask, which correspond to the mask to apply on the
 *    bus enable register so that the corresponding device is enabled. This is
 *    a 32bit value that is directly used. On STM32, this value mostly hold a single
 *    bit set to 1 (except for ETH).
 *
 * @return K_STATUS_OKAY of the clock is properly disabled, or an error
 *  status otherwise
 */
kstatus_t rcc_disable(bus_id_t busid, uint32_t clk_msk, rcc_opts_t flags)
{
    kstatus_t status = K_STATUS_OKAY;
    size_t reg;
    size_t reg_base = rcc_get_register(busid, flags);

    reg = ioread32(reg_base);
    reg &= ~clk_msk;
    iowrite32(reg_base, reg);

    return status;
}