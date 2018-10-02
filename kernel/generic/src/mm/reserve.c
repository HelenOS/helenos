/*
 * Copyright (c) 2011 Jakub Jermar
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

/** @addtogroup kernel_generic_mm
 * @{
 */

/**
 * @file
 * @brief Memory reservations.
 */

#include <assert.h>
#include <mm/reserve.h>
#include <mm/frame.h>
#include <mm/slab.h>
#include <synch/spinlock.h>
#include <typedefs.h>
#include <arch/types.h>

static bool reserve_initialized = false;

IRQ_SPINLOCK_STATIC_INITIALIZE_NAME(reserve_lock, "reserve_lock");
static ssize_t reserve = 0;

/** Initialize memory reservations tracking.
 *
 * This function must be called after frame zones are created and merged
 * and before any address space area is created.
 */
void reserve_init(void)
{
	reserve = frame_total_free_get();
	reserve_initialized = true;
}

/** Try to reserve memory.
 *
 * This function may not be called from contexts that do not allow memory
 * reclaiming, such as some invocations of frame_alloc_generic().
 *
 * @param size		Number of frames to reserve.
 * @return		True on success or false otherwise.
 */
bool reserve_try_alloc(size_t size)
{
	bool reserved = false;

	assert(reserve_initialized);

	irq_spinlock_lock(&reserve_lock, true);
	if (reserve >= 0 && (size_t) reserve >= size) {
		reserve -= size;
		reserved = true;
	} else {
		/*
		 * Some reservable frames may be cached by the slab allocator.
		 * Try to reclaim some reservable memory. Try to be gentle for
		 * the first time. If it does not help, try to reclaim
		 * everything.
		 */
		irq_spinlock_unlock(&reserve_lock, true);
		slab_reclaim(0);
		irq_spinlock_lock(&reserve_lock, true);
		if (reserve >= 0 && (size_t) reserve >= size) {
			reserve -= size;
			reserved = true;
		} else {
			irq_spinlock_unlock(&reserve_lock, true);
			slab_reclaim(SLAB_RECLAIM_ALL);
			irq_spinlock_lock(&reserve_lock, true);
			if (reserve >= 0 && (size_t) reserve >= size) {
				reserve -= size;
				reserved = true;
			}
		}
	}
	irq_spinlock_unlock(&reserve_lock, true);

	return reserved;
}

/** Reserve memory.
 *
 * This function simply marks the respective amount of memory frames reserved.
 * It does not implement any sort of blocking for the case there is not enough
 * reservable memory. It will simply take the reserve into negative numbers and
 * leave the blocking up to the allocation phase.
 *
 * @param size		Number of frames to reserve.
 */
void reserve_force_alloc(size_t size)
{
	if (!reserve_initialized)
		return;

	irq_spinlock_lock(&reserve_lock, true);
	reserve -= size;
	irq_spinlock_unlock(&reserve_lock, true);
}

/** Unreserve memory.
 *
 * @param size		Number of frames to unreserve.
 */
void reserve_free(size_t size)
{
	if (!reserve_initialized)
		return;

	irq_spinlock_lock(&reserve_lock, true);
	reserve += size;
	irq_spinlock_unlock(&reserve_lock, true);
}

/** @}
 */
