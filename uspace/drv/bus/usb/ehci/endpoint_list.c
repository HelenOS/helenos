/*
 * Copyright (c) 2014 Jan Vesely
 * Copyright (c) 2018 Ondrej Hlavaty
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
 * @brief EHCI driver transfer list implementation
 */

#include <assert.h>
#include <errno.h>
#include <libarch/barrier.h>

#include <usb/debug.h>

#include "endpoint_list.h"

/** Initialize transfer list structures.
 *
 * @param[in] instance Memory place to use.
 * @param[in] name Name of the new list.
 * @return Error code
 *
 * Allocates memory for internal ed_t structure.
 */
errno_t endpoint_list_init(endpoint_list_t *instance, const char *name)
{
	assert(instance);
	instance->name = name;
	if (dma_buffer_alloc(&instance->dma_buffer, sizeof(qh_t))) {
		usb_log_error("EPL(%p-%s): Failed to allocate list head.",
		    instance, name);
		return ENOMEM;
	}
	instance->list_head = instance->dma_buffer.virt;
	qh_init(instance->list_head, NULL);

	list_initialize(&instance->endpoint_list);
	fibril_mutex_initialize(&instance->guard);

	usb_log_debug2("EPL(%p-%s): Transfer list setup with ED: %p(%" PRIxn ").",
	    instance, name, instance->list_head,
	    addr_to_phys(instance->list_head));

	return EOK;
}

void endpoint_list_chain(endpoint_list_t *instance, const endpoint_list_t *next)
{
	assert(instance);
	assert(next);
	assert(instance->list_head);
	assert(next->list_head);

	usb_log_debug2("EPL(%p-%s): Chained with EPL(%p-%s).",
	    instance, instance->name, next, next->name);

	qh_append_qh(instance->list_head, next->list_head);
}

/** Add endpoint to the end of the list and queue.
 *
 * @param[in] instance List to use.
 * @param[in] endpoint Endpoint to add.
 *
 * The endpoint is added to the end of the list and queue.
 */
void endpoint_list_append_ep(endpoint_list_t *instance, ehci_endpoint_t *ep)
{
	assert(instance);
	assert(instance->list_head);
	assert(ep);
	assert(ep->qh);
	usb_log_debug2("EPL(%p-%s): Append endpoint(%p).",
	    instance, instance->name, ep);

	fibril_mutex_lock(&instance->guard);

	qh_t *last_qh = NULL;
	/* Add to the hardware queue. */
	if (list_empty(&instance->endpoint_list)) {
		/* There are no active EDs */
		last_qh = instance->list_head;
	} else {
		/* There are active EDs, get the last one */
		ehci_endpoint_t *last = ehci_endpoint_list_instance(
		    list_last(&instance->endpoint_list));
		last_qh = last->qh;
	}
	assert(last_qh);
	/* Keep link, this might point to the queue QH, or next chained queue */
	ep->qh->horizontal = last_qh->horizontal;
	/* Make sure QH update is written to the memory */
	write_barrier();

	/* Add QH to the hw queue */
	qh_append_qh(last_qh, ep->qh);
	/* Make sure QH is updated */
	write_barrier();
	/* Add to the sw list */
	list_append(&ep->eplist_link, &instance->endpoint_list);

	ehci_endpoint_t *first = ehci_endpoint_list_instance(
	    list_first(&instance->endpoint_list));
	usb_log_debug("EPL(%p-%s): EP(%p) added to list, first is %p(%p).",
	    instance, instance->name, ep, first, first->qh);
	if (last_qh == instance->list_head) {
		usb_log_debug2("EPL(%p-%s): head EP(%p-%" PRIxn "): %x:%x.",
		    instance, instance->name, last_qh,
		    addr_to_phys(instance->list_head),
		    last_qh->status, last_qh->horizontal);
	}
	fibril_mutex_unlock(&instance->guard);
}

/** Remove endpoint from the list and queue.
 *
 * @param[in] instance List to use.
 * @param[in] endpoint Endpoint to remove.
 */
void endpoint_list_remove_ep(endpoint_list_t *instance, ehci_endpoint_t *ep)
{
	assert(instance);
	assert(instance->list_head);
	assert(ep);
	assert(ep->qh);

	fibril_mutex_lock(&instance->guard);

	usb_log_debug2("EPL(%p-%s): removing EP(%p).",
	    instance, instance->name, ep);

	const char *qpos = NULL;
	qh_t *prev_qh;
	/* Remove from the hardware queue */
	if (list_first(&instance->endpoint_list) == &ep->eplist_link) {
		/* I'm the first one here */
		prev_qh = instance->list_head;
		qpos = "FIRST";
	} else {
		prev_qh = ehci_endpoint_list_instance(ep->eplist_link.prev)->qh;
		qpos = "NOT FIRST";
	}
	assert(qh_next(prev_qh) == addr_to_phys(ep->qh));
	prev_qh->horizontal = ep->qh->horizontal;
	/* Make sure ED is updated */
	write_barrier();

	usb_log_debug("EPL(%p-%s): EP(%p) removed (%s), horizontal %x.",
	    instance, instance->name,  ep, qpos, ep->qh->horizontal);

	/* Remove from the endpoint list */
	list_remove(&ep->eplist_link);
	fibril_mutex_unlock(&instance->guard);
}
/**
 * @}
 */

