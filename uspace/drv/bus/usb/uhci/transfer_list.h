/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver transfer list structure
 */

#ifndef DRV_UHCI_TRANSFER_LIST_H
#define DRV_UHCI_TRANSFER_LIST_H

#include <adt/list.h>
#include <fibril_synch.h>

#include "hw_struct/queue_head.h"
#include "uhci_batch.h"
/** Structure maintaining both hw queue and software list
 * of currently executed transfers
 */
typedef struct transfer_list {
	/** Guard against multiple add/remove races */
	fibril_mutex_t guard;
	/** UHCI hw structure representing this queue */
	qh_t *queue_head;
	/** Assigned name, for nicer debug output */
	const char *name;
	/** List of all batches in this list */
	list_t batch_list;
} transfer_list_t;

void transfer_list_fini(transfer_list_t *);
errno_t transfer_list_init(transfer_list_t *, const char *);
void transfer_list_set_next(transfer_list_t *, transfer_list_t *);
errno_t transfer_list_add_batch(transfer_list_t *, uhci_transfer_batch_t *);
void transfer_list_remove_batch(transfer_list_t *, uhci_transfer_batch_t *);
void transfer_list_check_finished(transfer_list_t *);
void transfer_list_abort_all(transfer_list_t *);

#endif

/**
 * @}
 */
