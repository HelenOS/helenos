/*
 * Copyright (c) 2015-2025 Arm Limited and Contributors.
 * Copyright (c) 2023 NVIDIA Corporation.
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
 * @brief ARM Generic Interrupt Controller v3 main functions.
 */

#include <stdbool.h>
#include <genarch/drivers/gicv3/gicv3.h>
#include <genarch/drivers/gicv3/tfa_adapt.h>
#include <genarch/drivers/gicv3/tfa/gicv3.h>
#include <genarch/drivers/gicv3/tfa/gicv3_private.h>
#include <genarch/drivers/gicv3/tfa/gicv3_common_private.h>

/* Check for valid SGI/PPI or SPI interrupt ID */
static bool is_valid_interrupt(unsigned int id);

/*
 * This function initialises the GIC distributor interface based upon the data
 * provided by the platform while initialising the driver.
 */
void gicv3_distif_init(uintptr_t gicd_base)
{
	/*
	 * Clear the "enable" bits for G1NS interrupts before configuring
	 * the ARE_NS bit. The Distributor might generate a system error
	 * otherwise.
	 */
	gicd_clr_ctlr(gicd_base, CTLR_ENABLE_G1NS_BIT, RWP_TRUE);

	/* Set the ARE_NS bit now that interrupts have been disabled */
	gicd_set_ctlr(gicd_base, CTLR_ARE_NS_BIT, RWP_TRUE);

	/* Set the default attribute of all SPIs */
	gicv3_spis_config_defaults(gicd_base);

	/* Enable the SPIs now that they have been configured */
	gicd_set_ctlr(gicd_base, CTLR_ENABLE_G1NS_BIT, RWP_TRUE);
}

/*
 * This function initialises the GIC Redistributor interface of the calling CPU
 * (identified by the 'proc_num' parameter) based upon the data provided by the
 * platform while initialising the driver.
 */
void gicv3_rdistif_init(uintptr_t proc_gicr_base)
{
	/* Power on redistributor (if it supports power management) */
	gicv3_rdistif_on(proc_gicr_base);

	/* Set the default attribute of all SGIs and PPIs */
	gicv3_ppi_sgi_config_defaults(proc_gicr_base);
}

/*
 * This function enables the GIC CPU interface of the calling CPU using only
 * system register accesses.
 */
void gicv3_cpuif_enable(uintptr_t proc_gicr_base)
{
	/* Mark the connected core as awake */
	gicv3_rdistif_mark_core_awake(proc_gicr_base);

	/* Enable system register access for EL1 */
	write_icc_sre_el1(ICC_SRE_SRE_BIT);
	isb();

	/* Program the idle priority in the PMR */
	write_icc_pmr_el1(GIC_PRI_MASK);

	/* Enable Group1 Non-secure interrupts */
	write_icc_igrpen1_el1(IGRPEN1_EL1_ENABLE_G1NS_BIT);

	isb();
	/* Add DSB to ensure visibility of System register writes */
	dsb();
}

/*
 * This function enables the interrupt identified by id. The proc_num
 * is used if the interrupt is SGI or PPI, and programs the corresponding
 * Redistributor interface.
 */
void gicv3_enable_interrupt(unsigned int id, uintptr_t gicd_base,
    uintptr_t proc_gicr_base)
{
	/*
	 * Ensure that any shared variable updates depending on out of band
	 * interrupt trigger are observed before enabling interrupt.
	 */
	dsbishst();
	if (!is_valid_interrupt(id)) {
		panic("Invalid interrupt ID: %u\n", id);
	}
	/* Check interrupt ID */
	if (IS_SGI_PPI(id)) {
		/* For SGIs: 0-15 and PPIs: 16-31 */
		gicr_set_isenabler(proc_gicr_base, id);
	} else {
		/* For SPIs: 32-1019 */
		gicd_set_isenabler(gicd_base, id);
	}
}

/*
 * This function disables the interrupt identified by id. The proc_num
 * is used if the interrupt is SGI or PPI, and programs the corresponding
 * Redistributor interface.
 */
void gicv3_disable_interrupt(unsigned int id, uintptr_t gicd_base,
    uintptr_t proc_gicr_base)
{
	/*
	 * Disable interrupt, and ensure that any shared variable updates
	 * depending on out of band interrupt trigger are observed afterwards.
	 */
	if (!is_valid_interrupt(id)) {
		panic("Invalid interrupt ID: %u\n", id);
	}
	/* Check interrupt ID */
	if (IS_SGI_PPI(id)) {
		/* For SGIs: 0-15 and PPIs: 16-31 */
		gicr_set_icenabler(proc_gicr_base, id);

		/* Write to clear enable requires waiting for pending writes */
		gicr_wait_for_pending_write(proc_gicr_base);
	} else {
		/* For SPIs: 32-1019 */
		gicd_set_icenabler(gicd_base, id);

		/* Write to clear enable requires waiting for pending writes */
		gicd_wait_for_pending_write(gicd_base);
	}

	dsbishst();
}

/*
 * This function raises the specified SGI to all PEs.
 */
void gicv3_raise_sgi_all(unsigned int sgi_num)
{
	uint64_t sgi_val;

	/* Verify interrupt number is in the SGI range */
	assert(sgi_num < MIN_PPI_ID);

	/* Raise SGI to all PEs */
	sgi_val = GICV3_SGIR_VALUE(0, 0, 0, sgi_num, SGIR_IRM_TO_ALL, 0);

	dsbishst();

	write_icc_sgi1r(sgi_val);

	isb();
}

/*
 * This function sets the interrupt routing for the given SPI interrupt id.
 * The interrupt routing is specified in routing mode and mpidr.
 *
 * The routing mode can be either of:
 *  - GICV3_IRM_ANY
 *  - GICV3_IRM_PE
 *
 * The mpidr is the affinity of the PE to which the interrupt will be routed,
 * and is ignored for routing mode GICV3_IRM_ANY.
 */
void gicv3_set_spi_routing(unsigned int id, unsigned int irm,
    u_register_t mpidr, uintptr_t gicd_base)
{
	unsigned long long aff;
	uint64_t router;

	assert((irm == GICV3_IRM_ANY) || (irm == GICV3_IRM_PE));
	assert(IS_SPI(id));

	aff = gicd_irouter_val_from_mpidr(mpidr, irm);
	gicd_write_irouter(gicd_base, id, aff);

	/*
	 * In implementations that do not require 1 of N distribution of SPIs,
	 * IRM might be RAZ/WI. Read back and verify IRM bit.
	 */
	if (irm == GICV3_IRM_ANY) {
		router = gicd_read_irouter(gicd_base, id);
		if (((router >> IROUTER_IRM_SHIFT) & IROUTER_IRM_MASK) == 0U) {
			panic("GICv3 implementation doesn't support routing ANY\n");
		}
	}
}

/*
 * This function checks the interrupt ID and returns true for SGIs, PPIs
 * and SPIs IDs. Any interrupt ID outside the range is invalid and returns
 * false.
 */
static bool is_valid_interrupt(unsigned int id)
{
	/*
	 * Valid interrupts:
	 * SGIs: 0-15, PPIs: 16-31
	 * SPIs: 32-1019
	 */
	if ((IS_SGI_PPI(id)) || (IS_SPI(id))) {
		return true;
	}

	return false;
}

/** @}
 */
