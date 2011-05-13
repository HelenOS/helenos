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
 * @brief Address keeping.
 */
#include <usb/addrkeep.h>
#include <errno.h>
#include <assert.h>

/** For loop over all used addresses in address keeping.
 *
 * @param link Iterator.
 * @param addresses Addresses keeping structure to iterate.
 */
#define for_all_used_addresses(link, addresses) \
	for (link = (addresses)->used_addresses.next; \
	    link != &(addresses)->used_addresses; \
	    link = link->next)

/** Get instance of usb_address_keeping_used_t. */
#define used_address_get_instance(lnk) \
	list_get_instance(lnk, usb_address_keeping_used_t, link)

/** Invalid value of devman handle. */
#define INVALID_DEVMAN_HANDLE \
	((devman_handle_t)-1)

/** Creates structure for used USB address.
 *
 * @param address USB address.
 * @return Initialized structure.
 * @retval NULL Out of memory.
 */
static usb_address_keeping_used_t *usb_address_keeping_used_create(
    usb_address_t address)
{
	usb_address_keeping_used_t *info
	    = malloc(sizeof(usb_address_keeping_used_t));
	if (info == NULL) {
		return NULL;
	}

	info->address = address;
	info->devman_handle = INVALID_DEVMAN_HANDLE;
	list_initialize(&info->link);
	return info;
}

/** Destroys structure for used USB address.
 *
 * @param info Structure to be destroyed.
 */
static void usb_address_keeping_used_destroy(usb_address_keeping_used_t *info)
{
	free(info);
}

/** Find used USB address structure by USB address.
 *
 * It is expected that guard mutex is already locked.
 *
 * @param addresses Address keeping info.
 * @param address Address to be found.
 * @return Structure describing looked for address.
 * @retval NULL Address not found.
 */
static usb_address_keeping_used_t *usb_address_keeping_used_find_no_lock(
    usb_address_keeping_t *addresses, usb_address_t address)
{
	link_t *link;
	for_all_used_addresses(link, addresses) {
		usb_address_keeping_used_t *info
		    = used_address_get_instance(link);

		if (info->address == address) {
			return info;
		}
	}

	return NULL;
}

/** Initialize address keeping structure.
 *
 * @param addresses Address keeping info.
 * @param max_address Maximum USB address (exclusive bound).
 */
void usb_address_keeping_init(usb_address_keeping_t *addresses,
    usb_address_t max_address)
{
	/*
	 * Items related with used addresses.
	 */
	addresses->max_address = max_address;
	list_initialize(&addresses->used_addresses);
	fibril_mutex_initialize(&addresses->used_addresses_guard);
	fibril_condvar_initialize(&addresses->used_addresses_condvar);

	/*
	 * Items related with default address.
	 */
	addresses->default_available = true;
	fibril_condvar_initialize(&addresses->default_condvar);
	fibril_mutex_initialize(&addresses->default_condvar_guard);
}

/** Reserved default USB address.
 *
 * This function blocks until reserved address is available.
 *
 * @see usb_address_keeping_release_default
 *
 * @param addresses Address keeping info.
 */
void usb_address_keeping_reserve_default(usb_address_keeping_t *addresses)
{
	fibril_mutex_lock(&addresses->default_condvar_guard);
	while (!addresses->default_available) {
		fibril_condvar_wait(&addresses->default_condvar,
			&addresses->default_condvar_guard);
	}
	addresses->default_available = false;
	fibril_mutex_unlock(&addresses->default_condvar_guard);
}

/** Releases default USB address.
 *
 * @see usb_address_keeping_reserve_default
 *
 * @param addresses Address keeping info.
 */
void usb_address_keeping_release_default(usb_address_keeping_t *addresses)
{
	fibril_mutex_lock(&addresses->default_condvar_guard);
	addresses->default_available = true;
	fibril_condvar_signal(&addresses->default_condvar);
	fibril_mutex_unlock(&addresses->default_condvar_guard);
}

/** Request free address assignment.
 *
 * This function does not block when there are not free addresses to be
 * assigned.
 *
 * @param addresses Address keeping info.
 * @return USB address that could be used or negative error code.
 * @retval ELIMIT No more addresses to assign.
 * @retval ENOMEM Out of memory.
 */
