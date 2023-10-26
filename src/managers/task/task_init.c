// SPDX-FileCopyrightText: 2023 Ledger SAS
// SPDX-License-Identifier: Apache-2.0

/**
 * @file Sentry task manager init automaton functions
 */
#include <string.h>
#include <inttypes.h>
#include <sentry/thread.h>
#include <sentry/task.h>
#include <sentry/sched.h>
#include "task_core.h"
#include "task_init.h"
#include "task_idle.h"

typedef enum task_mgr_state {
    TASK_MANAGER_STATE_BOOT = 0x0UL,                /**< at boot time */
    /* for each cell of task_meta_table */
    TASK_MANAGER_STATE_DISCOVER_SANITATION, /**<  magic & version check */
    TASK_MANAGER_STATE_CHECK_META_INTEGRITY,/**< metadata HMAC check */
    TASK_MANAGER_STATE_CHECK_TSK_INTEGRITY, /**< task HMAC check */
    TASK_MANAGER_STATE_INIT_LOCALINFO,      /**< init dynamic task info into local struct */
    TASK_MANAGER_STATE_TSK_MAP,             /**< task data copy, bss zeroify, stack init */
    TASK_MANAGER_STATE_TSK_SCHEDULE,        /**< schedule task (if start at bootup) */
    TASK_MANAGER_STATE_FINALIZE,            /**< all tasks added, finalize (sort task list) */
    TASK_MANAGER_STATE_READY,               /**< ready state, everything is clean */
    TASK_MANAGER_STATE_ERROR_SECURITY,      /**< hmac or magic error */
    TASK_MANAGER_STATE_ERROR_RUNTIME,       /**< others (sched...) */
} task_mgr_state_t;

struct task_mgr_ctx {
    task_mgr_state_t state;
    uint16_t         numtask;
    kstatus_t        status;
};

static struct task_mgr_ctx ctx;


static inline void task_swap(task_t *t1, task_t *t2)
{
    task_t pivot;
    memcpy(&pivot, t2, sizeof(task_t));
    memcpy(t2, t1, sizeof(task_t));
    memcpy(t1, &pivot, sizeof(task_t));
}

static inline void task_basic_sort(task_t *table)
{
    uint16_t i, j;
    secure_bool_t swapped;
    for (i = 0; i < task_get_num()+1; i++) {
        swapped = SECURE_FALSE;
        for (j = 0; j < task_get_num() - i; j++) {
            if (table[j].metadata->handle.id > table[j+1].metadata->handle.id) {
                task_swap(&table[j], &table[j + 1]);
                swapped = SECURE_TRUE;
            }
        }
        /* If no two elements were swapped, beaking */
        if (swapped == SECURE_TRUE) {
            break;
        }
    }
}

/**
 * @def the task table store all the tasks metadata, forged by the build system
 *
 * The kernel do not set any of this table content by itself, but instead let the
 * project build system fullfill the table, by upgrading this dedicated section.
 *
 * The build system is responsible for positioning each task metadata in its cell.
 *
 * This version of the kernel only support a central task list, meaning that the
 * build system needs to:
 *   1. compile the ELF of each task, independently
 *   2. deduce, once all tasks are compiled as if they are lonely on the target,
 *      a possible mapping where all task can be placed in the flash & SRAM task section
 *      the task mapping order is based on the label list (from the smaller to the higher)
 *      so that binary search can be done on the task set below
 *   3. upgrade each task ELF based on the calculated memory mapping
 *   4. forge the task metadata from the new ELF, including HMACs, save it to a dediacted file
 *   5. store the metadata in the first free cell of the .task_list section bellow
 *
 * In a different (v2?) mode, it is  possible to consider that tasks metadata can be stored
 * in a dedicated sextion of task ELF binary instead and mapped directly in the task region.
 * In that latter case, the task mapping and boot process would be sligthly different so that
 * the kernel would 'search and copy' the tasks metadata in its own section at boot time.
 * Although, once copied, the table would store the very same content.
 */
static const task_meta_t __task_meta_table[CONFIG_MAX_TASKS] __attribute__((used, section(".task_list")));

/**
 * @brief discover_sanitation state handling
 *
 * must be executed in TASK_MANAGER_STATE_DISCOVER_SANITATION state.
 * Move to TASK_MANAGER_STATE_CHECK_META_INTEGRITY only on success, or move to
 * TASK_MANAGER_STATE_ERROR_SECURITY otherwise.
 */
static inline kstatus_t task_init_discover_sanitation(task_meta_t const * const meta)
{
    kstatus_t status = K_SECURITY_INTEGRITY;
    /* entering state check */
    if (unlikely(ctx.state != TASK_MANAGER_STATE_DISCOVER_SANITATION)) {
        ctx.state = TASK_MANAGER_STATE_ERROR_SECURITY;
        goto end;
    }
    if (unlikely(meta->magic != CONFIG_TASK_MAGIC_VALUE)) {
        ctx.state = TASK_MANAGER_STATE_ERROR_SECURITY;
        goto end;
    }
    /* TODO version handling */
    ctx.state = TASK_MANAGER_STATE_CHECK_META_INTEGRITY;
    status = K_STATUS_OKAY;
end:
    return status;
}

