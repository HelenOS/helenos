/*
 * Copyright (c) 2026 Jakub Kodym
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

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 * @brief ARM Generic Interrupt Controller, Architecture version 3.0, 64-bit.
 *
 * IRQ controller present on the Quartz64A.
 */

#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#include <panic.h>
#include <genarch/drivers/gicv3/gicv3.h>
#include <genarch/drivers/gicv3/tfa/gicv3.h>
#include <genarch/drivers/gicv3/tfa/gic_common.h>
#include <genarch/drivers/gicv3/tfa/gicv3_private.h>

static uint32_t proc_affinity(void);
static uintptr_t proc_gicr_base(gicv3_t *irqc);
static uint32_t find_total_irq_num(gicv3_t *irqc);
static void gicv3_distr_init(gicv3_t *irqc);
static void gicv3_redistr_probe(gicv3_t *irqc);
static void gicv3_redistr_init(uintptr_t proc_gicr_base);
static void gicv3_cpuif_init(uintptr_t proc_gicr_base);

/**
 * Initialize the GICv3 controller and the global configuration.
 *
 * @param irqc Instance structure.
 * @param gicd_base Base address of the GIC Distributor Area.
 * @param gicr_base Base address of the GIC Redistributor Area.
 * @param gicr_base_addrs Array to store base address for each PE redistributor.
 * @param gicr_base_num Size of the gicr_base_addrs array.
 * @param aff_to_idx Function to map PE affinity to an index inside
 * gicr_base_addrs. Returns zero if
 */
void gicv3_init(gicv3_t *irqc, uintptr_t gicd_base, uintptr_t gicr_base,
    uintptr_t *gicr_base_addrs, unsigned gicr_base_num, aff_to_idx_t aff_to_idx)
{
	assert(irqc != NULL);
	assert(gicd_base != 0U);
	assert(gicr_base != 0U);
	assert(gicr_base_addrs != NULL);
	assert(gicr_base_num > 0U);
	assert(aff_to_idx != NULL);

	irqc->gicd_base = gicd_base;
	irqc->gicr_base = gicr_base;
	irqc->gicr_base_addrs = gicr_base_addrs;
	irqc->gicr_base_num = gicr_base_num;
	irqc->aff_to_idx = aff_to_idx;
	irqc->inum_total = find_total_irq_num(irqc);

	/* Initialize the distributor. */
	gicv3_distr_init(irqc);
	/*
	 * Probe the redistributor base addresses and store them in the platform
	 * provided array.
	 */
	gicv3_redistr_probe(irqc);

	/**
	 * Route SPI interrupt to the current PE by default.
	 *
	 * This is done because many devices (like Quartz64A) do not support 1 to N
	 * interrupt routing, and then the execution leads to panic inside the GICv3
	 * driver.
	 */
	uint64_t mpidr_val = MPIDR_EL1_read();
	for (unsigned int int_id = MIN_SPI_ID; int_id < irqc->inum_total; int_id++) {
		gicv3_set_spi_routing(int_id, GICV3_IRM_PE, mpidr_val, irqc->gicd_base);
	}

	dsbsy();
	isb();
}

/** Initialize the GICv3 redistributor and CPU interface for the current PE. */
void gicv3_pe_init(gicv3_t *irqc)
{
	assert(irqc != NULL);

	uint32_t affinity;
	unsigned int proc_idx;
	errno_t rv;
	uintptr_t proc_gicr_base;

	affinity = proc_affinity();
	rv = irqc->aff_to_idx(affinity, &proc_idx);

	/* The running core must be supported. */
	assert(rv == EOK);

	proc_gicr_base = irqc->gicr_base_addrs[proc_idx];

	assert(proc_gicr_base != 0U);

	/* Initialize the redistributor for the current CPU. */
	gicv3_redistr_init(proc_gicr_base);

	/* Enable the CPU interface. */
	gicv3_cpuif_init(proc_gicr_base);

	dsbsy();
	isb();
}

/** Obtain total number of interrupts that the controller supports. */
unsigned gicv3_irq_get_total(gicv3_t *irqc)
{
	return irqc->inum_total;
}

/** Obtain INTID of the pending interrupt. */
void gicv3_irq_get_id(unsigned *id)
{
	*id = gicv3_acknowledge_interrupt_el1();
}

/** Check if the interrupt is marked as spurious. */
bool gicv3_is_spurious(uint32_t id)
{
	return id == GIC_SPURIOUS_INTERRUPT;
}

/** Signal end of interrupt to the controller. */
void gicv3_end(uint32_t id)
{
	gicv3_end_of_interrupt_el1(id);
}

/** Enable specific interrupt.
 *
 * For IDs in the range of SGIs and PPIs, the interrupt will be enabled on the
 * calling PE. For IDs in the range of SPIs, the interrupt will be enabled on
 * all PEs.
 */
void gicv3_enable(gicv3_t *irqc, uint32_t id)
{
	uintptr_t gicr_base = proc_gicr_base(irqc);
	uintptr_t gicd_base = irqc->gicd_base;

	gicv3_enable_interrupt(id, gicd_base, gicr_base);
}

/** Disable specific interrupt.
 *
 * For IDs in the range of SGIs and PPIs, the interrupt will be disabled on the
 * calling PE. For IDs in the range of SPIs, the interrupt will be disabled on
 * all PEs.
 */
void gicv3_disable(gicv3_t *irqc, uint32_t id)
{
	uintptr_t gicr_base = proc_gicr_base(irqc);
	uintptr_t gicd_base = irqc->gicd_base;

	gicv3_disable_interrupt(id, gicd_base, gicr_base);
}

/** Send SGI to all PEs. */
void gicv3_send_sgi_all(unsigned int sgi_num)
{
	gicv3_raise_sgi_all(sgi_num);
}

/**
 * Helper functions
 */

uint32_t proc_affinity(void)
{
	uint64_t mpidr_val;
	uint32_t affinity;

	mpidr_val = MPIDR_EL1_read();
	affinity = (mpidr_val & MPIDR_AFF012_MASK) >> MPIDR_AFFINITY_AFF012_SHIFT;
	affinity |= (mpidr_val & MPIDR_AFF3_MASK) >> MPIDR_AFFINITY_AFF3_SHIFT;

	return affinity;
}

uintptr_t proc_gicr_base(gicv3_t *irqc)
{
	uint32_t aff_val;
	uint32_t proc_idx;
	errno_t rv;

	aff_val = proc_affinity();
	rv = irqc->aff_to_idx(aff_val, &proc_idx);

	/* The running core must be supported */
	assert(rv == EOK);

	return irqc->gicr_base_addrs[proc_idx];
}

uint32_t find_total_irq_num(gicv3_t *irqc)
{
	/*
	 * The interrupt ID space is contiguous from zero up to the maximum SPI ID.
	 * SGIs: 0-15,
	 * PPIs: 16-31,
	 * SPIs: 32+
	 */
	return gicv3_get_spi_limit(irqc->gicd_base);
}

void gicv3_distr_init(gicv3_t *irqc)
{
	gicv3_distif_init(irqc->gicd_base);
}

void gicv3_redistr_probe(gicv3_t *irqc)
{
	gicv3_rdistif_base_addrs_probe(irqc->gicr_base_addrs, irqc->gicr_base_num,
	    irqc->gicr_base, irqc->aff_to_idx);
}

void gicv3_redistr_init(uintptr_t proc_gicr_base)
{
	gicv3_rdistif_init(proc_gicr_base);
}

void gicv3_cpuif_init(uintptr_t proc_gicr_base)
{
	gicv3_cpuif_enable(proc_gicr_base);
}

/** @}
 */
