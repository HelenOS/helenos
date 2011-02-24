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

/** @addtogroup libusb
 * @{
 */
/** @file
 * USB address keeping for host controller drivers.
 */
#ifndef LIBUSB_ADDRKEEP_H_
#define LIBUSB_ADDRKEEP_H_

#include <usb/usb.h>
#include <fibril_synch.h>
#include <devman.h>

/** Info about used address. */
typedef struct {
	/** Linked list member. */
	link_t link;
	/** Address. */
	usb_address_t address;
	/** Corresponding devman handle. */
	devman_handle_t devman_handle;
} usb_address_keeping_used_t;

/** Structure for keeping track of free and used USB addresses. */
typedef struct {
	/** Head of list of used addresses. */
	link_t used_addresses;
	/** Upper bound for USB addresses. */
	usb_address_t max_address;
	/** Mutex protecting used address. */
	fibril_mutex_t used_addresses_guard;
	/** Condition variable for used addresses. */
	fibril_condvar_t used_addresses_condvar;

	/** Condition variable mutex for default address. */
	fibril_mutex_t default_condvar_guard;
	/** Condition variable for default address. */
	fibril_condvar_t default_condvar;
	/** Whether is default address available. */
	bool default_available;
} usb_address_keeping_t;

void usb_address_keeping_init(usb_address_keeping_t *, usb_address_t);

void usb_address_keeping_reserve_default(usb_address_keeping_t *);
void usb_address_keeping_release_default(usb_address_keeping_t *);

usb_address_t usb_address_keeping_request(usb_address_keeping_t *);
int usb_address_keeping_release(usb_address_keeping_t *, usb_address_t);
void usb_address_keeping_devman_bind(usb_address_keeping_t *, usb_address_t,
    devman_handle_t);
usb_address_t usb_address_keeping_find(usb_address_keeping_t *,
    devman_handle_t);

#endif
/**
 * @}
 */
