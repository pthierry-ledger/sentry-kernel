#include <inttypes.h>
#include <stdbool.h>

/* kernel includes */
#include <sentry/arch/asm-generic/platform.h>
#include <sentry/arch/asm-generic/membarriers.h>
#include <sentry/arch/asm-generic/interrupt.h>
#include <sentry/arch/asm-generic/interrupt.h>
#include <sentry/mm.h>
#include <bsp/drivers/clk/rcc.h>
#include <bsp/drivers/clk/pwr.h>

#if CONFIG_ARCH_ARM_CORTEX_M
#include <sentry/arch/asm-cortex-m/systick.h>
#else
#error "unsupported platform"
#endif
#include <sentry/thread.h>


/*
 * address if the PSP idle stack, as defined in the layout (see m7fw.ld)
 */

#if __GNUC__
#if __clang__
# pragma clang optimize off
#else
__attribute__((optimize("-fno-stack-protector")))
#endif
#endif
__attribute__((noreturn)) void _entrypoint(void)
{
    interrupt_disable();
    pwr_probe();
    rcc_probe();

    interrupt_init();

    platform_init();
    systick_init();

#if 0
// TODO
    /*
     * enable usleep(). Needs to be reexecuted after
     * core frequency upda to upgrade the usleep cycle per USEC_PER_SEC
     * calculation
     */
    perfo_init();

    clock_init();



    /* About CM7 clocking. TBD in IMX8MP (dunno companion mode model)*/
#endif

#if CONFIG_USE_SSP
    /* TODO initialize SSP with random seed */
#endif

#if 0
// TODO
#if defined(CONFIG_USE_ICACHE) && (CONFIG_USE_ICACHE == 1)
    if (icache_is_enabled()) {
       icache_disable();
    }
#endif

#if defined(CONFIG_USE_DCACHE) && (CONFIG_USE_DCACHE == 1)
    if (dcache_is_enabled()) {
       dcache_disable();
    }
#endif
#endif

    /* initialize memory backend controler (e.g. MPU )*/
    mm_initialize();
    mm_configure();

#if 0
#if defined(CONFIG_USE_ICACHE) && (CONFIG_USE_ICACHE == 1)
    icache_enable();
#endif

#if defined(CONFIG_USE_DCACHE) && (CONFIG_USE_DCACHE == 1)
    dcache_enable();
#endif


    // init systick
    set_core_frequency();
    systick_init();
    perfo_early_init();
#endif

    //__platform_spawn_kthread(thread, stack)

    do {

    } while (1);
    __builtin_unreachable();
    /* This part of the function is never reached */
}
