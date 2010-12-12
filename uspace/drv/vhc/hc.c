/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usb
 * @{
 */
/** @file
 * @brief Virtual HC (implementation).
 */

#include <ipc/ipc.h>
#include <adt/list.h>
#include <bool.h>
#include <async.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>

#include <usbvirt/hub.h>

#include "vhcd.h"
#include "hc.h"
#include "devices.h"
#include "hub.h"

#define USLEEP_BASE (0 * 500 * 1000)

#define USLEEP_VAR 5000

#define SHORTENING_VAR 15

#define PROB_OUTCOME_BABBLE 5
#define PROB_OUTCOME_CRCERROR 7

#define PROB_TEST(var, new_value, prob, number) \
	do { \
		if (((number) % (prob)) == 0) { \
			var = (new_value); \
		} \
	} while (0)

static link_t transaction_list;

#define TRANSACTION_FORMAT "T[%d:%d %s (%d)]"
#define TRANSACTION_PRINTF(t) \
	(t).target.address, (t).target.endpoint, \
	usbvirt_str_transaction_type((t).type), \
	(int)(t).len

#define transaction_get_instance(lnk) \
	list_get_instance(lnk, transaction_t, link)

static inline unsigned int pseudo_random(unsigned int *seed)
{
	*seed = ((*seed) * 873511) % 22348977 + 7;
	return ((*seed) >> 8);
}

/** Call transaction callback.
 * Calling this callback informs the backend that transaction was processed.
 */
static void process_transaction_with_outcome(transaction_t * transaction,
    usb_transaction_outcome_t outcome)
{
	dprintf(3, "processing transaction " TRANSACTION_FORMAT ", outcome: %s",
	    TRANSACTION_PRINTF(*transaction),
	    usb_str_transaction_outcome(outcome));
	
	transaction->callback(transaction->buffer, transaction->len, outcome,
	    transaction->callback_arg);
}

/** Host controller manager main function.
 */
void hc_manager(void)
{
	list_initialize(&transaction_list);
	
	static unsigned int seed = 4573;
	
	printf("%s: transaction processor ready.\n", NAME);
	
	while (true) {
		async_usleep(USLEEP_BASE + (pseudo_random(&seed) % USLEEP_VAR));
		
		if (list_empty(&transaction_list)) {
			continue;
		}
		
		char ports[HUB_PORT_COUNT + 2];
		hub_get_port_statuses(ports, HUB_PORT_COUNT + 1);
		dprintf(0, "virtual hub: addr=%d ports=%s",
		    virthub_dev.address, ports);
		
		link_t *first_transaction_link = transaction_list.next;
		transaction_t *transaction
		    = transaction_get_instance(first_transaction_link);
		list_remove(first_transaction_link);
		
		dprintf(3, "processing transaction " TRANSACTION_FORMAT "",
		    TRANSACTION_PRINTF(*transaction));
		
		usb_transaction_outcome_t outcome;
		outcome = virtdev_send_to_all(transaction);
		
		process_transaction_with_outcome(transaction, outcome);

		free(transaction);
	}
}

/** Create new transaction
 */
static transaction_t *transaction_create(usbvirt_transaction_type_t type,
    usb_target_t target,
    void * buffer, size_t len,
    hc_transaction_done_callback_t callback, void * arg)
{
	transaction_t * transaction = malloc(sizeof(transaction_t));
	
	list_initialize(&transaction->link);
	transaction->type = type;
	transaction->target = target;
	transaction->buffer = buffer;
	transaction->len = len;
	transaction->callback = callback;
	transaction->callback_arg = arg;
	
	dprintf(3, "creating transaction " TRANSACTION_FORMAT,
	    TRANSACTION_PRINTF(*transaction));
	
	return transaction;
}

/** Add transaction directioned towards the device.
 */
void hc_add_transaction_to_device(bool setup, usb_target_t target,
    void * buffer, size_t len,
    hc_transaction_done_callback_t callback, void * arg)
{
	transaction_t *transaction = transaction_create(
	    setup ? USBVIRT_TRANSACTION_SETUP : USBVIRT_TRANSACTION_OUT, target,
	    buffer, len, callback, arg);
	list_append(&transaction->link, &transaction_list);
}

/** Add transaction directioned from the device.
 */
void hc_add_transaction_from_device(usb_target_t target,
    void * buffer, size_t len,
    hc_transaction_done_callback_t callback, void * arg)
{
	transaction_t *transaction = transaction_create(USBVIRT_TRANSACTION_IN,
	    target, buffer, len, callback, arg);
	list_append(&transaction->link, &transaction_list);
}

/**
 * @}
 */
