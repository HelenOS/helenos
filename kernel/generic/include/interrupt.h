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

/** @addtogroup kernel_generic_interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_INTERRUPT_H_
#define KERN_INTERRUPT_H_

#include <arch/interrupt.h>
#include <print.h>
#include <stdarg.h>
#include <typedefs.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <arch.h>
#include <ddi/irq.h>
#include <stacktrace.h>
#include <arch/istate.h>

typedef void (*iroutine_t)(unsigned int, istate_t *);

typedef struct {
	const char *name;
	bool hot;
	iroutine_t handler;
	uint64_t cycles;
	uint64_t count;
} exc_table_t;

IRQ_SPINLOCK_EXTERN(exctbl_lock);
extern exc_table_t exc_table[];

extern void fault_from_uspace(istate_t *, const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(2, 3);
extern void fault_if_from_uspace(istate_t *, const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(2, 3);
extern istate_t *istate_get(thread_t *);
extern iroutine_t exc_register(unsigned int, const char *, bool, iroutine_t);
extern void exc_dispatch(unsigned int, istate_t *);
extern void exc_init(void);

extern void irq_initialize_arch(irq_t *);

extern void istate_decode(istate_t *);

#endif

/** @}
 */
