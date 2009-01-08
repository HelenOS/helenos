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
 * @brief	Generic TLB shootdown algorithm.
 *
 * The algorithm implemented here is based on the CMU TLB shootdown
 * algorithm and is further simplified (e.g. all CPUs receive all TLB
 * shootdown messages).
 */

#include <mm/tlb.h>
#include <mm/asid.h>
#include <arch/mm/tlb.h>
#include <smp/ipi.h>
#include <synch/spinlock.h>
#include <atomic.h>
#include <arch/interrupt.h>
#include <config.h>
#include <arch.h>
#include <panic.h>
#include <debug.h>
#include <cpu.h>

/**
 * This lock is used for synchronisation between sender and
 * recipients of TLB shootdown message. It must be acquired
 * before CPU structure lock.
 */
SPINLOCK_INITIALIZE(tlblock);

void tlb_init(void)
{
	tlb_arch_init();
}

#ifdef CONFIG_SMP

/** Send TLB shootdown message.
 *
 * This function attempts to deliver TLB shootdown message
 * to all other processors.
 *
 * This function must be called with interrupts disabled.
 *
 * @param type Type describing scope of shootdown.
 * @param asid Address space, if required by type.
 * @param page Virtual page address, if required by type.
 * @param count Number of pages, if required by type.
 */
void tlb_shootdown_start(tlb_invalidate_type_t type, asid_t asid,
    uintptr_t page, count_t count)
{
	unsigned int i;

	CPU->tlb_active = 0;
	spinlock_lock(&tlblock);
	
	for (i = 0; i < config.cpu_count; i++) {
		cpu_t *cpu;
		
		if (i == CPU->id)
			continue;

		cpu = &cpus[i];
		spinlock_lock(&cpu->lock);
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
			index_t idx = cpu->tlb_messages_count++;
			cpu->tlb_messages[idx].type = type;
			cpu->tlb_messages[idx].asid = asid;
			cpu->tlb_messages[idx].page = page;
			cpu->tlb_messages[idx].count = count;
		}
		spinlock_unlock(&cpu->lock);
	}
	
	tlb_shootdown_ipi_send();

busy_wait:	
	for (i = 0; i < config.cpu_count; i++)
		if (cpus[i].tlb_active)
			goto busy_wait;
}

/** Finish TLB shootdown sequence. */
void tlb_shootdown_finalize(void)
{
	spinlock_unlock(&tlblock);
	CPU->tlb_active = 1;
}

void tlb_shootdown_ipi_send(void)
{
	ipi_broadcast(VECTOR_TLB_SHOOTDOWN_IPI);
}

/** Receive TLB shootdown message. */
void tlb_shootdown_ipi_recv(void)
{
	tlb_invalidate_type_t type;
	asid_t asid;
	uintptr_t page;
	count_t count;
	unsigned int i;
	
	ASSERT(CPU);
	
	CPU->tlb_active = 0;
	spinlock_lock(&tlblock);
	spinlock_unlock(&tlblock);
	
	spinlock_lock(&CPU->lock);
	ASSERT(CPU->tlb_messages_count <= TLB_MESSAGE_QUEUE_LEN);

	for (i = 0; i < CPU->tlb_messages_count; CPU->tlb_messages_count--) {
		type = CPU->tlb_messages[i].type;
		asid = CPU->tlb_messages[i].asid;
		page = CPU->tlb_messages[i].page;
		count = CPU->tlb_messages[i].count;

		switch (type) {
		case TLB_INVL_ALL:
			tlb_invalidate_all();
			break;
		case TLB_INVL_ASID:
			tlb_invalidate_asid(asid);
			break;
		case TLB_INVL_PAGES:
		    	ASSERT(count);
			tlb_invalidate_pages(asid, page, count);
			break;
		default:
			panic("Unknown type (%d).", type);
			break;
		}
		if (type == TLB_INVL_ALL)
			break;
	}
	
	spinlock_unlock(&CPU->lock);
	CPU->tlb_active = 1;
}

#endif /* CONFIG_SMP */

/** @}
 */
