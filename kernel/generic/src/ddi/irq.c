/*
 * Copyright (c) 2006 Jakub Jermar
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
/**
 * @file
 * @brief IRQ dispatcher
 *
 * This file provides means of connecting IRQs with respective device drivers
 * and logic for dispatching interrupts to IRQ handlers defined by those
 * drivers.
 */

#include <ddi/irq.h>
#include <adt/hash.h>
#include <adt/hash_table.h>
#include <mm/slab.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <console/console.h>
#include <interrupt.h>
#include <mem.h>
#include <arch.h>

slab_cache_t *irq_cache = NULL;

/** Spinlock protecting the kernel IRQ hash table
 *
 * This lock must be taken only when interrupts are disabled.
 *
 */
IRQ_SPINLOCK_STATIC_INITIALIZE(irq_kernel_hash_table_lock);

/** The kernel IRQ hash table. */
static hash_table_t irq_kernel_hash_table;

/** Spinlock protecting the uspace IRQ hash table
 *
 * This lock must be taken only when interrupts are disabled.
 *
 */
IRQ_SPINLOCK_INITIALIZE(irq_uspace_hash_table_lock);

/** The uspace IRQ hash table */
hash_table_t irq_uspace_hash_table;

static size_t irq_ht_hash(const ht_link_t *);
static size_t irq_ht_key_hash(void *);
static bool irq_ht_equal(const ht_link_t *, const ht_link_t *);
static bool irq_ht_key_equal(void *, const ht_link_t *);

static hash_table_ops_t irq_ht_ops = {
	.hash = irq_ht_hash,
	.key_hash = irq_ht_key_hash,
	.equal = irq_ht_equal,
	.key_equal = irq_ht_key_equal
};

/** Last valid INR */
inr_t last_inr = 0;

/** Initialize IRQ subsystem
 *
 * @param inrs    Numbers of unique IRQ numbers or INRs.
 * @param chains  Number of buckets in the hash table.
 *
 */
void irq_init(size_t inrs, size_t chains)
{
	last_inr = inrs - 1;

	irq_cache = slab_cache_create("irq_t", sizeof(irq_t), 0, NULL, NULL,
	    FRAME_ATOMIC);
	assert(irq_cache);

	hash_table_create(&irq_uspace_hash_table, chains, 0, &irq_ht_ops);
	hash_table_create(&irq_kernel_hash_table, chains, 0, &irq_ht_ops);
}

/** Initialize one IRQ structure
 *
 * @param irq  Pointer to the IRQ structure to be initialized.
 *
 */
void irq_initialize(irq_t *irq)
{
	memsetb(irq, sizeof(irq_t), 0);
	irq_spinlock_initialize(&irq->lock, "irq.lock");
	irq->inr = -1;

	irq_initialize_arch(irq);
}

/** Register IRQ for device
 *
 * The irq structure must be filled with information about the interrupt source
 * and with the claim() function pointer and handler() function pointer.
 *
 * @param irq  IRQ structure belonging to a device.
 *
 */
void irq_register(irq_t *irq)
{
	irq_spinlock_lock(&irq_kernel_hash_table_lock, true);
	irq_spinlock_lock(&irq->lock, false);
	hash_table_insert(&irq_kernel_hash_table, &irq->link);
	irq_spinlock_unlock(&irq->lock, false);
	irq_spinlock_unlock(&irq_kernel_hash_table_lock, true);
}

/** Search and lock an IRQ hash table */
static irq_t *
irq_dispatch_and_lock_table(hash_table_t *h, irq_spinlock_t *l, inr_t inr)
{
	irq_spinlock_lock(l, false);
	ht_link_t *first = hash_table_find(h, &inr);
	for (ht_link_t *lnk = first; lnk;
	    lnk = hash_table_find_next(h, first, lnk)) {
		irq_t *irq = hash_table_get_inst(lnk, irq_t, link);
		irq_spinlock_lock(&irq->lock, false);
		if (irq->claim(irq) == IRQ_ACCEPT) {
			/* leave irq locked */
			irq_spinlock_unlock(l, false);
			return irq;
		}
		irq_spinlock_unlock(&irq->lock, false);
	}
	irq_spinlock_unlock(l, false);

	return NULL;
}

/** Dispatch the IRQ
 *
 * We assume this function is only called from interrupt context (i.e. that
 * interrupts are disabled prior to this call).
 *
 * This function attempts to lookup a fitting IRQ structure. In case of success,
 * return with interrupts disabled and holding the respective structure.
 *
 * @param inr  Interrupt number (aka inr or irq).
 *
 * @return IRQ structure of the respective device
 * @return NULL if no IRQ structure found
 *
 */
irq_t *irq_dispatch_and_lock(inr_t inr)
{
	/*
	 * If the kernel console override is on, then try first the kernel
	 * handlers and eventually fall back to uspace handlers.
	 *
	 * In the usual case the uspace handlers have precedence.
	 */

	if (console_override) {
		irq_t *irq = irq_dispatch_and_lock_table(&irq_kernel_hash_table,
		    &irq_kernel_hash_table_lock, inr);
		if (irq)
			return irq;

		return irq_dispatch_and_lock_table(&irq_uspace_hash_table,
		    &irq_uspace_hash_table_lock, inr);
	}

	irq_t *irq = irq_dispatch_and_lock_table(&irq_uspace_hash_table,
	    &irq_uspace_hash_table_lock, inr);
	if (irq)
		return irq;

	return irq_dispatch_and_lock_table(&irq_kernel_hash_table,
	    &irq_kernel_hash_table_lock, inr);
}

/** Return the hash of the key stored in the item. */
size_t irq_ht_hash(const ht_link_t *item)
{
	irq_t *irq = hash_table_get_inst(item, irq_t, link);
	return hash_mix(irq->inr);
}

/** Return the hash of the key. */
size_t irq_ht_key_hash(void *key)
{
	inr_t *inr = (inr_t *) key;
	return hash_mix(*inr);
}

/** Return true if the items have the same lookup key. */
bool irq_ht_equal(const ht_link_t *item1, const ht_link_t *item2)
{
	irq_t *irq1 = hash_table_get_inst(item1, irq_t, link);
	irq_t *irq2 = hash_table_get_inst(item2, irq_t, link);
	return irq1->inr == irq2->inr;
}

/** Return true if the key is equal to the item's lookup key. */
bool irq_ht_key_equal(void *key, const ht_link_t *item)
{
	inr_t *inr = (inr_t *) key;
	irq_t *irq = hash_table_get_inst(item, irq_t, link);
	return irq->inr == *inr;
}

/** @}
 */
