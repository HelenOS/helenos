/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2022 Jiří Zárevúcky
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

/** @addtogroup kernel_time
 * @{
 */

/**
 * @file
 * @brief High-level clock interrupt handler.
 *
 * This file contains the clock() function which is the source
 * of preemption. It is also responsible for executing expired
 * timeouts.
 *
 */

#include <time/clock.h>
#include <time/timeout.h>
#include <config.h>
#include <synch/spinlock.h>
#include <synch/waitq.h>
#include <halt.h>
#include <proc/scheduler.h>
#include <cpu.h>
#include <arch.h>
#include <adt/list.h>
#include <atomic.h>
#include <proc/thread.h>
#include <sysinfo/sysinfo.h>
#include <barrier.h>
#include <mm/frame.h>
#include <ddi/ddi.h>
#include <arch/cycle.h>
#include <preemption.h>

/* Pointer to variable with uptime */
uptime_t *uptime;

/** Physical memory area of the real time clock */
static parea_t clock_parea;

/** Initialize realtime clock counter
 *
 * The applications (and sometimes kernel) need to access accurate
 * information about realtime data. We allocate 1 page with these
 * data and update it periodically.
 *
 */
void clock_counter_init(void)
{
	uintptr_t faddr = frame_alloc(1, FRAME_LOWMEM | FRAME_ATOMIC, 0);
	if (faddr == 0)
		panic("Cannot allocate page for clock.");

	uptime = (uptime_t *) PA2KA(faddr);

	uptime->seconds1 = 0;
	uptime->seconds2 = 0;
	uptime->useconds = 0;

	ddi_parea_init(&clock_parea);
	clock_parea.pbase = faddr;
	clock_parea.frames = 1;
	clock_parea.unpriv = true;
	clock_parea.mapped = false;
	ddi_parea_register(&clock_parea);

	/*
	 * Prepare information for the userspace so that it can successfully
	 * physmem_map() the clock_parea.
	 *
	 */
	sysinfo_set_item_val("clock.faddr", NULL, (sysarg_t) faddr);
}

/** Update public counters
 *
 * Update it only on first processor
 */
static void clock_update_counters(uint64_t current_tick)
{
	if (CPU->id == 0) {
		uint64_t usec = (1000000 / HZ) * current_tick;

		sysarg_t secs = usec / 1000000;
		sysarg_t usecs = usec % 1000000;

		uptime->seconds1 = secs;
		write_barrier();
		uptime->useconds = usecs;
		write_barrier();
		uptime->seconds2 = secs;
	}
}

static void cpu_update_accounting(void)
{
	// FIXME: get_cycle() is unimplemented on several platforms
	uint64_t now = get_cycle();
	atomic_time_increment(&CPU->busy_cycles, now - CPU_LOCAL->last_cycle);
	CPU_LOCAL->last_cycle = now;
}

/** Clock routine
 *
 * Clock routine executed from clock interrupt handler
 * (assuming interrupts_disable()'d). Runs expired timeouts
 * and preemptive scheduling.
 *
 */
void clock(void)
{
	size_t missed_clock_ticks = CPU_LOCAL->missed_clock_ticks;
	CPU_LOCAL->missed_clock_ticks = 0;

	CPU_LOCAL->current_clock_tick += missed_clock_ticks + 1;
	uint64_t current_clock_tick = CPU_LOCAL->current_clock_tick;
	clock_update_counters(current_clock_tick);

	/* Account CPU usage */
	cpu_update_accounting();

	/*
	 * To avoid lock ordering problems,
	 * run all expired timeouts as you visit them.
	 *
	 */

	irq_spinlock_lock(&CPU->timeoutlock, false);

	link_t *cur;
	while ((cur = list_first(&CPU->timeout_active_list)) != NULL) {
		timeout_t *timeout = list_get_instance(cur, timeout_t, link);

		if (current_clock_tick <= timeout->deadline) {
			break;
		}

		list_remove(cur);
		timeout_handler_t handler = timeout->handler;
		void *arg = timeout->arg;
		atomic_bool *finished = &timeout->finished;

		irq_spinlock_unlock(&CPU->timeoutlock, false);

		handler(arg);

		/* Signal that the handler is finished. */
		atomic_store_explicit(finished, true, memory_order_release);

		irq_spinlock_lock(&CPU->timeoutlock, false);
	}

	irq_spinlock_unlock(&CPU->timeoutlock, false);

	/*
	 * Do CPU usage accounting and find out whether to preempt THREAD.
	 *
	 */

	if (THREAD) {
		if (current_clock_tick >= CPU_LOCAL->preempt_deadline && PREEMPTION_ENABLED) {
			thread_yield();
#ifdef CONFIG_UDEBUG
			/*
			 * Give udebug chance to stop the thread
			 * before it begins executing userspace code.
			 */
			istate_t *istate = THREAD->udebug.uspace_state;
			if ((istate) && (istate_from_uspace(istate)))
				udebug_before_thread_runs();
#endif
		}
	}
}

/** @}
 */
