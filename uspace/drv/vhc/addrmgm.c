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
 * @brief Address management.
 */

#include <usb/usb.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <fibril_synch.h>

#include "vhcd.h"
#include "conn.h"

#define ADDRESS_COUNT 100
#define DEFAULT_ADDRESS 0

typedef struct {
	usb_address_t address;
	bool available;
} address_info_t;

static address_info_t dev_address[ADDRESS_COUNT];
fibril_mutex_t address_guard;

typedef struct {
	fibril_mutex_t condvar_guard;
	fibril_condvar_t condvar;
	bool available;
} reservable_address_info_t;

static reservable_address_info_t default_address;

void address_init(void)
{
	usb_address_t i;
	for (i = 0; i < ADDRESS_COUNT; i++) {
		dev_address[i].address = i + 1;
		dev_address[i].available = true;
	}

	fibril_mutex_initialize(&address_guard);
	fibril_mutex_initialize(&default_address.condvar_guard);
	fibril_condvar_initialize(&default_address.condvar);
}

int reserve_default_address(device_t *dev)
{
	fibril_mutex_lock(&default_address.condvar_guard);
	while (!default_address.available) {
		fibril_condvar_wait(&default_address.condvar,
		    &default_address.condvar_guard);
	}
	fibril_mutex_unlock(&default_address.condvar_guard);

	return EOK;
}

int release_default_address(device_t *dev)
{
	fibril_mutex_lock(&default_address.condvar_guard);
	default_address.available = true;
	fibril_condvar_signal(&default_address.condvar);
	fibril_mutex_unlock(&default_address.condvar_guard);

	return EOK;
}

int request_address(device_t *dev, usb_address_t *address)
{
	fibril_mutex_lock(&address_guard);
	usb_address_t i;
	bool found = false;
	for (i = 0; i < ADDRESS_COUNT; i++) {
		if (dev_address[i].available) {
			dev_address[i].available = false;
			*address = dev_address[i].address;
			found = true;
			break;
		}
	}
	fibril_mutex_unlock(&address_guard);

	if (!found) {
		return ELIMIT;
	} else {
		return EOK;
	}
}

int release_address(device_t *dev, usb_address_t address)
{
	if (address == DEFAULT_ADDRESS) {
		return EPERM;
	}

	int rc = EPERM;

	fibril_mutex_lock(&address_guard);
	usb_address_t i;
	for (i = 0; i < ADDRESS_COUNT; i++) {
		if (dev_address[i].address == address) {
			if (dev_address[i].available) {
				rc = ENOENT;
				break;
			}

			dev_address[i].available = true;
			rc = EOK;
			break;
		}
	}
	fibril_mutex_unlock(&address_guard);

	return rc;
}

/**
 * @}
 */
