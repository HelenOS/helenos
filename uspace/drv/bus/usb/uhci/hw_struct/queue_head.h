/*
 * Copyright (c) 2010 Jan Vesely
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
