/*
 * Copyright (c) 2015-2023 Arm Limited and Contributors.
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
 * @brief ARM Generic Interrupt Controller v3 helper functions.
 */

#include <errno.h>
#include <typedefs.h>
#include <genarch/drivers/gicv3/tfa_adapt.h>
#include <genarch/drivers/gicv3/tfa/gic_common.h>
#include <genarch/drivers/gicv3/tfa/gicv3_private.h>
#include <genarch/drivers/gicv3/tfa/gicv3_common_private.h>
#include <genarch/drivers/gicv3/tfa/gicv3.h>

/*
 * This function marks the core as awake in the re-distributor and
 * ensures that the interface is active.
 */
void gicv3_rdistif_mark_core_awake(uintptr_t gicr_base)
{
	uint32_t waker = gicr_read_waker(gicr_base);

	/* Only try to mark it as awake when it is asleep. */
	if ((waker & WAKER_PS_BIT) == 0U) {
		/* It is already awake, nothing to do. */
		return;
	}

	/*
	 * ProcessorSleep must only be changed when ChildrenAsleep is 1.
	 * If PS is 1 and CA isn't, wait for that to happen.
	 */
	if ((waker & WAKER_CA_BIT) == 0U) {
		while ((gicr_read_waker(gicr_base) & WAKER_CA_BIT) == 0U) {
		}
	}

	/* Mark the connected core as awake */
	gicr_write_waker(gicr_base, gicr_read_waker(gicr_base) & ~WAKER_PS_BIT);

	/* Wait till the WAKER_CA_BIT changes to 0 */
	while ((gicr_read_waker(gicr_base) & WAKER_CA_BIT) != 0U) {
	}
}

/*
 * This function probes the Redistributor frames when the driver is initialised
 * and saves their base addresses. These base addresses are used later to
 * initialise each Redistributor interface.
 */
void gicv3_rdistif_base_addrs_probe(uintptr_t *rdistif_base_addrs,
    unsigned int rdistif_num,
    uintptr_t gicr_base,
    errno_t (*aff_to_idx)(uint32_t affinity, uint32_t *proc_idx_out))
{
	uint32_t aff;
	unsigned int proc_num;
	uint64_t typer_val;
	uintptr_t rdistif_base = gicr_base;

	assert(rdistif_base_addrs != NULL);
	assert(rdistif_num > 0U);
	assert(aff_to_idx != NULL);

	/*
	 * Iterate over the Redistributor frames. Store the base address of each
	 * frame in the platform provided array. Use the affinity field and the
	 * provided function to index into the array.
	 */
	do {
		typer_val = gicr_read_typer(rdistif_base);

		aff = (uint32_t)((typer_val >> TYPER_AFF_VAL_SHIFT) &
		    TYPER_AFF_VAL_MASK);
		int rv = aff_to_idx(aff, &proc_num);

		if (rv == EOK && proc_num < rdistif_num) {
			rdistif_base_addrs[proc_num] = rdistif_base;
		}
		rdistif_base += gicv3_redist_size(typer_val);
	} while ((typer_val & TYPER_LAST_BIT) == 0U);
}

/*
 * Helper function to get the maximum SPI INTID + 1.
 */
unsigned int gicv3_get_spi_limit(uintptr_t gicd_base)
{
	unsigned int spi_limit;
	unsigned int typer_reg = gicd_read_typer(gicd_base);

	/* (maximum SPI INTID + 1) is equal to 32 * (GICD_TYPER.ITLinesNumber+1) */
	spi_limit = ((typer_reg & TYPER_IT_LINES_NO_MASK) + 1U) << 5;

	/* Filter out special INTIDs 1020-1023 */
	if (spi_limit > (MAX_SPI_ID + 1U)) {
		return MAX_SPI_ID + 1U;
	}

	return spi_limit;
}

/*
 * Helper function to configure the default attributes of SPIs.
 */
void gicv3_spis_config_defaults(uintptr_t gicd_base)
{
	unsigned int i, num_ints;

	num_ints = gicv3_get_spi_limit(gicd_base);

	/* Treat all SPIs as G1NS by default. We do 32 at a time. */
	for (i = MIN_SPI_ID; i < num_ints; i += (1U << IGROUPR_SHIFT)) {
		gicd_write_igroupr(gicd_base, i, ~0U);
	}

	/* Setup the default SPI priorities doing four at a time */
	for (i = MIN_SPI_ID; i < num_ints; i += (1U << IPRIORITYR_SHIFT)) {
		gicd_write_ipriorityr(gicd_base, i, GICD_IPRIORITYR_DEF_VAL);
	}

	/*
	 * Treat all SPIs as level triggered by default, write 16 at a time
	 */
	for (i = MIN_SPI_ID; i < num_ints; i += (1U << ICFGR_SHIFT)) {
		gicd_write_icfgr(gicd_base, i, 0U);
	}
}

/*
 * Helper function to configure the default attributes of PPIs/SGIs
 */
void gicv3_ppi_sgi_config_defaults(uintptr_t gicr_base)
{
	unsigned int i, ppi_regs_num, regs_num;

	ppi_regs_num = 1U;

	/*
	 * Disable all SGIs (imp. def.)/PPIs before configuring them.
	 * This is a more scalable approach as it avoids clearing
	 * the enable bits in the GICD_CTLR.
	 */
	gicr_write_icenabler(gicr_base, 0U, ~0U);

	/* Wait for pending writes to GICR_ICENABLER */
	gicr_wait_for_pending_write(gicr_base);

	/* 32 interrupt IDs per GICR_IGROUPR register */
	/* Treat all SGIs/PPIs as G1NS by default */
	gicr_write_igroupr(gicr_base, 0U, ~0U);

	/* 4 interrupt IDs per GICR_IPRIORITYR register */
	regs_num = ppi_regs_num << 3;
	for (i = 0U; i < regs_num; ++i) {
		/* Setup the default PPI/SGI priorities doing 4 at a time */
		gicr_write_ipriorityr(gicr_base, i << 2, GICD_IPRIORITYR_DEF_VAL);
	}

	/* 16 interrupt IDs per GICR_ICFGR register */
	regs_num = ppi_regs_num << 1;
	for (i = (MIN_PPI_ID >> ICFGR_SHIFT); i < regs_num; ++i) {
		/* Configure all PPIs as level triggered by default */
		gicr_write_icfgr(gicr_base, i, 0U);
	}
}

/** @}
 */
