/*
 * SPDX-FileCopyrightText: 2007 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
