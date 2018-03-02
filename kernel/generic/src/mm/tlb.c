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

/** @addtogroup genericmm
 * @{
 */

/**
 * @file
 * @brief Generic TLB shootdown algorithm.
 *
 * The algorithm implemented here is based on the CMU TLB shootdown
 * algorithm and is further simplified (e.g. all CPUs receive all TLB
 * shootdown messages).
 */

#include <mm/tlb.h>
#include <mm/asid.h>
#include <arch/mm/tlb.h>
#include <assert.h>
#include <smp/ipi.h>
#include <synch/spinlock.h>
#include <atomic.h>
#include <arch/interrupt.h>
#include <config.h>
#include <arch.h>
#include <panic.h>
#include <cpu.h>

void tlb_init(void)
{
	tlb_arch_init();
}

#ifdef CONFIG_SMP

/**
 * This lock is used for synchronisation between sender and
 * recipients of TLB shootdown message. It must be acquired
 * before CPU structure lock.
 *
 */
IRQ_SPINLOCK_STATIC_INITIALIZE(tlblock);

/** Send TLB shootdown message.
 *
 * This function attempts to deliver TLB shootdown message
 * to all other processors.
 *
 * @param type  Type describing scope of shootdown.
 * @param asid  Address space, if required by type.
 * @param page  Virtual page address, if required by type.
 * @param count Number of pages, if required by type.
 *
 * @return The interrupt priority level as it existed prior to this call.
 *
 */
ipl_t tlb_shootdown_start(tlb_invalidate_type_t type, asid_t asid,
    uintptr_t page, size_t count)
{
	ipl_t ipl = interrupts_disable();
	CPU->tlb_active = false;
	irq_spinlock_lock(&tlblock, false);

	size_t i;
	for (i = 0; i < config.cpu_count; i++) {
		if (i == CPU->id)
			continue;

		cpu_t *cpu = &cpus[i];

		irq_spinlock_lock(&cpu->lock, false);
		if (cpu->tlb_messages_count == TLB_MESSAGE_QUEUE_LEN) {
			/*
			 * The message queue is full.
			 * Erase the queue and store one TLB_INVL_ALL message.
			 */
			cpu->tlb_messages_count = 1;
			cpu->tlb_messages[0].type = TLB_INVL_ALL;
			cpu->tlb_messages[0].asid = ASID_INVALID;
			cpu->tlb_messages[0].page = 0;
			cpu->tlb_messages[0].count = 0;
		} else {
			/*
			 * Enqueue the message.
			 */
			size_t idx = cpu->tlb_messages_count++;
			cpu->tlb_messages[idx].type = type;
			cpu->tlb_messages[idx].asid = asid;
			cpu->tlb_messages[idx].page = page;
			cpu->tlb_messages[idx].count = count;
		}
		irq_spinlock_unlock(&cpu->lock, false);
	}

	tlb_shootdown_ipi_send();

busy_wait:
	for (i = 0; i < config.cpu_count; i++) {
		if (cpus[i].tlb_active)
			goto busy_wait;
	}

	return ipl;
}

/** Finish TLB shootdown sequence.
 *
 * @param ipl Previous interrupt priority level.
 *
 */
void tlb_shootdown_finalize(ipl_t ipl)
{
	irq_spinlock_unlock(&tlblock, false);
	CPU->tlb_active = true;
	interrupts_restore(ipl);
}

void tlb_shootdown_ipi_send(void)
{
	ipi_broadcast(VECTOR_TLB_SHOOTDOWN_IPI);
}

/** Receive TLB shootdown message.
 *
 */
void tlb_shootdown_ipi_recv(void)
{
	assert(CPU);

	CPU->tlb_active = false;
	irq_spinlock_lock(&tlblock, false);
	irq_spinlock_unlock(&tlblock, false);

	irq_spinlock_lock(&CPU->lock, false);
	assert(CPU->tlb_messages_count <= TLB_MESSAGE_QUEUE_LEN);

	size_t i;
	for (i = 0; i < CPU->tlb_messages_count; i++) {
		tlb_invalidate_type_t type = CPU->tlb_messages[i].type;
		asid_t asid = CPU->tlb_messages[i].asid;
		uintptr_t page = CPU->tlb_messages[i].page;
		size_t count = CPU->tlb_messages[i].count;

		switch (type) {
		case TLB_INVL_ALL:
			tlb_invalidate_all();
			break;
		case TLB_INVL_ASID:
			tlb_invalidate_asid(asid);
			break;
		case TLB_INVL_PAGES:
			assert(count);
			tlb_invalidate_pages(asid, page, count);
			break;
		default:
			panic("Unknown type (%d).", type);
			break;
		}

		if (type == TLB_INVL_ALL)
			break;
	}

	CPU->tlb_messages_count = 0;
	irq_spinlock_unlock(&CPU->lock, false);
	CPU->tlb_active = true;
}

#endif /* CONFIG_SMP */

/** @}
 */
