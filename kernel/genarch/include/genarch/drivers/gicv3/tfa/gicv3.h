/*
 * Copyright (c) 2015-2024 Arm Limited and Contributors.
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
 * @brief ARM Generic Interrupt Controller v3 declarations.
 */

#ifndef KERN_TFA_GICV3_H_
#define KERN_TFA_GICV3_H_

/*
 * GICv3 and 3.1 miscellaneous definitions
 */

/* PPIs INTIDs 16-31 */
#define MAX_PPI_ID		U(31)

/* SGIs: 0-15, PPIs: 16-31 */
#define	IS_SGI_PPI(id)		((id) <= MAX_PPI_ID)

/* SPIs: 32-1019 */
#define	IS_SPI(id)		(((id) >= MIN_SPI_ID) && ((id) <= MAX_SPI_ID))

/*
 * GICv3 and 3.1 specific Distributor interface register offsets and constants
 */

/*
 * GICD_IROUTER<n> register is at 0x6000 + 8n, where n is the interrupt ID
 * and n >= 32, making the effective offset as 0x6100
 */
#define GICD_IROUTER		U(0x6000)

/* GICD_CTLR bit definitions */
#define CTLR_ENABLE_G1NS_SHIFT		1
#define CTLR_ARE_NS_SHIFT		5
#define GICD_CTLR_RWP_SHIFT		31

#define CTLR_ENABLE_G1NS_BIT		BIT_32(CTLR_ENABLE_G1NS_SHIFT)
#define CTLR_ARE_NS_BIT			BIT_32(CTLR_ARE_NS_SHIFT)
#define GICD_CTLR_RWP_BIT		BIT_32(GICD_CTLR_RWP_SHIFT)

/* GICD_IROUTER shifts and masks */
#define IROUTER_SHIFT		0
#define IROUTER_IRM_SHIFT	31
#define IROUTER_IRM_MASK	U(0x1)

#define GICV3_IRM_PE		U(0)
#define GICV3_IRM_ANY		U(1)

/*
 * Common GIC Redistributor interface registers & constants
 */
#define GICR_V4_PCPUBASE_SHIFT	0x12
#define GICR_V3_PCPUBASE_SHIFT	0x11
#define GICR_SGIBASE_OFFSET	U(65536)	/* 64 KB */
#define GICR_CTLR		U(0x0)
#define GICR_IIDR		U(0x04)
#define GICR_TYPER		U(0x08)
#define GICR_WAKER		U(0x14)
#define GICR_IGROUPR0		(GICR_SGIBASE_OFFSET + U(0x80))
#define GICR_ISENABLER0		(GICR_SGIBASE_OFFSET + U(0x100))
#define GICR_ICENABLER0		(GICR_SGIBASE_OFFSET + U(0x180))
#define GICR_IPRIORITYR		(GICR_SGIBASE_OFFSET + U(0x400))
#define GICR_ICFGR0		(GICR_SGIBASE_OFFSET + U(0xc00))

#define GICR_IGROUPR		GICR_IGROUPR0
#define GICR_ISENABLER		GICR_ISENABLER0
#define GICR_ICENABLER		GICR_ICENABLER0
#define GICR_ICFGR		GICR_ICFGR0

/* GICR_CTLR bit definitions */
#define GICR_CTLR_RWP_SHIFT	3
#define GICR_CTLR_RWP_BIT	BIT_32(GICR_CTLR_RWP_SHIFT)

/* GICR_WAKER bit definitions */
#define WAKER_CA_SHIFT		2
#define WAKER_PS_SHIFT		1

#define WAKER_CA_MASK		U(0x1)
#define WAKER_PS_MASK		U(0x1)

#define WAKER_CA_BIT		BIT_32(WAKER_CA_SHIFT)
#define WAKER_PS_BIT		BIT_32(WAKER_PS_SHIFT)

/* GICR_TYPER bit definitions */
#define TYPER_VLPI_SHIFT	1

#define TYPER_VLPI_BIT		BIT_32(TYPER_VLPI_SHIFT)

/* GICR_TYPER bit definitions */
#define TYPER_AFF_VAL_SHIFT	32
#define TYPER_LAST_SHIFT	4

#define TYPER_AFF_VAL_MASK	U(0xffffffff)

#define TYPER_LAST_BIT		BIT_32(TYPER_LAST_SHIFT)

/* GICR_IIDR bit definitions */
#define IIDR_PRODUCT_ID_MASK	U(0xff)
#define IIDR_IMPLEMENTER_MASK	U(0xfff)
#define IIDR_PRODUCT_ID_SHIFT	24
#define IIDR_IMPLEMENTER_SHIFT	0

