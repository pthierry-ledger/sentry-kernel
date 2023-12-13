// SPDX-FileCopyrightText: 2023 Ledger SAS
// SPDX-License-Identifier: Apache-2.0

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <sentry/managers/task.h>
#include <uapi/handle.h>

kstatus_t mgr_time_init(void);

#ifdef CONFIG_BUILD_TARGET_AUTOTEST
kstatus_t mgr_time_autotest(void);
#endif

/**
 * @brief add a new delayed job to the delay queue, with a delay of delay_ms
 */
kstatus_t mgr_time_delay_add_job(taskh_t job, uint32_t delay_ms);

/**
 * @brief add a new delayed signal to the delay queue, with a delay of delay_ms
 * the kernel will emit the signal toward the target job at end time
 */
kstatus_t mgr_time_delay_add_signal(taskh_t job, uint32_t delay_ms, sigh_t sig);


/**
 * delay ticker, to be called by the systick using JIFFIES_TO_MSEC(1)
 * to calculate back the ticker period
 */
void mgr_time_delay_tick(void);

#if defined(__cplusplus)
}
#endif

#endif/*!TIME_MANAGER_H*/