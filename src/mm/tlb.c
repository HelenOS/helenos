/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <mm/tlb.h>
#include <smp/ipi.h>
#include <synch/spinlock.h>
#include <typedefs.h>
#include <arch/smp/atomic.h>
#include <arch/interrupt.h>
#include <config.h>

#ifdef __SMP__
static spinlock_t tlblock;
static volatile int tlb_shootdown_cnt;

void tlb_init(void)
{
	spinlock_initialize(&tlblock);
	tlb_shootdown_cnt = 0;
}

/* must be called with interrupts disabled */
void tlb_shootdown_start(void)
{
	spinlock_lock(&tlblock);
	tlb_shootdown_ipi_send();
	
	while (tlb_shootdown_cnt < config.cpu_active - 1)
		;
		
	tlb_shootdown_cnt = 0;
}

void tlb_shootdown_finalize(void)
{
	spinlock_unlock(&tlblock);
}

void tlb_shootdown_ipi_send(void)
{
	ipi_broadcast(VECTOR_TLB_SHOOTDOWN_IPI);
}

void tlb_shootdown_ipi_recv(void)
{
	atomic_inc((int *) &tlb_shootdown_cnt);
	spinlock_lock(&tlblock);
	spinlock_unlock(&tlblock);
	tlb_invalidate(0);	/* TODO: use valid ASID */
}
#endif /* __SMP__ */
