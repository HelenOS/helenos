/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_ddi
 * @{
 */
/** @file
 */

#ifndef KERN_DDI_IRQ_H_
#define KERN_DDI_IRQ_H_

#include <typedefs.h>
#include <abi/ddi/irq.h>
#include <adt/list.h>
#include <adt/hash_table.h>
#include <synch/spinlock.h>
#include <proc/task.h>
#include <ipc/ipc.h>
#include <mm/slab.h>

typedef enum {
	IRQ_DECLINE,  /**< Decline to service. */
	IRQ_ACCEPT    /**< Accept to service. */
} irq_ownership_t;

typedef enum {
	IRQ_TRIGGER_LEVEL = 1,
	IRQ_TRIGGER_EDGE
} irq_trigger_t;

struct irq;

typedef void (*irq_handler_t)(struct irq *);

/** Type for function used to clear the interrupt. */
typedef void (*cir_t)(void *, inr_t);

/** IPC notification config structure.
 *
 * Primarily, this structure is encapsulated in the irq_t structure.
 * It is protected by irq_t::lock.
 *
 */
typedef struct {
	/** When false, notifications are not sent. */
	bool notify;
	/** True if the structure is in irq_uspace_hash_table_table */
	bool hashed_in;
	/** Answerbox for notifications. */
	answerbox_t *answerbox;
	/** Interface and method to be used for the notification. */
	sysarg_t imethod;
	/** Arguments that will be sent if the IRQ is claimed. */
	uint32_t scratch[IPC_CALL_LEN];
	/** Top-half IRQ code. */
	irq_code_t *code;
	/** Counter. */
	size_t counter;
} ipc_notif_cfg_t;

/** Structure representing one device IRQ.
 *
 * If one device has multiple interrupts, there will be multiple irq_t
 * instantions.
 */
typedef struct irq {
	/** Hash table link. */
	ht_link_t link;

	/** Lock protecting everything in this structure
	 *  except the link member. When both the IRQ
	 *  hash table lock and this lock are to be acquired,
	 *  this lock must not be taken first.
	 */
	IRQ_SPINLOCK_DECLARE(lock);

	/** Send EOI before processing the interrupt.
	 *  This is essential for timer interrupt which
	 *  has to be acknowledged before doing preemption
	 *  to make sure another timer interrupt will
	 *  be eventually generated.
	 */
	bool preack;

	/** Actual IRQ number. -1 if not yet assigned. */
	inr_t inr;
	/** Trigger level of the IRQ. */
	irq_trigger_t trigger;
	/** Claim ownership of the IRQ. */
	irq_ownership_t (*claim)(struct irq *);
	/** Handler for this IRQ and device. */
	irq_handler_t handler;
	/** Instance argument for the handler and the claim function. */
	void *instance;

	/** Clear interrupt routine. */
	cir_t cir;
	/** First argument to the clear interrupt routine. */
	void *cir_arg;

	/** Notification configuration structure. */
	ipc_notif_cfg_t notif_cfg;
} irq_t;

IRQ_SPINLOCK_EXTERN(irq_uspace_hash_table_lock);
extern hash_table_t irq_uspace_hash_table;

extern slab_cache_t *irq_cache;

extern inr_t last_inr;

extern void irq_init(size_t, size_t);
extern void irq_initialize(irq_t *);
extern void irq_register(irq_t *);
extern irq_t *irq_dispatch_and_lock(inr_t);

#endif

/** @}
 */
