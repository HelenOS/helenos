/*
 * SPDX-FileCopyrightText: 2016 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup genarch
 * @{
 */
/** @file
 * @brief ARM Generic Interrupt Controller, Architecture version 2.0.
 *
 * This IRQ controller is present on the QEMU virt platform for ARM.
 */

#include <arch/asm.h>
#include <genarch/drivers/gicv2/gicv2.h>
#include <assert.h>

/** Initialize GICv2 interrupt controller.
 *
 * @param irqc Instance structure.
 * @param distr Distributor registers.
 * @param cpui CPU interface registers.
 */
void gicv2_init(gicv2_t *irqc, gicv2_distr_regs_t *distr,
    gicv2_cpui_regs_t *cpui)
{
	irqc->distr = distr;
	irqc->cpui = cpui;

	/* Get maximum number of interrupts. */
	uint32_t typer = pio_read_32(&distr->typer);
	irqc->inum_total = (((typer & GICV2D_TYPER_IT_LINES_NUMBER_MASK) >>
	    GICV2D_TYPER_IT_LINES_NUMBER_SHIFT) + 1) * 32;

	/* Disable all interrupts. */
	for (unsigned i = 0; i < irqc->inum_total / 32; i++)
		pio_write_32(&distr->icenabler[i], 0xffffffff);

	/* Enable interrupts for all priority levels. */
	pio_write_32(&cpui->pmr, 0xff);

	/* Enable signaling of interrupts. */
	pio_write_32(&cpui->ctlr, GICV2C_CTLR_ENABLE_FLAG);
	pio_write_32(&distr->ctlr, GICV2D_CTLR_ENABLE_FLAG);
}

/** Obtain total number of interrupts that the controller supports. */
unsigned gicv2_inum_get_total(gicv2_t *irqc)
{
	return irqc->inum_total;
}

/** Obtain number of pending interrupt. */
void gicv2_inum_get(gicv2_t *irqc, unsigned *inum, unsigned *cpuid)
{
	uint32_t iar = pio_read_32(&irqc->cpui->iar);

	*inum = (iar & GICV2C_IAR_INTERRUPT_ID_MASK) >>
	    GICV2C_IAR_INTERRUPT_ID_SHIFT;
	*cpuid = (iar & GICV2C_IAR_CPUID_MASK) >> GICV2C_IAR_CPUID_SHIFT;
}

/** Signal end of interrupt to the controller. */
void gicv2_end(gicv2_t *irqc, unsigned inum, unsigned cpuid)
{
	assert((inum & ~((unsigned) GICV2C_IAR_INTERRUPT_ID_MASK >>
	    GICV2C_IAR_INTERRUPT_ID_SHIFT)) == 0);
	assert((cpuid & ~((unsigned) GICV2C_IAR_CPUID_MASK >>
	    GICV2C_IAR_CPUID_SHIFT)) == 0);

	uint32_t eoir = (inum << GICV2C_IAR_INTERRUPT_ID_SHIFT) |
	    (cpuid << GICV2C_IAR_CPUID_SHIFT);
	pio_write_32(&irqc->cpui->eoir, eoir);
}

/** Enable specific interrupt. */
void gicv2_enable(gicv2_t *irqc, unsigned inum)
{
	assert(inum < irqc->inum_total);

	pio_write_32(&irqc->distr->isenabler[inum / 32], 1 << (inum % 32));
}

/** Disable specific interrupt. */
void gicv2_disable(gicv2_t *irqc, unsigned inum)
{
	assert(inum < irqc->inum_total);

	pio_write_32(&irqc->distr->icenabler[inum / 32], 1 << (inum % 32));
}

/** @}
 */
