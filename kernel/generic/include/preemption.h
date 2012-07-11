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
/** @file
 */

#ifndef KERN_PREEMPTION_H_
#define KERN_PREEMPTION_H_

#include <arch.h>
#include <compiler/barrier.h>
#include <debug.h>

#define PREEMPTION_INC         (1 << 1)
#define PREEMPTION_NEEDED_FLAG (1 << 0)
#define PREEMPTION_NEEDED      (THE->preemption & PREEMPTION_NEEDED_FLAG)
#define PREEMPTION_DISABLED    (PREEMPTION_INC <= THE->preemption)
#define PREEMPTION_ENABLED     (!PREEMPTION_DISABLED)

/** Increment preemption disabled counter. */
#define preemption_disable() \
	do { \
		THE->preemption += PREEMPTION_INC; \
		compiler_barrier(); \
	} while (0)

/** Restores preemption and reschedules if out time slice already elapsed.*/
#define preemption_enable() \
	do { \
		preemption_enable_noresched(); \
		\
		if (PREEMPTION_ENABLED && PREEMPTION_NEEDED) { \
			preemption_enabled_scheduler(); \
		} \
	} while (0)

/** Restores preemption but never reschedules. */
#define preemption_enable_noresched() \
	do { \
		ASSERT(PREEMPTION_DISABLED); \
		compiler_barrier(); \
		THE->preemption -= PREEMPTION_INC; \
	} while (0)


extern void preemption_enabled_scheduler(void);
extern void preemption_set_needed(void);
extern void preemption_clear_needed(void);

#endif

/** @}
 */
