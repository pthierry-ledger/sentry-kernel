// SPDX-FileCopyrightText: 2023 Ledger SAS
// SPDX-License-Identifier: Apache-2.0

#ifndef SENTRY_MANAGERS_DEVICE_H
#define SENTRY_MANAGERS_DEVICE_H

#include <inttypes.h>
#include <uapi/device.h>
#include <uapi/capability.h>
#include <sentry/ktypes.h>

typedef struct device {
    devinfo_t           devinfo;      /**< device info (info shared with userspace) */
    sentry_capability_t capability;   /**< device associated capability */
    uint32_t            clk_id;       /**< clock identifier, as defined in dts */
    uint32_t            bus_id;       /**< bus identifier, as defined in dts */
    secure_bool_t       kernel_owned; /**< is device owned by kernel ? */
} device_t;

kstatus_t mgr_device_init(void);

kstatus_t mgr_device_watchdog(void);

secure_bool_t mgr_device_exists(devh_t d);

kstatus_t mgr_device_get_info(devh_t, const devinfo_t **devinfo);

secure_bool_t mgr_device_is_kernel(devh_t d);

#ifdef CONFIG_BUILD_TARGET_AUTOTEST
kstatus_t mgr_device_autotest(void);
#endif

kstatus_t mgr_device_get_devh_from_interrupt(uint8_t IRQn, devh_t *devh);

kstatus_t mgr_device_get_devinfo_from_interrupt(uint8_t IRQn, const devinfo_t **devinfo);

#endif/*SENTRY_MANAGERS_DEVICE_H*/
