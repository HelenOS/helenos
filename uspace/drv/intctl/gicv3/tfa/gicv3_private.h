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

/** @addtogroup uspace_drv_gicv3
 * @{
 */
/** @file GICv3 driver.
 * @brief ARM Generic Interrupt Controller v3 private declarations.
 */

#ifndef TFA_GICV3_PRIVATE_H_
#define TFA_GICV3_PRIVATE_H_

#include "gic_common.h"
#include "../tfa_adapt.h"

/* Calculate GIC register bit number corresponding to its interrupt ID */
#define	BIT_NUM(REG, id)	\
	((id) & ((1U << REG##R_SHIFT) - 1U))

/*
 * Calculate 8, 32 and 64-bit GICD register offset
 * corresponding to its interrupt ID
 */

#define	GICD_OFFSET(REG, id)	\
	(GICD_##REG##R + (((uintptr_t)(id) >> REG##R_SHIFT) << 2))

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

/*
 * Bit operations on GIC Redistributor register
 * corresponding to its interrupt ID
 */

/* Write bit in GIC Redistributor register */
#define	GICR_WRITE_BIT(REG, base, id)				\
	mmio_write_32((base) + GICR_OFFSET(REG, (id)),		\
		((uint32_t)1 << BIT_NUM(REG, (id))))

/*
 * Private GICv3 function prototypes for accessing the GIC registers
 * corresponding to a single interrupt ID. These functions use bitwise
 * operations or appropriate register accesses to modify or return
 * the bit-field corresponding the single interrupt ID.
 */
void gicr_set_isenabler(uintptr_t base, unsigned int id);

/*
 * Private GICv3 helper function prototypes
 */
unsigned int gicv3_get_spi_limit(uintptr_t gicd_base);

#endif /* TFA_GICV3_PRIVATE_H_ */
