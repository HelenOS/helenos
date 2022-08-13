/*
 * SPDX-FileCopyrightText: 2010 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver
 */

#ifndef DRV_UHCI_HW_STRUCT_QH_H
#define DRV_UHCI_HW_STRUCT_QH_H

#include <assert.h>
#include <stdint.h>
#include <usb/host/utils/malloc32.h>

#include "link_pointer.h"
#include "transfer_descriptor.h"

/** This structure is defined in UHCI design guide p. 31 */
typedef struct queue_head {
	/** Pointer to the next entity (another QH or TD */
	volatile link_pointer_t next;
	/**
	 * Pointer to the contained entities
	 * (execution controlled by vertical flag)
	 */
	volatile link_pointer_t element;
} __attribute__((packed, aligned(16))) qh_t;

/** Initialize queue head structure
 *
 * @param[in] instance qh_t structure to initialize.
 *
 * Sets both pointer to terminal NULL.
 */
static inline void qh_init(qh_t *instance)
{
	assert(instance);

	instance->element = LINK_POINTER_TERM;
	instance->next = LINK_POINTER_TERM;
}

/** Set queue head next pointer
 *
 * @param[in] instance qh_t structure to use.
 * @param[in] next Address of the next queue.
 *
 * Adds proper flag. If the pointer is NULL, sets next to terminal NULL.
 */
static inline void qh_set_next_qh(qh_t *instance, qh_t *next)
{
	/*
	 * Physical address has to be below 4GB,
	 * it is an UHCI limitation and malloc32
	 * should guarantee this
	 */
	const uint32_t pa = addr_to_phys(next);
	if (pa) {
		instance->next = LINK_POINTER_QH(pa);
	} else {
		instance->next = LINK_POINTER_TERM;
	}
}

/** Set queue head element pointer
 *
 * @param[in] instance qh_t structure to use.
 * @param[in] td Transfer descriptor to set as the first element.
 *
 * Adds proper flag. If the pointer is NULL, sets element to terminal NULL.
 */
static inline void qh_set_element_td(qh_t *instance, td_t *td)
{
	/*
	 * Physical address has to be below 4GB,
	 * it is an UHCI limitation and malloc32
	 * should guarantee this
	 */
	const uint32_t pa = addr_to_phys(td);
	if (pa) {
		instance->element = LINK_POINTER_TD(pa);
	} else {
		instance->element = LINK_POINTER_TERM;
	}
}

#endif

/**
 * @}
 */
