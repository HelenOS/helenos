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

/** @addtogroup sparc64mm	
 * @{
 */
/**
 * @file
 * @brief	D-cache shootdown algorithm.
 */

#include <arch/mm/cache.h>

#ifdef CONFIG_SMP
#ifdef CONFIG_VIRT_IDX_DCACHE

#include <smp/ipi.h>
#include <arch/interrupt.h>
#include <synch/spinlock.h>
#include <arch.h>
#include <debug.h>

/**
 * This spinlock is used by the processors to synchronize during the D-cache
 * shootdown.
 */
SPINLOCK_INITIALIZE(dcachelock);

/** Initialize the D-cache shootdown sequence.
 *
 * Start the shootdown sequence by sending out an IPI and wait until all
 * processors spin on the dcachelock spinlock.
 *
 * @param type 	Scope of the D-cache shootdown.
 * @param color	Color to be invalidated; applicable only for DCACHE_INVL_COLOR
 * 	  	and DCACHE_INVL_FRAME invalidation types.
 * @param frame	Frame to be invalidated; applicable only for DCACHE_INVL_FRAME
 * 	  	invalidation types.
 */
void dcache_shootdown_start(dcache_invalidate_type_t type, int color,
    uintptr_t frame)
{
	int i;

	CPU->arch.dcache_active = 0;
	spinlock_lock(&dcachelock);

	for (i = 0; i < config.cpu_count; i++) {
		cpu_t *cpu;

		if (i == CPU->id)
			continue;

		cpu = &cpus[i];
		spinlock_lock(&cpu->lock);
		if (cpu->arch.dcache_message_count ==
		    DCACHE_MSG_QUEUE_LEN) {
			/*
			 * The queue is full, flush the cache entirely.
			 */
			cpu->arch.dcache_message_count = 1;
			cpu->arch.dcache_messages[0].type = DCACHE_INVL_ALL;
			cpu->arch.dcache_messages[0].color = 0; /* ignored */
			cpu->arch.dcache_messages[0].frame = 0; /* ignored */
		} else {
			index_t idx = cpu->arch.dcache_message_count++;
			cpu->arch.dcache_messages[idx].type = type;
			cpu->arch.dcache_messages[idx].color = color;
			cpu->arch.dcache_messages[idx].frame = frame;
		}
		spinlock_unlock(&cpu->lock);
	}

	ipi_broadcast(IPI_DCACHE_SHOOTDOWN);	

busy_wait:
	for (i = 0; i < config.cpu_count; i++)
		if (cpus[i].arch.dcache_active)
			goto busy_wait;
}

/** Finish the D-cache shootdown sequence. */
void dcache_shootdown_finalize(void)
{
	spinlock_unlock(&dcachelock);
	CPU->arch.dcache_active = 1;
}

/** Process the D-cache shootdown IPI. */
void dcache_shootdown_ipi_recv(void)
{
	int i;

	ASSERT(CPU);

	CPU->arch.dcache_active = 0;
	spinlock_lock(&dcachelock);
	spinlock_unlock(&dcachelock);
	
	spinlock_lock(&CPU->lock);
	ASSERT(CPU->arch.dcache_message_count < DCACHE_MSG_QUEUE_LEN);
	for (i = 0; i < CPU->arch.dcache_message_count; i++) {
		switch (CPU->arch.dcache_messages[i].type) {
		case DCACHE_INVL_ALL:
			dcache_flush();
			goto flushed;
			break;
		case DCACHE_INVL_COLOR:
			dcache_flush_color(CPU->arch.dcache_messages[i].color);
			break;
		case DCACHE_INVL_FRAME:
			dcache_flush_frame(CPU->arch.dcache_messages[i].color,
			    CPU->arch.dcache_messages[i].frame);
			break;
		default:
			panic("unknown type (%d)\n",
			    CPU->arch.dcache_messages[i].type);
		}
	}
flushed:
	CPU->arch.dcache_message_count = 0;
	spinlock_unlock(&CPU->lock);

	CPU->arch.dcache_active = 1;
}

#endif /* CONFIG_VIRT_IDX_DCACHE */
#endif /* CONFIG_SMP */

/** @}
 */

