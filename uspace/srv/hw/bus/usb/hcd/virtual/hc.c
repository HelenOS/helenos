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

#include "vhcd.h"
#include "hc.h"

#define USLEEP_BASE (500 * 1000)

#define USLEEP_VAR 10000

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


typedef struct {
	link_t link;
	usb_target_t target;
	usb_direction_t direction;
	usb_transfer_type_t type;
	void * buffer;
	size_t len;
	hc_transaction_done_callback_t callback;
	void * callback_arg;
} transaction_t;

#define TRANSACTION_FORMAT "T[%d:%d %s %s (%d)]"
#define TRANSACTION_PRINTF(t) \
	(t).target.address, (t).target.endpoint, \
	usb_str_transfer_type((t).type), \
	((t).direction == USB_DIRECTION_IN ? "in" : "out"), \
	(int)(t).len

#define transaction_get_instance(lnk) \
	list_get_instance(lnk, transaction_t, link)

static inline unsigned int pseudo_random(unsigned int *seed)
{
	*seed = ((*seed) * 873511) % 22348977 + 7;
	return ((*seed) >> 8);
}

static void process_transaction_with_outcome(transaction_t * transaction,
    usb_transaction_outcome_t outcome)
{
	dprintf("processing transaction " TRANSACTION_FORMAT ", outcome: %s",
	    TRANSACTION_PRINTF(*transaction),
	    usb_str_transaction_outcome(outcome));
	
	transaction->callback(transaction->buffer, transaction->len, outcome,
	    transaction->callback_arg);
}

static void process_transaction(transaction_t * transaction)
{
	static unsigned int seed = 4089;
	
	unsigned int roulette = pseudo_random(&seed);
	usb_transaction_outcome_t outcome = USB_OUTCOME_OK;
	
	PROB_TEST(outcome, USB_OUTCOME_BABBLE, PROB_OUTCOME_BABBLE, roulette);
	PROB_TEST(outcome, USB_OUTCOME_CRCERROR, PROB_OUTCOME_CRCERROR, roulette);
	
	process_transaction_with_outcome(transaction, outcome);
}

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
		
		link_t * first_transaction_link = transaction_list.next;
		transaction_t * first_transaction
		    = transaction_get_instance(first_transaction_link);
		list_remove(first_transaction_link);
		
		process_transaction(first_transaction);
		
		free(first_transaction);
	}
}

static void hc_add_transaction(usb_transfer_type_t type, usb_target_t target,
    usb_direction_t direction,
    void * buffer, size_t len,
    hc_transaction_done_callback_t callback, void * arg)
{
	transaction_t * transaction = malloc(sizeof(transaction_t));
	
	list_initialize(&transaction->link);
	transaction->type = type;
	transaction->target = target;
	transaction->direction = direction;
	transaction->buffer = buffer;
	transaction->len = len;
	transaction->callback = callback;
	transaction->callback_arg = arg;
	
	dprintf("adding transaction " TRANSACTION_FORMAT,
	    TRANSACTION_PRINTF(*transaction));

	list_append(&transaction->link, &transaction_list);
}

void hc_add_out_transaction(usb_transfer_type_t type, usb_target_t target,
    void * buffer, size_t len,
    hc_transaction_done_callback_t callback, void * arg)
{
	hc_add_transaction(type, target, USB_DIRECTION_OUT,
	    buffer, len, callback, arg);
}

void hc_add_in_transaction(usb_transfer_type_t type, usb_target_t target,
    void * buffer, size_t len,
    hc_transaction_done_callback_t callback, void * arg)
{
	static unsigned int seed = 216;
	
	size_t i;
	char * data = (char *)buffer;
	for (i = 0; i < len; i++, data++) {
		*data = 'A' + (pseudo_random(&seed) % ('Z' - 'A'));
	}
	
	unsigned int shortening = pseudo_random(&seed) % SHORTENING_VAR;
	if (len > shortening) {
		len -= shortening;
	}
	
	hc_add_transaction(type, target, USB_DIRECTION_IN,
	    buffer, len, callback, arg);
}

/**
 * @}
 */
