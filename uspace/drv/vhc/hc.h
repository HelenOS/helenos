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
 * @brief Virtual HC.
 */
#ifndef VHCD_HC_H_
#define VHCD_HC_H_

#include <usb/usb.h>
#include <usbvirt/hub.h>

/** Callback after transaction is sent to USB.
 *
 * @param buffer Transaction data buffer.
 * @param size Transaction data size.
 * @param outcome Transaction outcome.
 * @param arg Custom argument.
 */
typedef void (*hc_transaction_done_callback_t)(void *buffer, size_t size,
    usb_transaction_outcome_t outcome, void *arg);

/** Pending transaction details. */
typedef struct {
	/** Linked-list link. */
	link_t link;
	/** Transaction type. */
	usbvirt_transaction_type_t type;
	/** Transfer type. */
	usb_transfer_type_t transfer_type;
	/** Device address. */
	usb_target_t target;
	/** Direction of the transaction. */
	usb_direction_t direction;
	/** Transaction data buffer. */
	void * buffer;
	/** Transaction data length. */
	size_t len;
	/** Callback after transaction is done. */
	hc_transaction_done_callback_t callback;
	/** Argument to the callback. */
	void * callback_arg;
} transaction_t;

void hc_manager(void);

void hc_add_transaction_to_device(bool setup,
    usb_target_t target, usb_transfer_type_t transfer_type,
    void * buffer, size_t len,
    hc_transaction_done_callback_t callback, void * arg);

void hc_add_transaction_from_device(
    usb_target_t target, usb_transfer_type_t transfer_type,
    void * buffer, size_t len,
    hc_transaction_done_callback_t callback, void * arg);


#endif
/**
 * @}
 */
