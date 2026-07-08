/*
 * Copyright (c) 2017-2025 Arm Limited and Contributors.
 * Copyright (c) 2020 NVIDIA Corporation.
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
 * @brief ARM Generic Interrupt Controller v3 x00 functions.
 */

/*
 * Driver for GIC-500 and GIC-600 specific features. This driver only provides
 * APIs for functions that are different to those generic ones in GICv3 driver.
 *
 * GIC-600 supports independently power-gating redistributor interface.
 */

#include <typedefs.h>
#include <stdbool.h>
#include <genarch/drivers/gicv3/tfa/gicv3.h>
#include <genarch/drivers/gicv3/tfa/arm_gicv3_common.h>

/* GIC-600 specific register offsets */
#define GICR_PWRR			0x24U

/* GICR_PWRR fields */
#define PWRR_RDPD_SHIFT			0
#define PWRR_RDAG_SHIFT			1
#define PWRR_RDGPD_SHIFT		2
#define PWRR_RDGPO_SHIFT		3

#define PWRR_RDPD			(1U << PWRR_RDPD_SHIFT)
#define PWRR_RDAG			(1U << PWRR_RDAG_SHIFT)
#define PWRR_RDGPD			(1U << PWRR_RDGPD_SHIFT)
#define PWRR_RDGPO			(1U << PWRR_RDGPO_SHIFT)

/*
 * Values to write to GICR_PWRR register to power redistributor
 * for operating through the core (GICR_PWRR.RDAG = 0)
 */
#define PWRR_ON				(0U << PWRR_RDPD_SHIFT)
#define PWRR_OFF			(1U << PWRR_RDPD_SHIFT)

/* GIC-600/700 specific accessor functions */
static void gicr_write_pwrr(uintptr_t base, unsigned int val)
{
	mmio_write_32(base + GICR_PWRR, val);
}

static uint32_t gicr_read_pwrr(uintptr_t base)
{
	return mmio_read_32(base + GICR_PWRR);
}

static void gicr_wait_group_not_in_transit(uintptr_t base)
{
	uint32_t pwrr;

	do {
		pwrr = gicr_read_pwrr(base);

		/* Check group not transitioning: RDGPD == RDGPO */
	} while (((pwrr & PWRR_RDGPD) >> PWRR_RDGPD_SHIFT) !=
	    ((pwrr & PWRR_RDGPO) >> PWRR_RDGPO_SHIFT));
}

static void gic600_pwr_on(uintptr_t base)
{
	do { /* Wait until group not transitioning */
		gicr_wait_group_not_in_transit(base);

		/* Power on redistributor */
		gicr_write_pwrr(base, PWRR_ON);

		/*
		 * Wait until the power on state is reflected.
		 * If RDPD == 0 then powered on.
		 */
	} while ((gicr_read_pwrr(base) & PWRR_RDPD) != PWRR_ON);
}

static bool gicv3_redists_need_power_mgmt(uintptr_t gicr_base)
{
	uint32_t reg = mmio_read_32(gicr_base + GICR_IIDR);

	/*
	 * The Arm GIC-600 and GIC-700 models have their redistributors
	 * powered down at reset.
	 */
	return (((reg & IIDR_MODEL_MASK) == IIDR_MODEL_ARM_GIC_600) ||
	    ((reg & IIDR_MODEL_MASK) == IIDR_MODEL_ARM_GIC_600AE) ||
	    ((reg & IIDR_MODEL_MASK) == IIDR_MODEL_ARM_GIC_700) ||
	    ((reg & IIDR_MODEL_MASK) == IIDR_MODEL_ARM_GIC_720AE));
}

/*
 * Power on GIC-600 redistributor (if configured and detected)
 */
void gicv3_rdistif_on(uintptr_t proc_gicr_base)
{
	/* Power redistributor on */
	if (gicv3_redists_need_power_mgmt(proc_gicr_base)) {
		gic600_pwr_on(proc_gicr_base);
	}
}

/** @}
 */
