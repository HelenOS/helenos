/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup libc
 *  @{
 */

/** @file
 *  Packet map and queue implementation.
 *  This file has to be compiled with both the packet server and the client.
 */

#include <assert.h>
#include <malloc.h>
#include <mem.h>
#include <fibril_synch.h>
#include <unistd.h>
#include <errno.h>

#include <sys/mman.h>

#include <adt/hash_table.h>
#include <net/packet.h>
#include <net/packet_header.h>

/** Packet hash table size. */
#define PACKET_HASH_TABLE_SIZE  128

/** Packet map global data. */
static struct {
	/** Safety lock. */
	fibril_rwlock_t lock;
	/** Packet map. */
	hash_table_t packet_map;
	/** Packet map operations */
	hash_table_operations_t operations;
} pm_globals;

typedef struct {
	link_t link;
	packet_t *packet;
} pm_entry_t;

/**
 * Hash function for the packet mapping hash table
 */
static hash_index_t pm_hash(unsigned long key[])
{
	return (hash_index_t) key[0] % PACKET_HASH_TABLE_SIZE;
}

/**
 * Key compare function for the packet mapping hash table
 */
static int pm_compare(unsigned long key[], hash_count_t keys, link_t *link)
{
	pm_entry_t *entry = list_get_instance(link, pm_entry_t, link);
	return entry->packet->packet_id == key[0];
}

/**
 * Remove callback for the packet mapping hash table
 */
static void pm_remove_callback(link_t *link)
{
	pm_entry_t *entry = list_get_instance(link, pm_entry_t, link);
	free(entry);
}

/**
 * Wrapper used when destroying the whole table
 */
static void pm_free_wrapper(link_t *link, void *ignored)
{
	pm_entry_t *entry = list_get_instance(link, pm_entry_t, link);
	free(entry);
}


/** Initializes the packet map.
 *
 * @return		EOK on success.
 * @return		ENOMEM if there is not enough memory left.
 */
int pm_init(void)
{
	int rc = EOK;

	fibril_rwlock_initialize(&pm_globals.lock);
	
	fibril_rwlock_write_lock(&pm_globals.lock);
	
	pm_globals.operations.hash = pm_hash;
	pm_globals.operations.compare = pm_compare;
	pm_globals.operations.remove_callback = pm_remove_callback;

	if (!hash_table_create(&pm_globals.packet_map, PACKET_HASH_TABLE_SIZE, 1,
	    &pm_globals.operations))
		rc = ENOMEM;
	
	fibril_rwlock_write_unlock(&pm_globals.lock);
	
	return rc;
}

/** Finds the packet mapping.
 *
 * @param[in] packet_id Packet identifier to be found.
 *
 * @return The found packet reference.
 * @return NULL if the mapping does not exist.
 *
 */
packet_t *pm_find(packet_id_t packet_id)
{
	if (!packet_id)
		return NULL;
	
	fibril_rwlock_read_lock(&pm_globals.lock);
	
	unsigned long key = packet_id;
	link_t *link = hash_table_find(&pm_globals.packet_map, &key);
	
	packet_t *packet;
	if (link != NULL) {
		pm_entry_t *entry =
		    hash_table_get_instance(link, pm_entry_t, link);
		packet = entry->packet;
	} else
		packet = NULL;
	
	fibril_rwlock_read_unlock(&pm_globals.lock);
	return packet;
}

/** Adds the packet mapping.
 *
 * @param[in] packet Packet to be remembered.
 *
 * @return EOK on success.
 * @return EINVAL if the packet is not valid.
 * @return ENOMEM if there is not enough memory left.
 *
 */
int pm_add(packet_t *packet)
{
	if (!packet_is_valid(packet))
		return EINVAL;
	
	fibril_rwlock_write_lock(&pm_globals.lock);
	
	pm_entry_t *entry = malloc(sizeof(pm_entry_t));
	if (entry == NULL) {
		fibril_rwlock_write_unlock(&pm_globals.lock);
		return ENOMEM;
	}
	
	entry->packet = packet;
	
	unsigned long key = packet->packet_id;
	hash_table_insert(&pm_globals.packet_map, &key, &entry->link);
	
	fibril_rwlock_write_unlock(&pm_globals.lock);
	
	return EOK;
}

/** Remove the packet mapping
 *
 * @param[in] packet The packet to be removed
 *
 */
void pm_remove(packet_t *packet)
{
	assert(packet_is_valid(packet));
	
	fibril_rwlock_write_lock(&pm_globals.lock);
	
	unsigned long key = packet->packet_id;
	hash_table_remove(&pm_globals.packet_map, &key, 1);
	
	fibril_rwlock_write_unlock(&pm_globals.lock);
}

/** Release the packet map. */
void pm_destroy(void)
{
	fibril_rwlock_write_lock(&pm_globals.lock);
	hash_table_apply(&pm_globals.packet_map, pm_free_wrapper, NULL);
	hash_table_destroy(&pm_globals.packet_map);
	/* Leave locked */
}

/** Add packet to the sorted queue.
 *
 * The queue is sorted in the ascending order.
 * The packet is inserted right before the packets of the same order value.
 *
 * @param[in,out] first First packet of the queue. Sets the first
 *                      packet of the queue. The original first packet
 *                      may be shifted by the new packet.
 * @param[in] packet    Packet to be added.
 * @param[in] order     Packet order value.
 * @param[in] metric    Metric value of the packet.
 *
 * @return EOK on success.
 * @return EINVAL if the first parameter is NULL.
 * @return EINVAL if the packet is not valid.
 *
 */
int pq_add(packet_t **first, packet_t *packet, size_t order, size_t metric)
{
	if ((!first) || (!packet_is_valid(packet)))
		return EINVAL;
	
	pq_set_order(packet, order, metric);
	if (packet_is_valid(*first)) {
		packet_t *cur = *first;
		
		do {
			if (cur->order < order) {
				if (cur->next)
					cur = pm_find(cur->next);
				else {
					cur->next = packet->packet_id;
					packet->previous = cur->packet_id;
					
					return EOK;
				}
			} else {
				packet->previous = cur->previous;
				packet->next = cur->packet_id;
				
				cur->previous = packet->packet_id;
				cur = pm_find(packet->previous);
				
				if (cur)
					cur->next = packet->packet_id;
				else
					*first = packet;
				
				return EOK;
			}
		} while (packet_is_valid(cur));
	}
	
	*first = packet;
	return EOK;
}

/** Finds the packet with the given order.
 *
 * @param[in] first	The first packet of the queue.
 * @param[in] order	The packet order value.
 * @return		The packet with the given order.
 * @return		NULL if the first packet is not valid.
 * @return		NULL if the packet is not found.
 */
packet_t *pq_find(packet_t *packet, size_t order)
{
	packet_t *item;

	if (!packet_is_valid(packet))
		return NULL;

	item = packet;
	do {
		if (item->order == order)
			return item;

		item = pm_find(item->next);
	} while (item && (item != packet) && packet_is_valid(item));

	return NULL;
}

/** Inserts packet after the given one.
 *
 * @param[in] packet	The packet in the queue.
 * @param[in] new_packet The new packet to be inserted.
 * @return		EOK on success.
 * @return		EINVAL if etiher of the packets is invalid.
 */
int pq_insert_after(packet_t *packet, packet_t *new_packet)
{
	packet_t *item;

	if (!packet_is_valid(packet) || !packet_is_valid(new_packet))
		return EINVAL;

	new_packet->previous = packet->packet_id;
	new_packet->next = packet->next;
	item = pm_find(packet->next);
	if (item)
		item->previous = new_packet->packet_id;
	packet->next = new_packet->packet_id;

	return EOK;
}

/** Detach the packet from the queue.
 *
 * @param[in] packet	The packet to be detached.
 * @return		The next packet in the queue. If the packet is the first
 *			one of the queue, this becomes the new first one.
 * @return		NULL if there is no packet left.
 * @return		NULL if the packet is not valid.
 */
packet_t *pq_detach(packet_t *packet)
{
	packet_t *next;
	packet_t *previous;

	if (!packet_is_valid(packet))
		return NULL;

	next = pm_find(packet->next);
	if (next)
		next->previous = packet->previous;
	
	previous = pm_find(packet->previous);
	if (previous)
		previous->next = packet->next ;
	
	packet->previous = 0;
	packet->next = 0;
	return next;
}

/** Sets the packet order and metric attributes.
 *
 * @param[in] packeti	The packet to be set.
 * @param[in] order	The packet order value.
 * @param[in] metric	The metric value of the packet.
 * @return		EOK on success.
 * @return		EINVAL if the packet is invalid.
 */
int pq_set_order(packet_t *packet, size_t order, size_t metric)
{
	if (!packet_is_valid(packet))
		return EINVAL;

	packet->order = order;
	packet->metric = metric;
	return EOK;
}

/** Sets the packet order and metric attributes.
 *
 * @param[in] packet	The packet to be set.
 * @param[out] order	The packet order value.
 * @param[out] metric	The metric value of the packet.
 * @return		EOK on success.
 * @return		EINVAL if the packet is invalid.
 */
int pq_get_order(packet_t *packet, size_t *order, size_t *metric)
{
	if (!packet_is_valid(packet))
		return EINVAL;

	if (order)
		*order = packet->order;

	if (metric)
		*metric = packet->metric;

	return EOK;
}

/** Releases the whole queue.
 *
 * Detaches all packets of the queue and calls the packet_release() for each of
 * them.
 *
 * @param[in] first	The first packet of the queue.
 * @param[in] packet_release The releasing function called for each of the
 *			packets after its detachment.
 */
void pq_destroy(packet_t *first, void (*packet_release)(packet_t *packet))
{
	packet_t *actual;
	packet_t *next;

	actual = first;
	while (packet_is_valid(actual)) {
		next = pm_find(actual->next);
		actual->next = 0;
		actual->previous = 0;
		if(packet_release)
			packet_release(actual);
		actual = next;
	}
}

/** Returns the next packet in the queue.
 *
 * @param[in] packet	The packet queue member.
 * @return		The next packet in the queue.
 * @return		NULL if there is no next packet.
 * @return		NULL if the packet is not valid.
 */
packet_t *pq_next(packet_t *packet)
{
	if (!packet_is_valid(packet))
		return NULL;

	return pm_find(packet->next);
}

/** Returns the previous packet in the queue.
 *
 * @param[in] packet	The packet queue member.
 * @return		The previous packet in the queue.
 * @return		NULL if there is no previous packet.
 * @return		NULL if the packet is not valid.
 */
packet_t *pq_previous(packet_t *packet)
{
	if (!packet_is_valid(packet))
		return NULL;

	return pm_find(packet->previous);
}

/** @}
 */
