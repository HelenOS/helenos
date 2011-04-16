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

/** @addtogroup genericmm
 * @{
 */

/**
 * @file
 * @brief Memory reservations.
 */

#include <mm/reserve.h>
#include <mm/frame.h>
#include <synch/spinlock.h>
#include <typedefs.h>
#include <arch/types.h>

IRQ_SPINLOCK_STATIC_INITIALIZE_NAME(reserve_lock, "reserve_lock");
static ssize_t reserve = 0;

bool reserve_try_alloc(size_t size)
{
	bool reserved = false;

	irq_spinlock_lock(&reserve_lock, true);
	if (reserve >= 0 && (size_t) reserve >= size) {
		reserve -= size;
		reserved = true;
	}
	irq_spinlock_unlock(&reserve_lock, true);

	return reserved;
}

void reserve_force_alloc(size_t size)
{
	irq_spinlock_lock(&reserve_lock, true);
	reserve -= size;
	irq_spinlock_unlock(&reserve_lock, true);
}

void reserve_free(size_t size)
{
	irq_spinlock_lock(&reserve_lock, true);
	reserve += size;
	irq_spinlock_unlock(&reserve_lock, true);
}

/** @}
 */