/**
 * @brief check_meta_integrity state handling
 *
 * must be executed in TASK_MANAGER_STATE_CHECK_META_INTEGRITY state.
 * Move to TASK_MANAGER_STATE_CHECK_TSK_INTEGRITY only on success, or move to
 * TASK_MANAGER_STATE_ERROR_SECURITY otherwise.
 */
static inline kstatus_t task_init_check_meta_integrity(task_meta_t const * const meta)
{
    kstatus_t status = K_SECURITY_INTEGRITY;
    /* entering state check */
    if (unlikely(ctx.state != TASK_MANAGER_STATE_CHECK_META_INTEGRITY)) {
        ctx.state = TASK_MANAGER_STATE_ERROR_SECURITY;
        goto end;
    }
    /* FIXME: call the hmac service in order to validate metadata integrity,
       and return the result */
    ctx.state = TASK_MANAGER_STATE_CHECK_TSK_INTEGRITY;
    status = K_STATUS_OKAY;
end:
    return status;
}

/**
 * @brief check_tsk_integrity state handling
 *
 * must be executed in TASK_MANAGER_STATE_CHECK_TSK_INTEGRITY state.
 * Move to TASK_MANAGER_STATE_INIT_LOCALINFO only on success, or move to
 * TASK_MANAGER_STATE_ERROR_SECURITY otherwise.
 */
static inline kstatus_t task_init_check_tsk_integrity(task_meta_t const * const meta)
{
    kstatus_t status = K_SECURITY_INTEGRITY;
    /* entering state check */
    if (unlikely(ctx.state != TASK_MANAGER_STATE_CHECK_TSK_INTEGRITY)) {
        ctx.state = TASK_MANAGER_STATE_ERROR_SECURITY;
        goto end;
    }
    /* FIXME: call the hmac service in order to validate metadata integrity,
       and return the result */
    ctx.state = TASK_MANAGER_STATE_INIT_LOCALINFO;
    status = K_STATUS_OKAY;
end:
    return status;
}

/**
 * @brief local info writting state handling
 *
 * must be executed in TASK_MANAGER_STATE_INIT_LOCALINFO state.
 * Move to TASK_MANAGER_STATE_TSK_MAP only on success, or move to
 * TASK_MANAGER_STATE_ERROR_SECURITY otherwise.
 */
static inline kstatus_t task_init_initiate_localinfo(task_meta_t const * const meta)
{
    kstatus_t status = K_SECURITY_INTEGRITY;
    task_t * task_table = task_get_table();
    uint16_t cell = ctx.numtask;

    /* entering state check */
    if (unlikely(ctx.state != TASK_MANAGER_STATE_INIT_LOCALINFO)) {
        ctx.state = TASK_MANAGER_STATE_ERROR_SECURITY;
        goto end;
    }
    /* no complex placement here, only push to end, sort at end of automaton */
    if (unlikely(cell == CONFIG_MAX_TASKS+1)) {
        ctx.state = TASK_MANAGER_STATE_ERROR_SECURITY;
        goto end;
    }
    /* forge local info, push back current and next afterward */
    task_table[cell].metadata = meta;
    task_table[cell].sp = task_initialize_sp(meta->stack_top, (meta->s_text + meta->main_offset));
    /* TODO: ipc & signals ? nothing to init as memset to 0 */
    ctx.state = TASK_MANAGER_STATE_TSK_MAP;
    status = K_STATUS_OKAY;
end:
    return status;
}

/**
 * @brief task memory mapping state handling
 *
 * must be executed in TASK_MANAGER_STATE_TSK_MAP state.
 * Move to TASK_MANAGER_STATE_TSK_SCHEDULE only on success, or move to
 * TASK_MANAGER_STATE_ERROR_SECURITY otherwise.
 */
static inline kstatus_t task_init_map(task_meta_t const * const meta)
{
    /* entering state check */
    if (unlikely(ctx.state != TASK_MANAGER_STATE_TSK_MAP)) {
        ctx.state = TASK_MANAGER_STATE_ERROR_SECURITY;
        goto end;
    }
    memcpy((void*)meta->s_vma_data, (void*)meta->s_data, meta->data_size);
    memset((void*)meta->s_bss, 0x0, meta->bss_size);
    ctx.state = TASK_MANAGER_STATE_TSK_SCHEDULE;
end:
    return K_STATUS_OKAY;
}

/**
 * @brief task scheduling handling
 *
 * must be executed in TASK_MANAGER_STATE_TSK_SCHEDULE state.
 * Move to TASK_MANAGER_STATE_DISCOVER_SANITATION if success and still some
 * tasks to analyze in the meta tab, or move to TASK_MANAGER_STATE_FINALIZE if
 * that was the tast task. Move to TASK_MANAGER_STATE_ERROR_SECURITY in case of
 * error.
 */
