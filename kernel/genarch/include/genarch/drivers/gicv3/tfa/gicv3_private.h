/*
 * Copyright (c) 2015-2021 ARM Limited and Contributors.
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
 * @brief ARM Generic Interrupt Controller v3 private declarations.
 */

#ifndef KERN_TFA_GICV3_PRIVATE_H_
#define KERN_TFA_GICV3_PRIVATE_H_

#include <genarch/drivers/gicv3/tfa/gic_common.h>
#include <genarch/drivers/gicv3/tfa/gicv3_common_private.h>
#include <genarch/drivers/gicv3/tfa/gicv3.h>
#include <genarch/drivers/gicv3/tfa/arch.h>

/*
 * GICv3 private macro definitions
 */

/* Constants to indicate the status of the RWP bit */
#define RWP_TRUE		U(1)
#define RWP_FALSE		U(0)

/* Calculate GIC register bit number corresponding to its interrupt ID */
#define	BIT_NUM(REG, id)	\
	((id) & ((1U << REG##R_SHIFT) - 1U))

/*
 * Calculate 8, 32 and 64-bit GICD register offset
 * corresponding to its interrupt ID
 */

#define	GICD_OFFSET(REG, id)	\
	(GICD_##REG##R + (((uintptr_t)(id) >> REG##R_SHIFT) << 2))

#define	GICD_OFFSET_64(REG, id)	\
	(GICD_##REG##R + (((uintptr_t)(id) >> REG##R_SHIFT) << 3))

/*
 * Read/Write 8, 32 and 64-bit GIC Distributor register
 * corresponding to its interrupt ID
 */

#define GICD_READ_64(REG, base, id)	\
	mmio_read_64((base) + GICD_OFFSET_64(REG, (id)))

#define GICD_WRITE(REG, base, id, val)	\
	mmio_write_32((base) + GICD_OFFSET(REG, (id)), (val))

#define GICD_WRITE_64(REG, base, id, val)	\
	mmio_write_64((base) + GICD_OFFSET_64(REG, (id)), (val))

/*
 * Bit operations on GIC Distributor register corresponding
 * to its interrupt ID
 */

/* Write bit in GIC Distributor register */
#define	GICD_WRITE_BIT(REG, base, id)			\
	mmio_write_32((base) + GICD_OFFSET(REG, (id)),	\
		((uint32_t)1 << BIT_NUM(REG, (id))))

/*
 * Calculate 8 and 32-bit GICR register offset
 * corresponding to its interrupt ID
 */

#define GICR_OFFSET(REG, id)	\
	(GICR_##REG##R + (((uintptr_t)(id) >> REG##R_SHIFT) << 2))

#define GICR_WRITE(REG, base, id, val)			\
	mmio_write_32((base) + GICR_OFFSET(REG, (id)), (val))

/*
 * Bit operations on GIC Redistributor register
 * corresponding to its interrupt ID
 */

/* Write bit in GIC Redistributor register */
#define	GICR_WRITE_BIT(REG, base, id)				\
	mmio_write_32((base) + GICR_OFFSET(REG, (id)),		\
		((uint32_t)1 << BIT_NUM(REG, (id))))

/*
 * Macro to convert an mpidr to a value suitable for programming into a
 * GICD_IROUTER. Bits[31:24] in the MPIDR are cleared as they are not relevant
 * to GICv3.
 */
static inline u_register_t gicd_irouter_val_from_mpidr(u_register_t mpidr,
    unsigned int irm)
{
	return (mpidr & MPIDR_AFFINITY_MASK) |
	    ((irm & IROUTER_IRM_MASK) << IROUTER_IRM_SHIFT);
}

/*
 * Private GICv3 function prototypes for accessing entire registers.
 * Note: The raw register values correspond to multiple interrupt IDs and
 * the number of interrupt IDs involved depends on the register accessed.
 */
void gicr_write_ipriorityr(uintptr_t base, unsigned int id, unsigned int val);

/*
 * Private GICv3 function prototypes for accessing the GIC registers
 * corresponding to a single interrupt ID. These functions use bitwise
 * operations or appropriate register accesses to modify or return
 * the bit-field corresponding the single interrupt ID.
 */
void gicr_set_isenabler(uintptr_t base, unsigned int id);
void gicr_set_icenabler(uintptr_t base, unsigned int id);

/*
 * Private GICv3 helper function prototypes
 */
unsigned int gicv3_get_spi_limit(uintptr_t gicd_base);
void gicv3_spis_config_defaults(uintptr_t gicd_base);
void gicv3_ppi_sgi_config_defaults(uintptr_t gicr_base);
void gicv3_rdistif_base_addrs_probe(uintptr_t *rdistif_base_addrs,
    unsigned int rdistif_num, uintptr_t gicr_base,
    errno_t (*aff_to_idx)(uint32_t affinity, uint32_t *proc_idx_out));
void gicv3_rdistif_mark_core_awake(uintptr_t gicr_base);

/*
 * GIC Distributor interface accessors
 */
/*
 * Wait for updates to:
 * GICD_CTLR[2:0] - the Group Enables
 * GICD_CTLR[7:4] - the ARE bits, E1NWF bit and DS bit
 * GICD_ICENABLER<n> - the clearing of enable state for SPIs
 */
static inline void gicd_wait_for_pending_write(uintptr_t gicd_base)
{
	while ((gicd_read_ctlr(gicd_base) & GICD_CTLR_RWP_BIT) != 0U) {
	}
}

static inline uint64_t gicd_read_irouter(uintptr_t base, unsigned int id)
{
	assert(id >= MIN_SPI_ID);
	return GICD_READ_64(IROUTE, base, id);
}

static inline void gicd_write_irouter(uintptr_t base,
    unsigned int id, uint64_t affinity)
{
	assert(id >= MIN_SPI_ID);
	GICD_WRITE_64(IROUTE, base, id, affinity);
}

static inline void gicd_clr_ctlr(uintptr_t base,
    unsigned int bitmap, unsigned int rwp)
{
	gicd_write_ctlr(base, gicd_read_ctlr(base) & ~bitmap);
	if (rwp != 0U) {
		gicd_wait_for_pending_write(base);
	}
}

static inline void gicd_set_ctlr(uintptr_t base,
    unsigned int bitmap, unsigned int rwp)
{
	gicd_write_ctlr(base, gicd_read_ctlr(base) | bitmap);
	if (rwp != 0U) {
		gicd_wait_for_pending_write(base);
	}
}

/*
 * GIC Redistributor interface accessors
 */
static inline uint32_t gicr_read_ctlr(uintptr_t base)
{
	return mmio_read_32(base + GICR_CTLR);
}

static inline uint64_t gicr_read_typer(uintptr_t base)
{
	return mmio_read_64(base + GICR_TYPER);
}

static inline uint32_t gicr_read_waker(uintptr_t base)
{
	return mmio_read_32(base + GICR_WAKER);
}

static inline void gicr_write_waker(uintptr_t base, uint32_t val)
{
	mmio_write_32(base + GICR_WAKER, val);
}

/*
 * Wait for updates to:
 * GICR_ICENABLER0
 * GICR_CTLR.DPG1S
 * GICR_CTLR.DPG1NS
 * GICR_CTLR.DPG0
 * GICR_CTLR, which clears EnableLPIs from 1 to 0
 */
static inline void gicr_wait_for_pending_write(uintptr_t gicr_base)
{
	while ((gicr_read_ctlr(gicr_base) & GICR_CTLR_RWP_BIT) != 0U) {
	}
}

/*
 * GIC Redistributor functions for accessing entire registers.
 * Note: The raw register values correspond to multiple interrupt IDs and
 * the number of interrupt IDs involved depends on the register accessed.
 */

static inline void gicr_write_icenabler(uintptr_t base, unsigned int reg_num,
    unsigned int val)
{
	mmio_write_32(base + GICR_ICENABLER + (reg_num << 2), val);
}

static inline void gicr_write_icfgr(uintptr_t base, unsigned int reg_num,
    unsigned int val)
{
	mmio_write_32(base + GICR_ICFGR + (reg_num << 2), val);
}

static inline void gicr_write_igroupr(uintptr_t base, unsigned int reg_num,
    unsigned int val)
{
	mmio_write_32(base + GICR_IGROUPR + (reg_num << 2), val);
}

#endif /* KERN_TFA_GICV3_PRIVATE_H_ */

/** @}
 */
