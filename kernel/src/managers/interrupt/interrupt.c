// SPDX-FileCopyrightText: 2023 Ledger SAS
// SPDX-License-Identifier: Apache-2.0

#include <uapi/handle.h>
#include <sentry/managers/clock.h>
#include <sentry/ktypes.h>
#include <sentry/arch/asm-generic/interrupt.h>
#include <sentry/arch/asm-generic/thread.h>
#include <sentry/arch/asm-generic/panic.h>
#include <sentry/managers/device.h>
#include <sentry/managers/task.h>

#include <bsp/drivers/clk/rcc.h>
#include <bsp/drivers/clk/pwr.h>
#include <bsp/drivers/flash/flash.h>

stack_frame_t *userisr_handler(stack_frame_t *frame, int IRQn)
{
    devh_t dev;
    taskh_t owner;
    /* get the device owning the interrupt */
    if (unlikely(mgr_device_get_devh_from_interrupt(IRQn, &dev) != K_STATUS_OKAY)) {
        /* interrupt with no known device ???? */
        panic();
    }
    /* get the task owning the device */
    if (unlikely(mgr_task_get_device_owner(dev, &owner) != K_STATUS_OKAY)) {
        /* user interrupt with no owning task ???? */
        panic();
    }
    irqh_t ev = {
        .irqn = IRQn,
        .id = IRQn, /** XXX: is that field has some logic, or being the IRQn only is enough? */
        .family = HANDLE_IRQ,
    };
    /* push the inth event into the task input events queue */
    if (unlikely(mgr_task_push_event(ev, owner) == K_STATUS_OKAY)) {
        /* failed to push IRQ event !!! XXX: what do we do ? */
        panic();
    }
    return frame;
}

kstatus_t mgr_interrupt_init(void)
{
    /** FIXME: implement init part of interrupt manager */
    interrupt_init(); /** still needed again ? */
    return K_STATUS_OKAY;
}

#ifdef CONFIG_BUILD_TARGET_AUTOTEST
kstatus_t mgr_interrupt_autotest(void)
{
    kstatus_t status = K_STATUS_OKAY;
    return status;
}
#endif

/** FIXME: add interrupt manipulation (add user handler, del user handler, etc.) */