static inline kstatus_t task_init_schedule(task_meta_t const * const meta)
{
    kstatus_t status = K_STATUS_OKAY;
    /* entering state check */
    if (unlikely(ctx.state != TASK_MANAGER_STATE_TSK_SCHEDULE)) {
        ctx.state = TASK_MANAGER_STATE_ERROR_SECURITY;
        goto end;
    }
    if (meta->flags & THREAD_FLAG_AUTOSTART) {
        status = sched_schedule(meta->handle);
    }
    if (unlikely(status != K_STATUS_OKAY)) {
        ctx.state = TASK_MANAGER_STATE_ERROR_RUNTIME;
        goto end;
    }
    if (ctx.numtask == task_get_num()) {
        ctx.state = TASK_MANAGER_STATE_FINALIZE;
    } else {
        ctx.state = TASK_MANAGER_STATE_DISCOVER_SANITATION;
    }
end:
    return status;
}

/**
 * ldscript provided
 */
extern size_t _idlestack;
/**
 * @brief finalize the task table construct
 *
 * add the idle task to the local tasks table
 * ordering it based on the label identifier (handle->id value).
 */
static inline kstatus_t task_init_finalize(void)
{
    task_t * task_table = task_get_table();

    /* entering state check */
    if (unlikely(ctx.state != TASK_MANAGER_STATE_FINALIZE)) {
        ctx.status = K_SECURITY_INTEGRITY;
        goto end;
    }
    /* adding idle task to list */
    task_meta_t *meta = task_idle_get_meta();
    meta->handle.rerun = 0;
    meta->handle.id = SCHED_IDLE_TASK_LABEL;
    meta->handle.familly = HANDLE_TASKID;
    meta->magic = CONFIG_TASK_MAGIC_VALUE;
    meta->flags = (THREAD_FLAG_AUTOSTART|THREAD_FLAG_PANICONEXIT);
    meta->stack_top = (size_t)&_idlestack; /* ldscript defined */
    meta->stack_size = 256; /* should be highly enough */
    /* should we though forge a HMAC for idle metadata here ? */
    task_table[ctx.numtask].metadata = meta;
    memset((void*)task_table[ctx.numtask].metadata, 0x0, sizeof(task_meta_t));

    ctx.numtask++;
    /* finishing with sorting task_table based on task label value */
    task_basic_sort(task_table);
    ctx.status = K_STATUS_OKAY;
    ctx.state = TASK_MANAGER_STATE_READY;
end:
    return ctx.status;
}

/**
 * @fn initialize the task context
 *
 * Considering all the potential tasks stored in the task list, the kernel
 * analyze all the cells, check for the metadata and the task integrity and
 * then initialize the task context (data copy, bss zeroification).
 * All tasks that are schedulable at bootup are added to the scheduler queue
 * (call to the sched_schedule() function).
 * The task init do NOT call sched_elect() neither spawn any thread directly.
 * It only prepare the overall task-set in association with the scheduler so
 * that the OS is ready to enter nominal mode.
 *
 * @return K_STATUS_OKAY if all tasks found are clear (I+A), or K_SECURITY_INTEGRITY
 *  if any HMAC calculation fails
 */
kstatus_t task_init(void)
{
    ctx.state = TASK_MANAGER_STATE_BOOT;
    ctx.numtask = 0; /* at the end, before adding idle task, must be equal
                        to buildsys set number of tasks */
    ctx.status = K_STATUS_OKAY;
    /* first zeroify the task table (JTAG reflush case) */
    task_t * task_table = task_get_table();
    memset(task_table, 0x0, (CONFIG_MAX_TASKS+1)*sizeof(task_t));

    ctx.state = TASK_MANAGER_STATE_DISCOVER_SANITATION;
    /* for all tasks, discover, analyse, and init */
    for (uint16_t cell = 0; cell < task_get_num(); ++cell) {
        ctx.status = task_init_discover_sanitation(&__task_meta_table[cell]);
        if (unlikely(ctx.status != K_STATUS_OKAY)) {
            goto end;
        }
        ctx.status = task_init_check_meta_integrity(&__task_meta_table[cell]);
        if (unlikely(ctx.status != K_STATUS_OKAY)) {
            goto end;
        }
        ctx.status = task_init_check_tsk_integrity(&__task_meta_table[cell]);
        if (unlikely(ctx.status != K_STATUS_OKAY)) {
            goto end;
        }
        ctx.status = task_init_initiate_localinfo(&__task_meta_table[cell]);
        if (unlikely(ctx.status != K_STATUS_OKAY)) {
            goto end;
        }
        ctx.status = task_init_map(&__task_meta_table[cell]);
        if (unlikely(ctx.status != K_STATUS_OKAY)) {
            goto end;
        }
        ctx.status = task_init_schedule(&__task_meta_table[cell]);
        if (unlikely(ctx.status != K_STATUS_OKAY)) {
            goto end;
        }
    }
    /* finalize, adding idle task */
    task_init_finalize();
end:
    return ctx.status;
}


/**
 * @fn function that can be called periodically by external security watchdog
 *
 * This function recalculate the metadata integrity (and can recalculate the
 * task .text+rodata potentially)
 */
kstatus_t task_watchdog(void)
{
    return K_STATUS_OKAY;
}