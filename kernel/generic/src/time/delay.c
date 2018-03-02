/*
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

/** @addtogroup time
 * @{
 */

/**
 * @file
 * @brief	Active delay function.
 */

#include <time/delay.h>
#include <proc/thread.h>
#include <typedefs.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>

/** Delay the execution for the given number of microseconds (or slightly more).
 *
 * The delay is implemented as active delay loop.
 *
 * @param usec Number of microseconds to sleep.
 */
void delay(uint32_t usec)
{
	/*
	 * The delay loop is calibrated for each and every CPU in the system.
	 * If running in a thread context, it is therefore necessary to disable
	 * thread migration. We want to do this in a lightweight manner.
	 */
	if (THREAD)
		thread_migration_disable();
	asm_delay_loop(usec * CPU->delay_loop_const);
	if (THREAD)
		thread_migration_enable();
}

/** @}
 */
