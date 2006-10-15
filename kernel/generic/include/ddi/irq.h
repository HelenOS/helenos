/*
 * Copyright (C) 2006 Jakub Jermar
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

/** @addtogroup genericddi
 * @{
 */
/** @file
 */

#ifndef KERN_IRQ_H_
#define KERN_IRQ_H_

#include <arch/types.h>
#include <adt/list.h>
#include <ipc/ipc.h>
#include <ipc/irq.h>
#include <atomic.h>
#include <synch/spinlock.h>

typedef enum {
	IRQ_DECLINE,		/**< Decline to service. */
	IRQ_ACCEPT		/**< Accept to service. */
} irq_ownership_t;

typedef enum {
	IRQ_TRIGGER_LEVEL = 1,
	IRQ_TRIGGER_EDGE
} irq_trigger_t;

typedef struct irq irq_t;

typedef void (* irq_handler_t)(irq_t *irq, void *arg, ...);

/** Structure representing one device IRQ.
 *
 * If one device has multiple interrupts, there will
 * be multiple irq_t instantions with the same
 * devno.
 */
struct irq {
	/** Hash table link. */
	link_t link;

	/** Lock protecting everything in this structure
	 *  except the link member. When both the IRQ
	 *  hash table lock and this lock are to be acquired,
	 *  this lock must not be taken first.
	 */
	SPINLOCK_DECLARE(lock);

	/** Unique device number. -1 if not yet assigned. */
	devno_t devno;

	/** Actual IRQ number. -1 if not yet assigned. */
	inr_t inr;
	/** Trigger level of the IRQ.*/
	irq_trigger_t trigger;
	/** Claim ownership of the IRQ. */
	irq_ownership_t (* claim)(void);
	/** Handler for this IRQ and device. */
	irq_handler_t handler;
	/** Argument for the handler. */
	void *arg;

	/** Answerbox of the task that wanted to be notified. */
	answerbox_t *notif_answerbox;
	/** Pseudo-code to be performed by the top-half
	 *  before a notification is sent. */
	irq_code_t *code;
	/** Method of the notification. */
	unative_t method;
	/** Counter of IRQ notifications. */
	atomic_t counter;
};

extern void irq_init(count_t inrs, count_t chains);
extern void irq_initialize(irq_t *irq);
extern void irq_register(irq_t *irq);
extern irq_t *irq_dispatch_and_lock(inr_t inr);
extern irq_t *irq_find_and_lock(inr_t inr, devno_t devno);

#endif

/** @}
 */
