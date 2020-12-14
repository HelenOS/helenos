/*
 * Copyright (c) 2014 Jan Vesely
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
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver transfer list structure
 */
#ifndef DRV_EHCI_ENDPOINT_LIST_H
#define DRV_EHCI_ENDPOINT_LIST_H

#include <adt/list.h>
#include <assert.h>
#include <fibril_synch.h>

#include "ehci_bus.h"
#include "hw_struct/queue_head.h"

/** Structure maintains both EHCI queue and software list of active endpoints. */
typedef struct endpoint_list {
	/** Guard against add/remove races */
	fibril_mutex_t guard;
	/** EHCI hw structure at the beginning of the queue */
	qh_t *list_head;
	dma_buffer_t dma_buffer;
	/** Assigned name, provides nicer debug output */
	const char *name;
	/** Sw list of all active EDs */
	list_t endpoint_list;
} endpoint_list_t;

/** Dispose transfer list structures.
 *
 * @param[in] instance Memory place to use.
 *
 * Frees memory of the internal ed_t structure.
 */
static inline void endpoint_list_fini(endpoint_list_t *instance)
{
	assert(instance);
	dma_buffer_free(&instance->dma_buffer);
	instance->list_head = NULL;
}

errno_t endpoint_list_init(endpoint_list_t *instance, const char *name);
void endpoint_list_chain(endpoint_list_t *instance, const endpoint_list_t *next);
void endpoint_list_append_ep(endpoint_list_t *instance, ehci_endpoint_t *ep);
void endpoint_list_remove_ep(endpoint_list_t *instance, ehci_endpoint_t *ep);
#endif
/**
 * @}
 */
