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
 * @brief IRQ dispatcher.
 *
 * This file provides means of connecting IRQs with particular
 * devices and logic for dispatching interrupts to IRQ handlers
 * defined by those devices.
 *
 * This code is designed to support:
 * - multiple devices sharing single IRQ
 * - multiple IRQs per single device
 * - multiple instances of the same device
 *
 *
 * Note about architectures.
 *
 * Some architectures has the term IRQ well defined. Examples
 * of such architectures include amd64, ia32 and mips32. Some
 * other architectures, such as sparc64, don't use the term
 * at all. In those cases, we boldly step forward and define what
 * an IRQ is.
 *
 * The implementation is generic enough and still allows the
 * architectures to use the hardware layout effectively.
 * For instance, on amd64 and ia32, where there is only 16
 * IRQs, the irq_hash_table can be optimized to a one-dimensional
 * array. Next, when it is known that the IRQ numbers (aka INR's)
 * are unique, the claim functions can always return IRQ_ACCEPT.
 *
 *
 * Note about the irq_hash_table.
 *
 * The hash table is configured to use two keys: inr and devno.
 * However, the hash index is computed only from inr. Moreover,
 * if devno is -1, the match is based on the return value of
 * the claim() function instead of on devno.
 */

#include <ddi/irq.h>
#include <adt/hash_table.h>
#include <mm/slab.h>
#include <typedefs.h>
#include <synch/spinlock.h>
#include <console/console.h>
#include <interrupt.h>
#include <memstr.h>
#include <arch.h>

#define KEY_INR    0
#define KEY_DEVNO  1

/** Spinlock protecting the kernel IRQ hash table.
 *
 * This lock must be taken only when interrupts are disabled.
 *
 */
IRQ_SPINLOCK_STATIC_INITIALIZE(irq_kernel_hash_table_lock);

/** The kernel IRQ hash table. */
static hash_table_t irq_kernel_hash_table;

/** Spinlock protecting the uspace IRQ hash table.
 *
 * This lock must be taken only when interrupts are disabled.
 *
 */
IRQ_SPINLOCK_INITIALIZE(irq_uspace_hash_table_lock);

/** The uspace IRQ hash table. */
hash_table_t irq_uspace_hash_table;

/**
 * Hash table operations for cases when we know that
 * there will be collisions between different keys.
 *
 */
static size_t irq_ht_hash(sysarg_t *key);
static bool irq_ht_compare(sysarg_t *key, size_t keys, link_t *item);
static void irq_ht_remove(link_t *item);

static hash_table_operations_t irq_ht_ops = {
	.hash = irq_ht_hash,
	.compare = irq_ht_compare,
	.remove_callback = irq_ht_remove,
};

/**
 * Hash table operations for cases when we know that
 * there will be no collisions between different keys.
 * However, there might be still collisions among
 * elements with single key (sharing of one IRQ).
 *
 */
static size_t irq_lin_hash(sysarg_t *key);
static bool irq_lin_compare(sysarg_t *key, size_t keys, link_t *item);
static void irq_lin_remove(link_t *item);

static hash_table_operations_t irq_lin_ops = {
	.hash = irq_lin_hash,
	.compare = irq_lin_compare,
	.remove_callback = irq_lin_remove,
};

/** Number of buckets in either of the hash tables. */
static size_t buckets;

/** Last valid INR. */
inr_t last_inr = 0;

/** Initialize IRQ subsystem.
 *
 * @param inrs   Numbers of unique IRQ numbers or INRs.
 * @param chains Number of chains in the hash table.
 *
 */
void irq_init(size_t inrs, size_t chains)
{
	buckets = chains;
	last_inr = inrs - 1;

	/*
	 * Be smart about the choice of the hash table operations.
	 * In cases in which inrs equals the requested number of
	 * chains (i.e. where there is no collision between
	 * different keys), we can use optimized set of operations.
	 */
	if (inrs == chains) {
		hash_table_create(&irq_uspace_hash_table, chains, 2,
		    &irq_lin_ops);
		hash_table_create(&irq_kernel_hash_table, chains, 2,
		    &irq_lin_ops);
	} else {
		hash_table_create(&irq_uspace_hash_table, chains, 2,
		    &irq_ht_ops);
		hash_table_create(&irq_kernel_hash_table, chains, 2,
		    &irq_ht_ops);
	}
}

/** Initialize one IRQ structure.
 *
 * @param irq Pointer to the IRQ structure to be initialized.
 *
 */
void irq_initialize(irq_t *irq)
{
	memsetb(irq, sizeof(irq_t), 0);
	link_initialize(&irq->link);
	irq_spinlock_initialize(&irq->lock, "irq.lock");
	link_initialize(&irq->notif_cfg.link);
	irq->inr = -1;
	irq->devno = -1;
	
	irq_initialize_arch(irq);
}

/** Register IRQ for device.
 *
 * The irq structure must be filled with information
 * about the interrupt source and with the claim()
 * function pointer and handler() function pointer.
 *
 * @param irq IRQ structure belonging to a device.
 *
 * @return True on success, false on failure.
 *
 */
void irq_register(irq_t *irq)
{
	sysarg_t key[] = {
		(sysarg_t) irq->inr,
		(sysarg_t) irq->devno
	};
	
	irq_spinlock_lock(&irq_kernel_hash_table_lock, true);
	irq_spinlock_lock(&irq->lock, false);
	hash_table_insert(&irq_kernel_hash_table, key, &irq->link);
	irq_spinlock_unlock(&irq->lock, false);
	irq_spinlock_unlock(&irq_kernel_hash_table_lock, true);
}

/** Search and lock the uspace IRQ hash table.
 *
 */
static irq_t *irq_dispatch_and_lock_uspace(inr_t inr)
{
	link_t *lnk;
	sysarg_t key[] = {
		(sysarg_t) inr,
		(sysarg_t) -1    /* Search will use claim() instead of devno */
	};
	
	irq_spinlock_lock(&irq_uspace_hash_table_lock, false);
	lnk = hash_table_find(&irq_uspace_hash_table, key);
	if (lnk) {
		irq_t *irq = hash_table_get_instance(lnk, irq_t, link);
		irq_spinlock_unlock(&irq_uspace_hash_table_lock, false);
		return irq;
	}
	irq_spinlock_unlock(&irq_uspace_hash_table_lock, false);
	
	return NULL;
}

/** Search and lock the kernel IRQ hash table.
 *
 */
static irq_t *irq_dispatch_and_lock_kernel(inr_t inr)
{
	link_t *lnk;
	sysarg_t key[] = {
		(sysarg_t) inr,
		(sysarg_t) -1    /* Search will use claim() instead of devno */
	};
	
	irq_spinlock_lock(&irq_kernel_hash_table_lock, false);
	lnk = hash_table_find(&irq_kernel_hash_table, key);
	if (lnk) {
		irq_t *irq = hash_table_get_instance(lnk, irq_t, link);
		irq_spinlock_unlock(&irq_kernel_hash_table_lock, false);
		return irq;
	}
	irq_spinlock_unlock(&irq_kernel_hash_table_lock, false);
	
	return NULL;
}

/** Dispatch the IRQ.
 *
 * We assume this function is only called from interrupt
 * context (i.e. that interrupts are disabled prior to
 * this call).
 *
 * This function attempts to lookup a fitting IRQ
 * structure. In case of success, return with interrupts
 * disabled and holding the respective structure.
 *
 * @param inr Interrupt number (aka inr or irq).
 *
 * @return IRQ structure of the respective device or NULL.
 *
 */
irq_t *irq_dispatch_and_lock(inr_t inr)
{
	/*
	 * If the kernel console override is on,
	 * then try first the kernel handlers
	 * and eventually fall back to uspace
	 * handlers.
	 *
	 * In the usual case the uspace handlers
	 * have precedence.
	 */
	
	if (console_override) {
		irq_t *irq = irq_dispatch_and_lock_kernel(inr);
		if (irq)
			return irq;
		
		return irq_dispatch_and_lock_uspace(inr);
	}
	
	irq_t *irq = irq_dispatch_and_lock_uspace(inr);
	if (irq)
		return irq;
	
	return irq_dispatch_and_lock_kernel(inr);
}

/** Compute hash index for the key.
 *
 * This function computes hash index into
 * the IRQ hash table for which there
 * can be collisions between different
 * INRs.
 *
 * The devno is not used to compute the hash.
 *
 * @param key The first of the keys is inr and the second is devno or -1.
 *
 * @return Index into the hash table.
 *
 */
size_t irq_ht_hash(sysarg_t key[])
{
	inr_t inr = (inr_t) key[KEY_INR];
	return inr % buckets;
}

/** Compare hash table element with a key.
 *
 * There are two things to note about this function.
 * First, it is used for the more complex architecture setup
 * in which there are way too many interrupt numbers (i.e. inr's)
 * to arrange the hash table so that collisions occur only
 * among same inrs of different devnos. So the explicit check
 * for inr match must be done.
 * Second, if devno is -1, the second key (i.e. devno) is not
 * used for the match and the result of the claim() function
 * is used instead.
 *
 * This function assumes interrupts are already disabled.
 *
 * @param key  Keys (i.e. inr and devno).
 * @param keys This is 2.
 * @param item The item to compare the key with.
 *
 * @return True on match or false otherwise.
 *
 */
bool irq_ht_compare(sysarg_t key[], size_t keys, link_t *item)
{
	irq_t *irq = hash_table_get_instance(item, irq_t, link);
	inr_t inr = (inr_t) key[KEY_INR];
	devno_t devno = (devno_t) key[KEY_DEVNO];
	
	bool rv;
	
	irq_spinlock_lock(&irq->lock, false);
	if (devno == -1) {
		/* Invoked by irq_dispatch_and_lock(). */
		rv = ((irq->inr == inr) &&
		    (irq->claim(irq) == IRQ_ACCEPT));
	} else {
		/* Invoked by irq_find_and_lock(). */
		rv = ((irq->inr == inr) && (irq->devno == devno));
	}
	
	/* unlock only on non-match */
	if (!rv)
		irq_spinlock_unlock(&irq->lock, false);
	
	return rv;
}

/** Unlock IRQ structure after hash_table_remove().
 *
 * @param lnk Link in the removed and locked IRQ structure.
 */
void irq_ht_remove(link_t *lnk)
{
	irq_t *irq __attribute__((unused))
	    = hash_table_get_instance(lnk, irq_t, link);
	irq_spinlock_unlock(&irq->lock, false);
}

/** Compute hash index for the key.
 *
 * This function computes hash index into
 * the IRQ hash table for which there
 * are no collisions between different
 * INRs.
 *
 * @param key The first of the keys is inr and the second is devno or -1.
 *
 * @return Index into the hash table.
 *
 */
size_t irq_lin_hash(sysarg_t key[])
{
	inr_t inr = (inr_t) key[KEY_INR];
	return inr;
}

/** Compare hash table element with a key.
 *
 * There are two things to note about this function.
 * First, it is used for the less complex architecture setup
 * in which there are not too many interrupt numbers (i.e. inr's)
 * to arrange the hash table so that collisions occur only
 * among same inrs of different devnos. So the explicit check
 * for inr match is not done.
 * Second, if devno is -1, the second key (i.e. devno) is not
 * used for the match and the result of the claim() function
 * is used instead.
 *
 * This function assumes interrupts are already disabled.
 *
 * @param key  Keys (i.e. inr and devno).
 * @param keys This is 2.
 * @param item The item to compare the key with.
 *
 * @return True on match or false otherwise.
 *
 */
bool irq_lin_compare(sysarg_t key[], size_t keys, link_t *item)
{
	irq_t *irq = list_get_instance(item, irq_t, link);
	devno_t devno = (devno_t) key[KEY_DEVNO];
	bool rv;
	
	irq_spinlock_lock(&irq->lock, false);
	if (devno == -1) {
		/* Invoked by irq_dispatch_and_lock() */
		rv = (irq->claim(irq) == IRQ_ACCEPT);
	} else {
		/* Invoked by irq_find_and_lock() */
		rv = (irq->devno == devno);
	}
	
	/* unlock only on non-match */
	if (!rv)
		irq_spinlock_unlock(&irq->lock, false);
	
	return rv;
}

/** Unlock IRQ structure after hash_table_remove().
 *
 * @param lnk Link in the removed and locked IRQ structure.
 *
 */
void irq_lin_remove(link_t *lnk)
{
	irq_t *irq __attribute__((unused))
	    = hash_table_get_instance(lnk, irq_t, link);
	irq_spinlock_unlock(&irq->lock, false);
}

/** @}
 */
