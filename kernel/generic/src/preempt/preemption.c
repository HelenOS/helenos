/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup generic
 * @{
 */

/**
 * @file preemption.c
 * @brief Preemption control.
 */

#include <preemption.h>
#include <proc/scheduler.h>


/** Determines if we are executing an exception/interrupt handler. */
static bool in_exc_handler(void)
{
	/* Err on the safe side until all exception processing code is audited. */
	return true;
}

/** Preemption was enabled. Calls scheduler(). */
void preemption_enabled_scheduler(void)
{
	ASSERT(PREEMPTION_ENABLED);
	ASSERT(PREEMPTION_NEEDED);
	
	/* 
	 * Avoid a race between a thread about to invoke the scheduler() 
	 * after checking THREAD->need_resched and an interrupt that
	 * occurs right after the check.
	 * 
	 * Also ensures that code that relies on disabled interrupts
	 * to suppress preemption continues to work.
	 */
	if (!interrupts_disabled() && !in_exc_handler()) {
		preemption_clear_needed();
		/* We may be preempted here, so we'll scheduler() again. Too bad. */
		scheduler();
	} 
}

/** Sets a flag to reschedule the next time preemption is enabled. */
void preemption_set_needed(void)
{
	/* No need to disable interrupts. */
	THE->preemption |= PREEMPTION_NEEDED_FLAG;
}

/** Instructs not to reschedule immediately when preemption is enabled. */
void preemption_clear_needed(void)
{
	/* No need to disable interrupts. */
	THE->preemption &= ~PREEMPTION_NEEDED_FLAG;
}

/** @}
 */
