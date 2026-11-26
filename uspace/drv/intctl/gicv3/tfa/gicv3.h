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

/** @addtogroup uspace_drv_gicv3
 * @{
 */
/** @file GICv3 driver.
 * @brief ARM Generic Interrupt Controller v3 declarations.
 */

#ifndef TFA_GICV3_H_
#define TFA_GICV3_H_

/* PPIs INTIDs 16-31 */
#define MAX_PPI_ID		U(31)

/* SGIs: 0-15, PPIs: 16-31 */
#define	IS_SGI_PPI(id)		((id) <= MAX_PPI_ID)

#define GICR_SGIBASE_OFFSET	U(65536)	/* 64 KB */
#define GICR_ISENABLER0		(GICR_SGIBASE_OFFSET + U(0x100))

#define GICR_ISENABLER		GICR_ISENABLER0

#include "../tfa_adapt.h"

/*
 * GICv3 EL3 driver API
 */

void gicv3_enable_interrupt(unsigned int id, uintptr_t gicd_base, uintptr_t proc_gicr_base);

#endif /* TFA_GICV3_H_ */

/** @}
 */
