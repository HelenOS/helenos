/*
 * Copyright (c) 2009 Lukas Mejdrech
 * Copyright (c) 2011 Radim Vansa
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

/** @addtogroup libpacket
 *  @{
 */

/** @file
 * Packet server implementation.
 */

#include <align.h>
#include <assert.h>
#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <fibril_synch.h>
#include <unistd.h>
#include <sys/mman.h>
#include <ipc/packet.h>
#include <ipc/net.h>
#include <net/packet.h>
#include <net/packet_header.h>

#include "packet_server.h"

#define PACKET_SERVER_PROFILE 1

/** Number of queues cacheing the unused packets */
#define FREE_QUEUES_COUNT	7
/** Maximum number of packets in each queue */
#define FREE_QUEUE_MAX_LENGTH	16

/** The default address length reserved for new packets. */
#define DEFAULT_ADDR_LEN	32

/** The default prefix reserved for new packets. */
#define DEFAULT_PREFIX		64

/** The default suffix reserved for new packets. */
#define DEFAULT_SUFFIX		64

/** The queue with unused packets */
typedef struct packet_queue {
	packet_t *first;	/**< First packet in the queue */
	size_t packet_size; /**< Maximal size of the packets in this queue */
	int count;			/**< Length of the queue */
} packet_queue_t;

/** Packet server global data. */
static struct {
	/** Safety lock. */
	fibril_mutex_t lock;
	/** Free packet queues. */
	packet_queue_t free_queues[FREE_QUEUES_COUNT];
	
	/** Total packets allocated. */
	packet_id_t next_id;
} ps_globals = {
	.lock = FIBRIL_MUTEX_INITIALIZER(ps_globals.lock),
	.free_queues = {
		{ NULL, PAGE_SIZE, 0},
		{ NULL, PAGE_SIZE * 2, 0},
		{ NULL, PAGE_SIZE * 4, 0},
		{ NULL, PAGE_SIZE * 8, 0},
		{ NULL, PAGE_SIZE * 16, 0},
		{ NULL, PAGE_SIZE * 32, 0},
		{ NULL, PAGE_SIZE * 64, 0},
	},
	.next_id = 1
};

/** Clears and initializes the packet according to the given dimensions.
 *
 * @param[in] packet	The packet to be initialized.
 * @param[in] addr_len	The source and destination addresses maximal length in
 *			bytes.
 * @param[in] max_prefix The maximal prefix length in bytes.
 * @param[in] max_content The maximal content length in bytes.
 * @param[in] max_suffix The maximal suffix length in bytes.
 */
static void packet_init(packet_t *packet,
	size_t addr_len, size_t max_prefix, size_t max_content, size_t max_suffix)
{
	/* Clear the packet content */
	bzero(((void *) packet) + sizeof(packet_t),
	    packet->length - sizeof(packet_t));
	
	/* Clear the packet header */
	packet->order = 0;
	packet->metric = 0;
	packet->previous = 0;
	packet->next = 0;
	packet->offload_info = 0;
	packet->offload_mask = 0;
	packet->addr_len = 0;
	packet->src_addr = sizeof(packet_t);
	packet->dest_addr = packet->src_addr + addr_len;
	packet->max_prefix = max_prefix;
	packet->max_content = max_content;
	packet->data_start = packet->dest_addr + addr_len + packet->max_prefix;
	packet->data_end = packet->data_start;
}

/**
 * Releases the memory allocated for the packet
 *
 * @param[in] packet Pointer to the memory where the packet was allocated
 */
static void packet_dealloc(packet_t *packet)
{
	pm_remove(packet);
	munmap(packet, packet->length);
}

/** Creates a new packet of dimensions at least as given.
 *
 * @param[in] length	The total length of the packet, including the header,
 *			the addresses and the data of the packet.
 * @param[in] addr_len	The source and destination addresses maximal length in
 *			bytes.
 * @param[in] max_prefix The maximal prefix length in bytes.
 * @param[in] max_content The maximal content length in bytes.
 * @param[in] max_suffix The maximal suffix length in bytes.
 * @return		The packet of dimensions at least as given.
 * @return		NULL if there is not enough memory left.
 */
