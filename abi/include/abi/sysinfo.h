/*
 * Copyright (c) 2010 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup abi_generic
 * @{
 */
/** @file
 * Data structures passed between kernel sysinfo and user space.
 */

#ifndef _ABI_SYSINFO_H_
#define _ABI_SYSINFO_H_

#include <_bits/size_t.h>
#include <stdbool.h>
#include <abi/proc/task.h>
#include <abi/proc/thread.h>
#include <stdint.h>

enum {
	/** Number of load components */
	LOAD_STEPS = 3,

	/** Maximum name sizes */
	TASK_NAME_BUFLEN = 64,
	EXC_NAME_BUFLEN  = 20,
};

/** Item value type
 *
 */
typedef enum {
	SYSINFO_VAL_UNDEFINED = 0,     /**< Undefined value */
	SYSINFO_VAL_VAL = 1,           /**< Constant numeric value */
	SYSINFO_VAL_DATA = 2,          /**< Constant binary data */
	SYSINFO_VAL_FUNCTION_VAL = 3,  /**< Generated numeric value */
	SYSINFO_VAL_FUNCTION_DATA = 4  /**< Generated binary data */
} sysinfo_item_val_type_t;

/** Statistics about a single CPU
 *
 */
typedef struct {
	unsigned int id;         /**< CPU ID as stored by kernel */
	bool active;             /**< CPU is activate */
	uint16_t frequency_mhz;  /**< Frequency in MHz */
	uint64_t idle_cycles;    /**< Number of idle cycles */
	uint64_t busy_cycles;    /**< Number of busy cycles */
} stats_cpu_t;

/** Physical memory statistics
 *
 */
typedef struct {
	uint64_t total;    /**< Total physical memory (bytes) */
	uint64_t unavail;  /**< Unavailable (reserved, firmware) bytes */
	uint64_t used;     /**< Allocated physical memory (bytes) */
	uint64_t free;     /**< Free physical memory (bytes) */
} stats_physmem_t;

/** IPC statistics
 *
 * Associated with a task.
 *
 */
typedef struct {
	uint64_t call_sent;           /**< IPC calls sent */
	uint64_t call_received;       /**< IPC calls received */
	uint64_t answer_sent;         /**< IPC answers sent */
	uint64_t answer_received;     /**< IPC answers received */
	uint64_t irq_notif_received;  /**< IPC IRQ notifications */
	uint64_t forwarded;           /**< IPC messages forwarded */
} stats_ipc_t;

/** Statistics about a single task
 *
 */
typedef struct {
	task_id_t task_id;            /**< Task ID */
	char name[TASK_NAME_BUFLEN];  /**< Task name (in kernel) */
	size_t virtmem;               /**< Size of VAS (bytes) */
	size_t resmem;                /**< Size of resident (used) memory (bytes) */
	size_t threads;               /**< Number of threads */
	uint64_t ucycles;             /**< Number of CPU cycles in user space */
	uint64_t kcycles;             /**< Number of CPU cycles in kernel */
	stats_ipc_t ipc_info;         /**< IPC statistics */
} stats_task_t;

/** Statistics about a single thread
 *
 */
typedef struct {
	thread_id_t thread_id;  /**< Thread ID */
	task_id_t task_id;      /**< Associated task ID */
	state_t state;          /**< Thread state */
	int priority;           /**< Thread priority */
	uint64_t ucycles;       /**< Number of CPU cycles in user space */
	uint64_t kcycles;       /**< Number of CPU cycles in kernel */
	bool on_cpu;            /**< Associated with a CPU */
	unsigned int cpu;       /**< Associated CPU ID (if on_cpu is true) */
} stats_thread_t;

/** Statistics about a single IPC connection
 *
 */
typedef struct {
	task_id_t caller;  /**< Source task ID */
	task_id_t callee;  /**< Target task ID */
} stats_ipcc_t;

/** Statistics about a single exception
 *
 */
typedef struct {
	unsigned int id;             /**< Exception ID */
	char desc[EXC_NAME_BUFLEN];  /**< Description */
	bool hot;                    /**< Active or inactive exception */
	uint64_t cycles;             /**< Number of CPU cycles in the handler */
	uint64_t count;              /**< Number of handled exceptions */
} stats_exc_t;

/** Load fixed-point value */
typedef uint32_t load_t;

#endif

/** @}
 */
