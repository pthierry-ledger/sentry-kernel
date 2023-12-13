// SPDX-FileCopyrightText: 2023 Ledger SAS
// SPDX-License-Identifier: Apache-2.0

/**
 * @file Sentry task manager init automaton functions
 */
#include <inttypes.h>
#include <string.h>
#include <sentry/thread.h>
#include <sentry/managers/task.h>
#include "task_core.h"
#include "task_init.h"
#include "task_autotest.h"

/**
 * ldscript provided
 */
extern size_t _autotest_svcexchange;
extern size_t _sautotest;
extern size_t _eautotest;

static task_meta_t autotest_meta;

void task_autotest_init(void)
{
    memset((void*)&autotest_meta, 0x0, sizeof(task_meta_t));
    autotest_meta.handle.rerun = 0;
    autotest_meta.handle.id = SCHED_AUTOTEST_TASK_LABEL;
    autotest_meta.handle.family = HANDLE_TASKID;
    autotest_meta.quantum = 10;
    autotest_meta.priority = 1;
    autotest_meta.magic = CONFIG_TASK_MAGIC_VALUE;
    autotest_meta.flags.start_mode = JOB_FLAG_START_AUTO;
    autotest_meta.flags.exit_mode = JOB_FLAG_EXIT_RESET;
    autotest_meta.s_text = (size_t)&_sautotest;
    autotest_meta.text_size = ((size_t)&_eautotest - (size_t)&_sautotest);
    autotest_meta.entrypoint_offset = 0x1UL;
    autotest_meta.finalize_offset = 0x0UL; /* TBD for idle */
    autotest_meta.rodata_size = 0UL;
    autotest_meta.data_size = 0UL;
    autotest_meta.bss_size = 0UL;
    autotest_meta.heap_size = 0UL;
    autotest_meta.s_svcexchange = (size_t)&_autotest_svcexchange;
    autotest_meta.stack_size = 2048; /* should be highly enough */
}

task_meta_t *task_autotest_get_meta(void)
{
    return &autotest_meta;
}