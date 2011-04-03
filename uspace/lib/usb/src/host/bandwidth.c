/*
 * Copyright (c) 2011 Jan Vesely
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

#include <assert.h>
#include <errno.h>
#include <usb/host/bandwidth.h>

typedef struct {
	usb_address_t address;
	usb_endpoint_t endpoint;
	usb_direction_t direction;
} __attribute__((aligned (sizeof(unsigned long)))) transfer_t;
/*----------------------------------------------------------------------------*/
typedef struct {
	transfer_t transfer;
	link_t link;
	bool used;
	size_t required;
} transfer_status_t;
/*----------------------------------------------------------------------------*/
#define BUCKET_COUNT 7
#define MAX_KEYS (sizeof(transfer_t) / sizeof(unsigned long))
/*----------------------------------------------------------------------------*/
static hash_index_t transfer_hash(unsigned long key[])
{
	hash_index_t hash = 0;
	unsigned i = 0;
	for (;i < MAX_KEYS; ++i) {
		hash ^= key[i];
	}
	hash %= BUCKET_COUNT;
	return hash;
}
/*----------------------------------------------------------------------------*/
static int transfer_compare(
    unsigned long key[], hash_count_t keys, link_t *item)
{
	assert(item);
	transfer_status_t *status =
	    hash_table_get_instance(item, transfer_status_t, link);
	const size_t bytes =
	    keys < MAX_KEYS ? keys * sizeof(unsigned long) : sizeof(transfer_t);
	return bcmp(key, &status->transfer, bytes);
}
/*----------------------------------------------------------------------------*/
static void transfer_remove(link_t *item)
{
	assert(item);
	transfer_status_t *status =
	    hash_table_get_instance(item, transfer_status_t, link);
	assert(status);
	free(status);
}
/*----------------------------------------------------------------------------*/
hash_table_operations_t op = {
	.hash = transfer_hash,
	.compare = transfer_compare,
	.remove_callback = transfer_remove,
};
/*----------------------------------------------------------------------------*/
size_t bandwidth_count_usb11(usb_speed_t speed, usb_transfer_type_t type,
    size_t size, size_t max_packet_size)
{
	const unsigned packet_count =
	    (size + max_packet_size - 1) / max_packet_size;
	/* TODO: It may be that ISO and INT transfers use only one data packet
	 * per transaction, but I did not find text in UB spec that confirms
	 * this */
	/* NOTE: All data packets will be considered to be max_packet_size */
	switch (speed)
	{
	case USB_SPEED_LOW:
		assert(type == USB_TRANSFER_INTERRUPT);
		/* Protocol overhead 13B
		 * (3 SYNC bytes, 3 PID bytes, 2 Endpoint + CRC bytes, 2
		 * CRC bytes, and a 3-byte interpacket delay)
		 * see USB spec page 45-46. */
		/* Speed penalty 8: low speed is 8-times slower*/
		return packet_count * (13 + max_packet_size) * 8;
	case USB_SPEED_FULL:
		/* Interrupt transfer overhead see above
		 * or page 45 of USB spec */
		if (type == USB_TRANSFER_INTERRUPT)
			return packet_count * (13 + max_packet_size);

		assert(type == USB_TRANSFER_ISOCHRONOUS);
		/* Protocol overhead 9B
		 * (2 SYNC bytes, 2 PID bytes, 2 Endpoint + CRC bytes, 2 CRC
		 * bytes, and a 1-byte interpacket delay)
		 * see USB spec page 42 */
		return packet_count * (9 + max_packet_size);
	default:
		return 0;
	}
}
/*----------------------------------------------------------------------------*/
int bandwidth_init(bandwidth_t *instance, size_t bandwidth,
    size_t (*usage_fnc)(usb_speed_t, usb_transfer_type_t, size_t, size_t))
{
	assert(instance);
	fibril_mutex_initialize(&instance->guard);
	instance->free = bandwidth;
	instance->usage_fnc = usage_fnc;
	bool ht =
	    hash_table_create(&instance->reserved, BUCKET_COUNT, MAX_KEYS, &op);
	return ht ? EOK : ENOMEM;
}
/*----------------------------------------------------------------------------*/
void bandwidth_destroy(bandwidth_t *instance)
{
	hash_table_destroy(&instance->reserved);
}
/*----------------------------------------------------------------------------*/
int bandwidth_reserve(bandwidth_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction, usb_speed_t speed,
    usb_transfer_type_t transfer_type, size_t max_packet_size, size_t size,
    unsigned interval)
{
	if (transfer_type != USB_TRANSFER_ISOCHRONOUS &&
	    transfer_type != USB_TRANSFER_INTERRUPT) {
		return ENOTSUP;
	}

	assert(instance);
	assert(instance->usage_fnc);

	transfer_t trans = {
		.address = address,
		.endpoint = endpoint,
		.direction = direction,
	};
	fibril_mutex_lock(&instance->guard);
	const size_t required =
	    instance->usage_fnc(speed, transfer_type, size, max_packet_size);

	if (required > instance->free) {
		fibril_mutex_unlock(&instance->guard);
		return ENOSPC;
	}

	link_t *item =
	    hash_table_find(&instance->reserved, (unsigned long*)&trans);
	if (item != NULL) {
		fibril_mutex_unlock(&instance->guard);
		return EEXISTS;
	}

	transfer_status_t *status = malloc(sizeof(transfer_status_t));
	if (status == NULL) {
		fibril_mutex_unlock(&instance->guard);
		return ENOMEM;
	}

	status->transfer = trans;
	status->required = required;
	status->used = false;
	link_initialize(&status->link);

	hash_table_insert(&instance->reserved,
	    (unsigned long*)&status->transfer, &status->link);
	instance->free -= required;
	fibril_mutex_unlock(&instance->guard);
	return EOK;
	/* TODO: compute bandwidth used */
}
/*----------------------------------------------------------------------------*/
int bandwidth_release(bandwidth_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(instance);
	transfer_t trans = {
		.address = address,
		.endpoint = endpoint,
		.direction = direction,
	};
	fibril_mutex_lock(&instance->guard);
	link_t *item =
	    hash_table_find(&instance->reserved, (unsigned long*)&trans);
	if (item == NULL) {
		fibril_mutex_unlock(&instance->guard);
		return EINVAL;
	}

	transfer_status_t *status =
	    hash_table_get_instance(item, transfer_status_t, link);

	instance->free += status->required;

	hash_table_remove(&instance->reserved,
	    (unsigned long*)&trans, MAX_KEYS);

	fibril_mutex_unlock(&instance->guard);
	return EOK;
	/* TODO: compute bandwidth freed */
}
/*----------------------------------------------------------------------------*/
int bandwidth_use(bandwidth_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(instance);
	transfer_t trans = {
		.address = address,
		.endpoint = endpoint,
		.direction = direction,
	};
	fibril_mutex_lock(&instance->guard);
	link_t *item =
	    hash_table_find(&instance->reserved, (unsigned long*)&trans);
	int ret = EOK;
	if (item != NULL) {
		transfer_status_t *status =
		    hash_table_get_instance(item, transfer_status_t, link);
		assert(status);
		if (status->used) {
			ret = EINPROGRESS;
		}
		status->used = true;
	} else {
		ret = EINVAL;
	}
	fibril_mutex_unlock(&instance->guard);
	return ret;
}
/*----------------------------------------------------------------------------*/
int bandwidth_free(bandwidth_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(instance);
	transfer_t trans = {
		.address = address,
		.endpoint = endpoint,
		.direction = direction,
	};
	fibril_mutex_lock(&instance->guard);
	link_t *item =
	    hash_table_find(&instance->reserved, (unsigned long*)&trans);
	int ret = EOK;
	if (item != NULL) {
		transfer_status_t *status =
		    hash_table_get_instance(item, transfer_status_t, link);
		assert(status);
		if (!status->used) {
			ret = ENOENT;
		}
		status->used = false;
	} else {
		ret = EINVAL;
	}
	fibril_mutex_unlock(&instance->guard);
	return ret;
}