static packet_t *packet_alloc(size_t length, size_t addr_len,
	size_t max_prefix, size_t max_content, size_t max_suffix)
{
	packet_t *packet;
	int rc;

	/* Global lock is locked */
	assert(fibril_mutex_is_locked(&ps_globals.lock));
	/* The length is some multiple of PAGE_SIZE */
	assert(!(length & (PAGE_SIZE - 1)));

	packet = (packet_t *) mmap(NULL, length, PROTO_READ | PROTO_WRITE,
		MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if (packet == MAP_FAILED)
		return NULL;
	
	/* Using 32bit packet_id the id could overflow */
	packet_id_t pid;
	do {
		pid = ps_globals.next_id;
		ps_globals.next_id++;
	} while (!pid || pm_find(pid));
	packet->packet_id = pid;

	packet->length = length;
	packet_init(packet, addr_len, max_prefix, max_content, max_suffix);
	packet->magic_value = PACKET_MAGIC_VALUE;
	rc = pm_add(packet);
	if (rc != EOK) {
		packet_dealloc(packet);
		return NULL;
	}
	
	return packet;
}

/** Return the packet of dimensions at least as given.
 *
 * Try to reuse free packets first.
 * Create a new packet aligned to the memory page size if none available.
 * Lock the global data during its processing.
 *
 * @param[in] addr_len	The source and destination addresses maximal length in
 *			bytes.
 * @param[in] max_prefix The maximal prefix length in bytes.
 * @param[in] max_content The maximal content length in bytes.
 * @param[in] max_suffix The maximal suffix length in bytes.
 * @return		The packet of dimensions at least as given.
 * @return		NULL if there is not enough memory left.
 */
static packet_t *packet_get_local(size_t addr_len,
	size_t max_prefix, size_t max_content, size_t max_suffix)
{
	size_t length = ALIGN_UP(sizeof(packet_t) + 2 * addr_len
		+ max_prefix + max_content + max_suffix, PAGE_SIZE);
	
	if (length > PACKET_MAX_LENGTH)
		return NULL;

	fibril_mutex_lock(&ps_globals.lock);
	
	packet_t *packet;
	unsigned int index;
	
	for (index = 0; index < FREE_QUEUES_COUNT; index++) {
		if ((length > ps_globals.free_queues[index].packet_size) &&
			(index < FREE_QUEUES_COUNT - 1))
			continue;
		
		packet = ps_globals.free_queues[index].first;
		while (packet_is_valid(packet) && (packet->length < length))
			packet = pm_find(packet->next);
		
		if (packet_is_valid(packet)) {
			ps_globals.free_queues[index].count--;
			if (packet == ps_globals.free_queues[index].first) {
				ps_globals.free_queues[index].first = pq_detach(packet);
			} else {
				pq_detach(packet);
			}
			
			packet_init(packet, addr_len, max_prefix, max_content, max_suffix);
			fibril_mutex_unlock(&ps_globals.lock);
			
			return packet;
		}
	}
	
	packet = packet_alloc(length, addr_len,
		max_prefix, max_content, max_suffix);
	
	fibril_mutex_unlock(&ps_globals.lock);
	
	return packet;
}

/** Release the packet and returns it to the appropriate free packet queue.
 *
 * @param[in] packet	The packet to be released.
 *
 */
static void packet_release(packet_t *packet)
{
	int index;
	int result;

	assert(fibril_mutex_is_locked(&ps_globals.lock));

	for (index = 0; (index < FREE_QUEUES_COUNT - 1) &&
	    (packet->length > ps_globals.free_queues[index].packet_size); index++) {
		;
	}
	
	ps_globals.free_queues[index].count++;
	result = pq_add(&ps_globals.free_queues[index].first, packet,
		packet->length,	packet->length);
	assert(result == EOK);
}

/** Releases the packet queue.
 *
 * @param[in] packet_id	The first packet identifier.
 * @return		EOK on success.
 * @return		ENOENT if there is no such packet.
 */
static int packet_release_wrapper(packet_id_t packet_id)
{
	packet_t *packet;

	packet = pm_find(packet_id);
	if (!packet_is_valid(packet))
		return ENOENT;

	fibril_mutex_lock(&ps_globals.lock);
	pq_destroy(packet, packet_release);
	fibril_mutex_unlock(&ps_globals.lock);

	return EOK;
}

/** Shares the packet memory block.
 * @param[in] packet	The packet to be shared.
 * @return		EOK on success.
 * @return		EINVAL if the packet is not valid.
 * @return		EINVAL if the calling module does not accept the memory.
 * @return		ENOMEM if the desired and actual sizes differ.
 * @return		Other error codes as defined for the
 *			async_share_in_finalize() function.
 */
static int packet_reply(packet_t *packet)
{
	ipc_callid_t callid;
	size_t size;

	if (!packet_is_valid(packet))
		return EINVAL;

	if (!async_share_in_receive(&callid, &size)) {
		async_answer_0(callid, EINVAL);
		return EINVAL;
	}

	if (size != packet->length) {
		async_answer_0(callid, ENOMEM);
		return ENOMEM;
	}
	
	return async_share_in_finalize(callid, packet,
	    PROTO_READ | PROTO_WRITE);
}

/** Processes the packet server message.
 *
 * @param[in] callid	The message identifier.
 * @param[in] call	The message parameters.
 * @param[out] answer	The message answer parameters.
 * @param[out] answer_count The last parameter for the actual answer in the
 *			answer parameter.
 * @return		EOK on success.
 * @return		ENOMEM if there is not enough memory left.
 * @return		ENOENT if there is no such packet as in the packet
 *			message parameter.
 * @return		ENOTSUP if the message is not known.
 * @return		Other error codes as defined for the
 *			packet_release_wrapper() function.
 */
int packet_server_message(ipc_callid_t callid, ipc_call_t *call, ipc_call_t *answer,
    size_t *answer_count)
{
	packet_t *packet;
	
	if (!IPC_GET_IMETHOD(*call))
		return EOK;
	
	*answer_count = 0;
	switch (IPC_GET_IMETHOD(*call)) {
	case NET_PACKET_CREATE_1:
		packet = packet_get_local(DEFAULT_ADDR_LEN, DEFAULT_PREFIX,
			IPC_GET_CONTENT(*call), DEFAULT_SUFFIX);
		if (!packet)
			return ENOMEM;
		*answer_count = 2;
		IPC_SET_ARG1(*answer, (sysarg_t) packet->packet_id);
		IPC_SET_ARG2(*answer, (sysarg_t) packet->length);
		return EOK;
	
	case NET_PACKET_CREATE_4:
		packet = packet_get_local(
			((DEFAULT_ADDR_LEN < IPC_GET_ADDR_LEN(*call)) ?
		    IPC_GET_ADDR_LEN(*call) : DEFAULT_ADDR_LEN),
		    DEFAULT_PREFIX + IPC_GET_PREFIX(*call),
		    IPC_GET_CONTENT(*call),
		    DEFAULT_SUFFIX + IPC_GET_SUFFIX(*call));
		if (!packet)
			return ENOMEM;
		*answer_count = 2;
		IPC_SET_ARG1(*answer, (sysarg_t) packet->packet_id);
		IPC_SET_ARG2(*answer, (sysarg_t) packet->length);
		return EOK;
	
	case NET_PACKET_GET:
		packet = pm_find(IPC_GET_ID(*call));
		if (!packet_is_valid(packet)) {
			return ENOENT;
		}
		return packet_reply(packet);
	
	case NET_PACKET_GET_SIZE:
		packet = pm_find(IPC_GET_ID(*call));
		if (!packet_is_valid(packet))
			return ENOENT;
		IPC_SET_ARG1(*answer, (sysarg_t) packet->length);
		*answer_count = 1;
		return EOK;
	
	case NET_PACKET_RELEASE:
		return packet_release_wrapper(IPC_GET_ID(*call));
	}
	
	return ENOTSUP;
}

int packet_server_init()
{
	return EOK;
}

/** @}
 */
