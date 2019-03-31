/*
 * Copyright (c) 2007 Martin Decky
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

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#include <arch/mach/msim/dorder.h>
#include <arch/mach/msim/msim.h>
#include <stdint.h>
#include <smp/ipi.h>
#include <interrupt.h>
#include <arch/asm.h>
#include <typedefs.h>

static irq_t dorder_irq;

#ifdef CONFIG_SMP

void ipi_broadcast_arch(int ipi)
{
	pio_write_32(((ioport32_t *) MSIM_DORDER_ADDRESS), 0x7fffffff);
}

#endif

static irq_ownership_t dorder_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

static void dorder_irq_handler(irq_t *irq)
{
	dorder_ipi_ack(1 << dorder_cpuid());
}

void dorder_init(void)
{
	irq_initialize(&dorder_irq);
	dorder_irq.inr = MSIM_DORDER_IRQ;
	dorder_irq.claim = dorder_claim;
	dorder_irq.handler = dorder_irq_handler;
	irq_register(&dorder_irq);

	cp0_unmask_int(MSIM_DORDER_IRQ);
}

uint32_t dorder_cpuid(void)
{
	return pio_read_32((ioport32_t *) MSIM_DORDER_ADDRESS);
}

void dorder_ipi_ack(uint32_t mask)
{
	pio_write_32((ioport32_t *) (MSIM_DORDER_ADDRESS + 4), mask);
}

/** @}
 */