usb_address_t usb_address_keeping_request(usb_address_keeping_t *addresses)
{
	usb_address_t previous_address = 0;
	usb_address_t free_address = 0;

	fibril_mutex_lock(&addresses->used_addresses_guard);
	link_t *new_address_position;
	if (list_empty(&addresses->used_addresses)) {
		free_address = 1;
		new_address_position = addresses->used_addresses.next;
	} else {
		usb_address_keeping_used_t *first
		    = used_address_get_instance(addresses->used_addresses.next);
		previous_address = first->address;
		
		for_all_used_addresses(new_address_position, addresses) {
			usb_address_keeping_used_t *info
			    = used_address_get_instance(new_address_position);
			if (info->address > previous_address + 1) {
				free_address = previous_address + 1;
				break;
			}
			previous_address = info->address;
		}

		if (free_address == 0) {
			usb_address_keeping_used_t *last
			    = used_address_get_instance(addresses->used_addresses.next);
			free_address = last->address + 1;
		}
	}

	if (free_address >= addresses->max_address) {
		free_address = ELIMIT;
		goto leave;
	}

	usb_address_keeping_used_t *used
	    = usb_address_keeping_used_create(free_address);
	if (used == NULL) {
		free_address = ENOMEM;
		goto leave;
	}

	list_prepend(&used->link, new_address_position);

leave:
	fibril_mutex_unlock(&addresses->used_addresses_guard);

	return free_address;
}

/** Release USB address.
 *
 * @param addresses Address keeping info.
 * @param address Address to be released.
 * @return Error code.
 * @retval ENOENT Address is not in use.
 */
int usb_address_keeping_release(usb_address_keeping_t *addresses,
    usb_address_t address)
{
	int rc = ENOENT;

	fibril_mutex_lock(&addresses->used_addresses_guard);

	usb_address_keeping_used_t *info
	    = usb_address_keeping_used_find_no_lock(addresses, address);

	if (info != NULL) {
		rc = EOK;
		list_remove(&info->link);
		usb_address_keeping_used_destroy(info);
	}

	fibril_mutex_unlock(&addresses->used_addresses_guard);

	return rc;
}

/** Bind devman handle with USB address.
 *
 * When the @p address is invalid (e.g. no such entry), the request
 * is silently ignored.
 *
 * @param addresses Address keeping info.
 * @param address USB address.
 * @param handle Devman handle.
 */
void usb_address_keeping_devman_bind(usb_address_keeping_t *addresses,
    usb_address_t address, devman_handle_t handle)
{
	fibril_mutex_lock(&addresses->used_addresses_guard);

	usb_address_keeping_used_t *info
	    = usb_address_keeping_used_find_no_lock(addresses, address);
	if (info == NULL) {
		goto leave;
	}

	assert(info->address == address);
	info->devman_handle = handle;

	/*
	 * Inform that new handle was added.
	 */
	fibril_condvar_broadcast(&addresses->used_addresses_condvar);

leave:
	fibril_mutex_unlock(&addresses->used_addresses_guard);
}

/** Find address by its devman handle.
 *
 * @param addresses Address keeping info.
 * @param handle Devman handle.
 * @return USB address or negative error code.
 * @retval ENOENT No such address.
 */
static usb_address_t usb_address_keeping_find_no_lock(
    usb_address_keeping_t *addresses, devman_handle_t handle)
{
	usb_address_t address = ENOENT;

	link_t *link;
	for_all_used_addresses(link, addresses) {
		usb_address_keeping_used_t *info
		    = used_address_get_instance(link);

		if (info->devman_handle == handle) {
			address = info->address;
			break;
		}
	}

	return address;
}

/** Find USB address by its devman handle.
 *
 * This function blocks until corresponding address is found.
 *
 * @param addresses Address keeping info.
 * @param handle Devman handle.
 * @return USB address or negative error code.
 */
usb_address_t usb_address_keeping_find(usb_address_keeping_t *addresses,
    devman_handle_t handle)
{
	usb_address_t address = ENOENT;

	fibril_mutex_lock(&addresses->used_addresses_guard);
	while (true) {
		address = usb_address_keeping_find_no_lock(addresses, handle);
		if (address != ENOENT) {
			break;
		}
		fibril_condvar_wait(&addresses->used_addresses_condvar,
		    &addresses->used_addresses_guard);
	}

	fibril_mutex_unlock(&addresses->used_addresses_guard);

	return address;
}

/**
 * @}
 */