#define IIDR_MODEL_MASK		(IIDR_PRODUCT_ID_MASK << IIDR_PRODUCT_ID_SHIFT | \
				 IIDR_IMPLEMENTER_MASK << IIDR_IMPLEMENTER_SHIFT)

/*
 * GICv3 and 3.1 CPU interface registers & constants
 */
/* ICC_SRE bit definitions */
#define ICC_SRE_SRE_BIT		BIT_32(0)

/* ICC_IGRPEN1_EL1 bit definitions */
#define IGRPEN1_EL1_ENABLE_G1NS_SHIFT	0
#define IGRPEN1_EL1_ENABLE_G1NS_BIT	BIT_32(IGRPEN1_EL1_ENABLE_G1NS_SHIFT)

/* ICC_IAR1_EL1 bit definitions */
#define IAR1_EL1_INTID_MASK		U(0xffffff)

/* ICC SGI macros */
#define SGIR_TGT_MASK			ULL(0xffff)
#define SGIR_AFF1_SHIFT			16
#define SGIR_INTID_SHIFT		24
#define SGIR_INTID_MASK			ULL(0xf)
#define SGIR_AFF2_SHIFT			32
#define SGIR_IRM_SHIFT			40
#define SGIR_IRM_MASK			ULL(0x1)
#define SGIR_AFF3_SHIFT			48
#define SGIR_AFF_MASK			ULL(0xff)

#define SGIR_IRM_TO_AFF			U(0)
#define SGIR_IRM_TO_ALL			U(1)

#define GICV3_SGIR_VALUE(_aff3, _aff2, _aff1, _intid, _irm, _tgt)	\
	((((uint64_t) (_aff3) & SGIR_AFF_MASK) << SGIR_AFF3_SHIFT) |	\
	 (((uint64_t) (_irm) & SGIR_IRM_MASK) << SGIR_IRM_SHIFT) |	\
	 (((uint64_t) (_aff2) & SGIR_AFF_MASK) << SGIR_AFF2_SHIFT) |	\
	 (((_intid) & SGIR_INTID_MASK) << SGIR_INTID_SHIFT) |		\
	 (((_aff1) & SGIR_AFF_MASK) << SGIR_AFF1_SHIFT) |		\
	 ((_tgt) & SGIR_TGT_MASK))

/*
 * GICv3 and 3.1 ITS registers and constants
 */

#include <genarch/drivers/gicv3/tfa_adapt.h>

static inline uintptr_t gicv3_redist_size(uint64_t typer_val)
{
	/* Using V4 implementation just to stay compatible. */

	if ((typer_val & TYPER_VLPI_BIT) != 0U) {
		return 1U << GICR_V4_PCPUBASE_SHIFT;
	} else {
		return 1U << GICR_V3_PCPUBASE_SHIFT;
	}
}

/*
 * Helper GICv3 and 3.1 macros for SEL1
 */
static inline uint32_t gicv3_acknowledge_interrupt_el1(void)
{
	return (uint32_t)read_icc_iar1_el1() & IAR1_EL1_INTID_MASK;
}

static inline void gicv3_end_of_interrupt_el1(unsigned int id)
{
	/*
	 * Interrupt request deassertion from peripheral to GIC happens
	 * by clearing interrupt condition by a write to the peripheral
	 * register. It is desired that the write transfer is complete
	 * before the core tries to change GIC state from 'AP/Active' to
	 * a new state on seeing 'EOI write'.
	 * Since ICC interface writes are not ordered against Device
	 * memory writes, a barrier is required to ensure the ordering.
	 * The dsb will also ensure *completion* of previous writes with
	 * DEVICE nGnRnE attribute.
	 */
	dsbishst();
	write_icc_eoir1_el1(id);
}

/*
 * GICv3 EL3 driver API
 */
void gicv3_distif_init(uintptr_t gicd_base);
void gicv3_rdistif_init(uintptr_t proc_gicr_base);
void gicv3_rdistif_on(uintptr_t proc_gicr_base);
void gicv3_cpuif_enable(uintptr_t proc_gicr_base);

void gicv3_enable_interrupt(unsigned int id, uintptr_t gicd_base,
    uintptr_t proc_gicr_base);
void gicv3_disable_interrupt(unsigned int id, uintptr_t gicd_base,
    uintptr_t proc_gicr_base);
void gicv3_raise_sgi_all(unsigned int sgi_num);
void gicv3_set_spi_routing(unsigned int id, unsigned int irm,
    u_register_t mpidr, uintptr_t gicd_base);

#endif /* KERN_TFA_GICV3_H_ */

/** @}
 */
