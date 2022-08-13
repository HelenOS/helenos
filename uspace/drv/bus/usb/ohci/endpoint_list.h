/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver transfer list structure
 */
#ifndef DRV_OHCI_ENDPOINT_LIST_H
#define DRV_OHCI_ENDPOINT_LIST_H

#include <adt/list.h>
#include <assert.h>
#include <fibril_synch.h>
#include <stdint.h>
#include <usb/host/utils/malloc32.h>

#include "ohci_bus.h"
#include "hw_struct/endpoint_descriptor.h"

/** Structure maintains both OHCI queue and software list of active endpoints. */
typedef struct endpoint_list {
	/** Guard against add/remove races */
	fibril_mutex_t guard;
	/** OHCI hw structure at the beginning of the queue */
	ed_t *list_head;
	/** Physical address of the first(dummy) ED */
	uint32_t list_head_pa;
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
	free32(instance->list_head);
	instance->list_head = NULL;
}

errno_t endpoint_list_init(endpoint_list_t *instance, const char *name);
void endpoint_list_set_next(
    const endpoint_list_t *instance, const endpoint_list_t *next);
void endpoint_list_add_ep(endpoint_list_t *instance, ohci_endpoint_t *ep);
void endpoint_list_remove_ep(endpoint_list_t *instance, ohci_endpoint_t *ep);

#endif

/**
 * @}
 */
