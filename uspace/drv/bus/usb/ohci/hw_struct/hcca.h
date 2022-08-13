/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static_assert(sizeof(hcca_t) == 256, "");

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
