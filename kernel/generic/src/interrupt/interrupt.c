/*
 * Copyright (c) 2005 Ondrej Palkovsky
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

/** @addtogroup genericinterrupt
 * @{
 */
/**
 * @file
 * @brief Interrupt redirector.
 *
 * This file provides means of registering interrupt handlers
 * by kernel functions and calling the handlers when interrupts
 * occur.
 *
 */

#include <interrupt.h>
#include <debug.h>
#include <console/kconsole.h>
#include <console/console.h>
#include <console/cmd.h>
#include <ipc/event.h>
#include <synch/mutex.h>
#include <time/delay.h>
#include <macros.h>
#include <panic.h>
#include <print.h>
#include <symtab.h>
#include <proc/thread.h>

static struct {
	const char *name;
	iroutine f;
} exc_table[IVT_ITEMS];

SPINLOCK_INITIALIZE(exctbl_lock);

/** Register exception handler
 *
 * @param n       Exception number
 * @param name    Description
 * @param handler Exception handler
 *
 */
iroutine exc_register(int n, const char *name, iroutine handler)
{
	ASSERT(n < IVT_ITEMS);
	
	spinlock_lock(&exctbl_lock);
	
	iroutine old = exc_table[n].f;
	exc_table[n].f = handler;
	exc_table[n].name = name;
	
	spinlock_unlock(&exctbl_lock);
	
	return old;
}

/** Dispatch exception according to exception table
 *
 * Called directly from the assembler code.
 * CPU is interrupts_disable()'d.
 *
 */
void exc_dispatch(int n, istate_t *istate)
{
	ASSERT(n < IVT_ITEMS);
	
	/* Account user cycles */
	if (THREAD) {
		irq_spinlock_lock(&THREAD->lock, false);
		thread_update_accounting(true);
		irq_spinlock_unlock(&THREAD->lock, false);
	}
	
#ifdef CONFIG_UDEBUG
	if (THREAD)
		THREAD->udebug.uspace_state = istate;
#endif
	
	exc_table[n].f(n + IVT_FIRST, istate);
	
#ifdef CONFIG_UDEBUG
	if (THREAD)
		THREAD->udebug.uspace_state = NULL;
#endif
	
	/* This is a safe place to exit exiting thread */
	if ((THREAD) && (THREAD->interrupted) && (istate_from_uspace(istate)))
		thread_exit();
	
	if (THREAD) {
		irq_spinlock_lock(&THREAD->lock, false);
		thread_update_accounting(false);
		irq_spinlock_unlock(&THREAD->lock, false);
	}
}

/** Default 'null' exception handler
 *
 */
static void exc_undef(int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "Unhandled exception %d.", n);
	panic("Unhandled exception %d.", n);
}

/** Terminate thread and task if exception came from userspace.
 *
 */
void fault_if_from_uspace(istate_t *istate, const char *fmt, ...)
{
	if (!istate_from_uspace(istate))
		return;
	
	printf("Task %s (%" PRIu64 ") killed due to an exception at "
	    "program counter %p.\n", TASK->name, TASK->taskid,
	    istate_get_pc(istate));
	
	stack_trace_istate(istate);
	
	printf("Kill message: ");
	
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf("\n");
	
	/*
	 * Userspace can subscribe for FAULT events to take action
	 * whenever a thread faults. (E.g. take a dump, run a debugger).
	 * The notification is always available, but unless Udebug is enabled,
	 * that's all you get.
	 */
	if (event_is_subscribed(EVENT_FAULT)) {
		/* Notify the subscriber that a fault occurred. */
		event_notify_3(EVENT_FAULT, LOWER32(TASK->taskid),
		    UPPER32(TASK->taskid), (unative_t) THREAD);
		
#ifdef CONFIG_UDEBUG
		/* Wait for a debugging session. */
		udebug_thread_fault();
#endif
	}
	
	task_kill(TASK->taskid);
	thread_exit();
}

#ifdef CONFIG_KCONSOLE

/** Print all exceptions
 *
 */
static int cmd_exc_print(cmd_arg_t *argv)
{
#if (IVT_ITEMS > 0)
	unsigned int i;
	
	spinlock_lock(&exctbl_lock);
	
#ifdef __32_BITS__
	printf("Exc Description          Handler    Symbol\n");
	printf("--- -------------------- ---------- --------\n");
#endif
	
#ifdef __64_BITS__
	printf("Exc Description          Handler            Symbol\n");
	printf("--- -------------------- ------------------ --------\n");
#endif
	
	for (i = 0; i < IVT_ITEMS; i++) {
		const char *symbol = symtab_fmt_name_lookup((unative_t) exc_table[i].f);
		
#ifdef __32_BITS__
		printf("%-3u %-20s %10p %s\n", i + IVT_FIRST, exc_table[i].name,
			exc_table[i].f, symbol);
#endif
		
#ifdef __64_BITS__
		printf("%-3u %-20s %18p %s\n", i + IVT_FIRST, exc_table[i].name,
			exc_table[i].f, symbol);
#endif
		
		if (((i + 1) % 20) == 0) {
			printf(" -- Press any key to continue -- ");
			spinlock_unlock(&exctbl_lock);
			indev_pop_character(stdin);
			spinlock_lock(&exctbl_lock);
			printf("\n");
		}
	}
	
	spinlock_unlock(&exctbl_lock);
#endif
	
	return 1;
}

static cmd_info_t exc_info = {
	.name = "exc",
	.description = "Print exception table.",
	.func = cmd_exc_print,
	.help = NULL,
	.argc = 0,
	.argv = NULL
};

#endif /* CONFIG_KCONSOLE */

/** Initialize generic exception handling support
 *
 */
void exc_init(void)
{
	(void) exc_undef;
	
#if (IVT_ITEMS > 0)
	unsigned int i;
	
	for (i = 0; i < IVT_ITEMS; i++)
		exc_register(i, "undef", (iroutine) exc_undef);
#endif
	
#ifdef CONFIG_KCONSOLE
	cmd_initialize(&exc_info);
	if (!cmd_register(&exc_info))
		printf("Cannot register command %s\n", exc_info.name);
#endif
}

/** @}
 */
