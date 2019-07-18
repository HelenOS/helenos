/*
 * Copyright (c) 2007 Martin Decky
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 * @brief Shutdown procedures.
 */

#include <abi/shutdown.h>
#include <shutdown.h>
#include <log.h>
#include <cpu.h>
#include <console/kconsole.h>
#include <console/console.h>
#include <proc/thread.h>
#include <stdlib.h>

/* pointer to the thread for the shutdown process */
thread_t *shutdown_thread = NULL;

/** Halt flag */
atomic_t haltstate = 0;

/** Halt wrapper
 *
 * Set halt flag and halt the CPU.
 *
 */
void halt(void)
{
#if (defined(CONFIG_DEBUG)) && (defined(CONFIG_KCONSOLE))
	bool rundebugger = false;

	if (!atomic_load(&haltstate)) {
		atomic_store(&haltstate, 1);
		rundebugger = true;
	}
#else
	atomic_store(&haltstate, 1);
#endif

	interrupts_disable();

#if (defined(CONFIG_DEBUG)) && (defined(CONFIG_KCONSOLE))
	if ((rundebugger) && (kconsole_check_poll()))
		kconsole("panic", "\nLast resort kernel console ready.\n", false);
#endif

	if (CPU)
		log(LF_OTHER, LVL_NOTE, "cpu%u: halted", CPU->id);
	else
		log(LF_OTHER, LVL_NOTE, "cpu: halted");

	cpu_halt();
}

/* Reboots the kernel */
void reboot(void)
{
	task_done();

#ifdef CONFIG_DEBUG
	log(LF_OTHER, LVL_DEBUG, "Rebooting the system");
#endif

	arch_reboot();
	halt();
}

void abort_shutdown()
{
	irq_spinlock_lock(&threads_lock, true);
	thread_t *thread = atomic_load(&shutdown_thread);
	if (thread != NULL) {
		thread_interrupt(thread);
		atomic_store(&shutdown_thread, NULL);
	}
	irq_spinlock_unlock(&threads_lock, true);
}

/* argument structure for the shutdown thread */
typedef struct {
	sysarg_t mode;
	sysarg_t delay;
} sys_shutdown_arg_t;

/* function for the shutdown thread */
static void sys_shutdown_function(void *arguments)
{
	sys_shutdown_arg_t *arg = (sys_shutdown_arg_t *)arguments;

	if (arg->delay != 0) {
		thread_sleep(arg->delay);
	}

	if (thread_interrupted(THREAD)) {
		free(arguments);
		return;
	}

	if (arg->mode == SHUTDOWN_REBOOT) {
		reboot();
	} else {
		halt();
	}
}

/* system call handler for shutdown */
sys_errno_t sys_shutdown(sysarg_t mode, sysarg_t delay, sysarg_t kconsole)
{

#if (defined(CONFIG_DEBUG)) && (defined(CONFIG_KCONSOLE))
	if (kconsole) {
		grab_console();
	}
#endif

	abort_shutdown();

	/* `cancel` or default has been called */
	if (mode != SHUTDOWN_HALT && mode != SHUTDOWN_REBOOT) {
		return EOK;
	}

	sys_shutdown_arg_t *arg = malloc(sizeof(sys_shutdown_arg_t));
	if (arg == NULL) {
		return ENOMEM;
	}

	//TODO: find a better way for accessing the kernel task
	irq_spinlock_lock(&tasks_lock, true);
	task_t *kernel_task = task_find_by_id(1);
	irq_spinlock_unlock(&tasks_lock, true);

	if (kernel_task == NULL) {
		goto error;
	}

	arg->mode = mode;
	arg->delay = delay;

	thread_t *thread = thread_create(sys_shutdown_function, arg, kernel_task, THREAD_FLAG_NONE, "shutdown");

	if (thread == NULL) {
		goto error;
	}

	thread_ready(thread);
	atomic_store(&shutdown_thread, thread);
	return EOK;

error:
	free(arg);
	return ENOENT;
}

/** @}
 */
