/*
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */

#ifndef DRV_OHCI_HW_STRUCT_HCCA_H
#define DRV_OHCI_HW_STRUCT_HCCA_H

#include <stdlib.h>
#include <stdint.h>
#include <macros.h>
#include <assert.h>

#include "mem_access.h"

#define HCCA_INT_EP_COUNT  32

/** Host controller communication area.
 * Shared memory used for communication between the controller and the driver.
 */
typedef struct hcca {
	/** Interrupt endpoints */
	uint32_t int_ep[HCCA_INT_EP_COUNT];
	/** Frame number. */
	uint16_t frame_number;
	PADD16(1);
	/** Pointer to the last completed TD. (useless) */
	uint32_t done_head;
	/** Padding to make the size 256B */
	PADD32(30);
} hcca_t;

static_assert(sizeof(hcca_t) == 256);

/** Allocate properly aligned structure.
 *
 * The returned structure is zeroed upon allocation.
 *
 * @return Usable HCCA memory structure.
 */
static inline hcca_t *hcca_get(void)
{
	hcca_t *hcca = memalign(sizeof(hcca_t), sizeof(hcca_t));
	if (hcca)
		memset(hcca, 0, sizeof(hcca_t));
	return hcca;
}

/** Set HCCA interrupt endpoint pointer table entry.
 * @param hcca HCCA memory structure.
 * @param index table index.
 * @param pa Physical address.
 */
static inline void hcca_set_int_ep(hcca_t *hcca, unsigned index, uintptr_t pa)
{
	assert(hcca);
	assert(index < ARRAY_SIZE(hcca->int_ep));
	OHCI_MEM32_WR(hcca->int_ep[index], pa);
}

#endif

/**
 * @}
 */

