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
	usb_transfer_type_t transfer_type;
	size_t max_packet_size;
	size_t size;
} __attribute__((aligned (sizeof(unsigned long)))) transfer_t;
/*----------------------------------------------------------------------------*/
typedef struct {
	transfer_t transfer;
	link_t link;
	bool used;
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
static int trans_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	assert(item);
	transfer_status_t *status =
	    hash_table_get_instance(item, transfer_status_t, link);
	const size_t bytes =
	    keys < MAX_KEYS ? keys * sizeof(unsigned long) : sizeof(transfer_t);
	return bcmp(key, &status->transfer, bytes);
}
/*----------------------------------------------------------------------------*/
static void dummy(link_t *item) {}
/*----------------------------------------------------------------------------*/
hash_table_operations_t op = {
	.hash = transfer_hash,
	.compare = trans_compare,
	.remove_callback = dummy,
};
/*----------------------------------------------------------------------------*/
int bandwidth_init(bandwidth_t *instance)
{
	assert(instance);
	fibril_mutex_initialize(&instance->guard);
	return
	    hash_table_create(&instance->reserved, BUCKET_COUNT, MAX_KEYS, &op);
}
/*----------------------------------------------------------------------------*/
void bandwidth_destroy(bandwidth_t *instance)
{
	hash_table_destroy(&instance->reserved);
}
/*----------------------------------------------------------------------------*/
int bandwidth_reserve(bandwidth_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_transfer_type_t transfer_type,
    size_t max_packet_size, size_t size, unsigned interval)
{
	assert(instance);
	transfer_t trans = {
		.address = address,
		.endpoint = endpoint,
		.transfer_type = transfer_type,
		.max_packet_size = max_packet_size,
		.size = size,
	};
	fibril_mutex_lock(&instance->guard);
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
	status->used = false;
	link_initialize(&status->link);

	hash_table_insert(&instance->reserved,
	    (unsigned long*)&status->transfer, &status->link);
	fibril_mutex_unlock(&instance->guard);
	return EOK;
	/* TODO: compute bandwidth used */
}
/*----------------------------------------------------------------------------*/
int bandwidth_release(bandwidth_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_transfer_type_t transfer_type,
    size_t max_packet_size, size_t size, unsigned interval)
{
	assert(instance);
	transfer_t trans = {
		.address = address,
		.endpoint = endpoint,
		.transfer_type = transfer_type,
		.max_packet_size = max_packet_size,
		.size = size,
	};
	fibril_mutex_lock(&instance->guard);
	link_t *item =
	    hash_table_find(&instance->reserved, (unsigned long*)&trans);
	if (item == NULL) {
		fibril_mutex_unlock(&instance->guard);
		return EINVAL;
	}

	hash_table_remove(&instance->reserved,
	    (unsigned long*)&trans, MAX_KEYS);

	fibril_mutex_unlock(&instance->guard);
	return EOK;
	/* TODO: compute bandwidth freed */
}
/*----------------------------------------------------------------------------*/
int bandwidth_use(bandwidth_t *instance, usb_address_t address,
    usb_endpoint_t endpoint, usb_transfer_type_t transfer_type,
    size_t max_packet_size, size_t size, unsigned interval)
{
	assert(instance);
	transfer_t trans = {
		.address = address,
		.endpoint = endpoint,
		.transfer_type = transfer_type,
		.max_packet_size = max_packet_size,
		.size = size,
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
    usb_endpoint_t endpoint, usb_transfer_type_t transfer_type,
    size_t max_packet_size, size_t size, unsigned interval)
{
	assert(instance);
	transfer_t trans = {
		.address = address,
		.endpoint = endpoint,
		.transfer_type = transfer_type,
		.max_packet_size = max_packet_size,
		.size = size,
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
