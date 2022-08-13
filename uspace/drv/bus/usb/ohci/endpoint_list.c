/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver transfer list implementation
 */

#include <assert.h>
#include <errno.h>
#include <barrier.h>

#include <usb/debug.h>
#include <usb/host/utils/malloc32.h>

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
	instance->list_head = malloc32(sizeof(ed_t));
	if (!instance->list_head) {
		usb_log_error("Failed to allocate list head.");
		return ENOMEM;
	}
	instance->list_head_pa = addr_to_phys(instance->list_head);
	usb_log_debug2("Transfer list %s setup with ED: %p(0x%0" PRIx32 ")).",
	    name, instance->list_head, instance->list_head_pa);

	ed_init(instance->list_head, NULL, NULL);
	list_initialize(&instance->endpoint_list);
	fibril_mutex_initialize(&instance->guard);
	return EOK;
}

/** Set the next list in transfer list chain.
 *
 * @param[in] instance List to lead.
 * @param[in] next List to append.
 *
 * Does not check whether this replaces an existing list.
 */
void endpoint_list_set_next(
    const endpoint_list_t *instance, const endpoint_list_t *next)
{
	assert(instance);
	assert(next);
	ed_append_ed(instance->list_head, next->list_head);
}

/** Add endpoint to the list and queue.
 *
 * @param[in] instance List to use.
 * @param[in] endpoint Endpoint to add.
 *
 * The endpoint is added to the end of the list and queue.
 */
void endpoint_list_add_ep(endpoint_list_t *instance, ohci_endpoint_t *ep)
{
	assert(instance);
	assert(ep);
	usb_log_debug2("Queue %s: Adding endpoint(%p).", instance->name, ep);

	fibril_mutex_lock(&instance->guard);

	ed_t *last_ed = NULL;
	/* Add to the hardware queue. */
	if (list_empty(&instance->endpoint_list)) {
		/* There are no active EDs */
		last_ed = instance->list_head;
	} else {
		/* There are active EDs, get the last one */
		ohci_endpoint_t *last = list_get_instance(
		    list_last(&instance->endpoint_list), ohci_endpoint_t, eplist_link);
		last_ed = last->ed;
	}
	/* Keep link */
	ep->ed->next = last_ed->next;
	/* Make sure ED is written to the memory */
	write_barrier();

	/* Add ed to the hw queue */
	ed_append_ed(last_ed, ep->ed);
	/* Make sure ED is updated */
	write_barrier();

	/* Add to the sw list */
	list_append(&ep->eplist_link, &instance->endpoint_list);

	ohci_endpoint_t *first = list_get_instance(
	    list_first(&instance->endpoint_list), ohci_endpoint_t, eplist_link);
	usb_log_debug("HCD EP(%p) added to list %s, first is %p(%p).",
	    ep, instance->name, first, first->ed);
	if (last_ed == instance->list_head) {
		usb_log_debug2("%s head ED(%p-0x%0" PRIx32 "): %x:%x:%x:%x.",
		    instance->name, last_ed, instance->list_head_pa,
		    last_ed->status, last_ed->td_tail, last_ed->td_head,
		    last_ed->next);
	}
	fibril_mutex_unlock(&instance->guard);
}

/** Remove endpoint from the list and queue.
 *
 * @param[in] instance List to use.
 * @param[in] endpoint Endpoint to remove.
 */
void endpoint_list_remove_ep(endpoint_list_t *instance, ohci_endpoint_t *ep)
{
	assert(instance);
	assert(instance->list_head);
	assert(ep);
	assert(ep->ed);

	fibril_mutex_lock(&instance->guard);

	usb_log_debug2("Queue %s: removing endpoint(%p).", instance->name, ep);

	const char *qpos = NULL;
	ed_t *prev_ed;
	/* Remove from the hardware queue */
	if (list_first(&instance->endpoint_list) == &ep->eplist_link) {
		/* I'm the first one here */
		prev_ed = instance->list_head;
		qpos = "FIRST";
	} else {
		ohci_endpoint_t *prev =
		    list_get_instance(ep->eplist_link.prev, ohci_endpoint_t, eplist_link);
		prev_ed = prev->ed;
		qpos = "NOT FIRST";
	}
	assert(ed_next(prev_ed) == addr_to_phys(ep->ed));
	prev_ed->next = ep->ed->next;
	/* Make sure ED is updated */
	write_barrier();

	usb_log_debug("HCD EP(%p) removed (%s) from %s, next %x.",
	    ep, qpos, instance->name, ep->ed->next);

	/* Remove from the endpoint list */
	list_remove(&ep->eplist_link);
	fibril_mutex_unlock(&instance->guard);
}
/**
 * @}
 */
