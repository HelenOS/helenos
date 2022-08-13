/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
