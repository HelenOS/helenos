/*
 * Copyright (c) 2011 Jan Vesely
 * All rights eps.
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

#include <bool.h>
#include <assert.h>
#include <errno.h>

#include <usb/host/usb_endpoint_manager.h>

#define BUCKET_COUNT 7

#define MAX_KEYS (3)
typedef struct {
	link_t link;
	size_t bw;
	endpoint_t *ep;
} node_t;
/*----------------------------------------------------------------------------*/
static hash_index_t node_hash(unsigned long key[])
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
static int node_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	assert(item);
	node_t *node = hash_table_get_instance(item, node_t, link);
	assert(node);
	assert(node->ep);
	bool match = true;
	switch (keys) {
	case 3:
		match = match && (key[2] == node->ep->direction);
	case 2:
		match = match && (key[1] == (unsigned long)node->ep->endpoint);
	case 1:
		match = match && (key[0] == (unsigned long)node->ep->address);
		break;
	default:
		match = false;
	}
	return match;
}
/*----------------------------------------------------------------------------*/
static void node_remove(link_t *item)
{
	assert(item);
	node_t *node = hash_table_get_instance(item, node_t, link);
	endpoint_destroy(node->ep);
	free(node);
}
/*----------------------------------------------------------------------------*/
static hash_table_operations_t op = {
	.hash = node_hash,
	.compare = node_compare,
	.remove_callback = node_remove,
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
int usb_endpoint_manager_init(usb_endpoint_manager_t *instance,
    size_t available_bandwidth)
{
	assert(instance);
	fibril_mutex_initialize(&instance->guard);
	fibril_condvar_initialize(&instance->change);
	instance->free_bw = available_bandwidth;
	bool ht =
	    hash_table_create(&instance->ep_table, BUCKET_COUNT, MAX_KEYS, &op);
	return ht ? EOK : ENOMEM;
}
/*----------------------------------------------------------------------------*/
void usb_endpoint_manager_destroy(usb_endpoint_manager_t *instance)
{
	hash_table_destroy(&instance->ep_table);
}
/*----------------------------------------------------------------------------*/
int usb_endpoint_manager_register_ep(usb_endpoint_manager_t *instance,
    endpoint_t *ep, size_t data_size)
{
	assert(ep);
	size_t bw = bandwidth_count_usb11(ep->speed, ep->transfer_type,
	    data_size, ep->max_packet_size);
	assert(instance);

	unsigned long key[MAX_KEYS] =
	    {ep->address, ep->endpoint, ep->direction};
	fibril_mutex_lock(&instance->guard);

	link_t *item =
	    hash_table_find(&instance->ep_table, key);
	if (item != NULL) {
		fibril_mutex_unlock(&instance->guard);
		return EEXISTS;
	}

	if (bw > instance->free_bw) {
		fibril_mutex_unlock(&instance->guard);
		return ENOSPC;
	}

	node_t *node = malloc(sizeof(node_t));
	if (node == NULL) {
		fibril_mutex_unlock(&instance->guard);
		return ENOMEM;
	}

	node->bw = bw;
	node->ep = ep;
	link_initialize(&node->link);

	hash_table_insert(&instance->ep_table, key, &node->link);
	instance->free_bw -= bw;
	fibril_mutex_unlock(&instance->guard);
	fibril_condvar_broadcast(&instance->change);
	return EOK;
}
/*----------------------------------------------------------------------------*/
int usb_endpoint_manager_unregister_ep(usb_endpoint_manager_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction)
{
	assert(instance);
	unsigned long key[MAX_KEYS] = {address, endpoint, direction};

	fibril_mutex_lock(&instance->guard);
	link_t *item = hash_table_find(&instance->ep_table, key);
	if (item == NULL) {
		fibril_mutex_unlock(&instance->guard);
		return EINVAL;
	}

	node_t *node = hash_table_get_instance(item, node_t, link);
	instance->free_bw += node->bw;
	hash_table_remove(&instance->ep_table, key, MAX_KEYS);

	fibril_mutex_unlock(&instance->guard);
	fibril_condvar_broadcast(&instance->change);
	return EOK;
}
/*----------------------------------------------------------------------------*/
endpoint_t * usb_endpoint_manager_get_ep(usb_endpoint_manager_t *instance,
    usb_address_t address, usb_endpoint_t endpoint, usb_direction_t direction,
    size_t *bw)
{
	assert(instance);
	unsigned long key[MAX_KEYS] = {address, endpoint, direction};

	fibril_mutex_lock(&instance->guard);
	link_t *item = hash_table_find(&instance->ep_table, key);
	if (item == NULL) {
		fibril_mutex_unlock(&instance->guard);
		return NULL;
	}
	node_t *node = hash_table_get_instance(item, node_t, link);
	if (bw)
		*bw = node->bw;

	fibril_mutex_unlock(&instance->guard);
	return node->ep;
}
