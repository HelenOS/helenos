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
/** @addtogroup drv usbuhcihc
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#ifndef DRV_UHCI_QH_H
#define DRV_UHCI_QH_H

/* libc */
#include <assert.h>

#include "link_pointer.h"
#include "utils/malloc32.h"

typedef struct queue_head {
	volatile link_pointer_t next;
	volatile link_pointer_t element;
} __attribute__((packed)) qh_t;
/*----------------------------------------------------------------------------*/
/** Initialize queue head structure
 *
 * @param[in] instance qh_t structure to initialize.
 *
 * Sets both pointer to terminal NULL.
 */
static inline void qh_init(qh_t *instance)
{
	assert(instance);

	instance->element = 0 | LINK_POINTER_TERMINATE_FLAG;
	instance->next = 0 | LINK_POINTER_TERMINATE_FLAG;
}
/*----------------------------------------------------------------------------*/
/** Set queue head next pointer
 *
 * @param[in] instance qh_t structure to use.
 * @param[in] pa Physical address of the next queue head.
 *
 * Adds proper flag. If the pointer is NULL or terminal, sets next to terminal
 * NULL.
 */
static inline void qh_set_next_qh(qh_t *instance, uint32_t pa)
{
	/* Address is valid and not terminal */
	if (pa && ((pa & LINK_POINTER_TERMINATE_FLAG) == 0)) {
		instance->next = LINK_POINTER_QH(pa);
	} else {
		instance->next = LINK_POINTER_TERM;
	}
}
/*----------------------------------------------------------------------------*/
/** Set queue head element pointer
 *
 * @param[in] instance qh_t structure to initialize.
 * @param[in] pa Physical address of the next queue head.
 *
 * Adds proper flag. If the pointer is NULL or terminal, sets element
 * to terminal NULL.
 */
static inline void qh_set_element_qh(qh_t *instance, uint32_t pa)
{
	/* Address is valid and not terminal */
	if (pa && ((pa & LINK_POINTER_TERMINATE_FLAG) == 0)) {
		instance->element = LINK_POINTER_QH(pa);
	} else {
		instance->element = LINK_POINTER_TERM;
	}
}
/*----------------------------------------------------------------------------*/
/** Set queue head element pointer
 *
 * @param[in] instance qh_t structure to initialize.
 * @param[in] pa Physical address of the TD structure.
 *
 * Adds proper flag. If the pointer is NULL or terminal, sets element
 * to terminal NULL.
 */
static inline void qh_set_element_td(qh_t *instance, uint32_t pa)
{
	if (pa && ((pa & LINK_POINTER_TERMINATE_FLAG) == 0)) {
		instance->element = LINK_POINTER_TD(pa);
	} else {
		instance->element = LINK_POINTER_TERM;
	}
}

#endif
/**
 * @}
 */
