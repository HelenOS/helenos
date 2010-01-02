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

/** @addtogroup packet
 *  @{
 */

/** @file
 *  Packet server implementation.
 */

#include <align.h>
#include <assert.h>
#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <unistd.h>

#include <ipc/ipc.h>
#include <sys/mman.h>

#include "../../err.h"
#include "../../messages.h"

#include "packet.h"
#include "packet_client.h"
#include "packet_header.h"
#include "packet_messages.h"
#include "packet_server.h"

#define FREE_QUEUES_COUNT	7

/** The default address length reserved for new packets.
 */
#define DEFAULT_ADDR_LEN	32

/** The default prefix reserved for new packets.
 */
#define DEFAULT_PREFIX		64

/** The default suffix reserved for new packets.
 */
#define DEFAULT_SUFFIX		64

/** Packet server global data.
 */
static struct{
	/** Safety lock.
	 */
	fibril_mutex_t lock;
	/** Free packet queues.
	 */
	packet_t free[ FREE_QUEUES_COUNT ];
	/** Packet length upper bounds of the free packet queues.
	 *  The maximal lengths of packets in each queue in the ascending order.
	 *  The last queue is not limited.
	 */
	size_t sizes[ FREE_QUEUES_COUNT ];
	/** Total packets allocated.
	 */
	unsigned int count;
} ps_globals = {
	.lock = {
		.counter = 1,
		.waiters = {
			.prev = & ps_globals.lock.waiters,
			.next = & ps_globals.lock.waiters,
		}
	},
	.free = { NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	.sizes = { PAGE_SIZE, PAGE_SIZE * 2, PAGE_SIZE * 4, PAGE_SIZE * 8, PAGE_SIZE * 16, PAGE_SIZE * 32, PAGE_SIZE * 64 },
	.count = 0
};

/** @name Packet server support functions
 */
/*@{*/

/** Returns the packet of dimensions at least as given.
 *  Tries to reuse free packets first.
 *  Creates a&nbsp;new packet aligned to the memory page size if none available.
 *  Locks the global data during its processing.
 *  @param[in] addr_len The source and destination addresses maximal length in bytes.
 *  @param[in] max_prefix The maximal prefix length in bytes.
 *  @param[in] max_content The maximal content length in bytes.
 *  @param[in] max_suffix The maximal suffix length in bytes.
 *  @returns The packet of dimensions at least as given.
 *  @returns NULL if there is not enough memory left.
 */
packet_t	packet_get( size_t addr_len, size_t max_prefix, size_t max_content, size_t max_suffix );

/** Releases the packet queue.
 *  @param[in] packet_id The first packet identifier.
 *  @returns EOK on success.
 *  @returns ENOENT if there is no such packet.
 */
int	packet_release_wrapper( packet_id_t packet_id );

/** Releases the packet and returns it to the appropriate free packet queue.
 *  Should be used only when the global data are locked.
 *  @param[in] packet The packet to be released.
 */
void packet_release( packet_t packet );

/** Creates a&nbsp;new packet of dimensions at least as given.
 *  Should be used only when the global data are locked.
 *  @param[in] length The total length of the packet, including the header, the addresses and the data of the packet.
 *  @param[in] addr_len The source and destination addresses maximal length in bytes.
 *  @param[in] max_prefix The maximal prefix length in bytes.
 *  @param[in] max_content The maximal content length in bytes.
 *  @param[in] max_suffix The maximal suffix length in bytes.
 *  @returns The packet of dimensions at least as given.
 *  @returns NULL if there is not enough memory left.
 */
packet_t	packet_create( size_t length, size_t addr_len, size_t max_prefix, size_t max_content, size_t max_suffix );

/** Clears and initializes the packet according to the given dimensions.
 *  @param[in] packet The packet to be initialized.
 *  @param[in] addr_len The source and destination addresses maximal length in bytes.
 *  @param[in] max_prefix The maximal prefix length in bytes.
 *  @param[in] max_content The maximal content length in bytes.
 *  @param[in] max_suffix The maximal suffix length in bytes.
 */
void	packet_init( packet_t packet, size_t addr_len, size_t max_prefix, size_t max_content, size_t max_suffix );

/** Shares the packet memory block.
 *  @param[in] packet The packet to be shared.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet is not valid.
 *  @returns EINVAL if the calling module does not accept the memory.
 *  @returns ENOMEM if the desired and actual sizes differ.
 *  @returns Other error codes as defined for the async_share_in_finalize() function.
 */
int packet_reply( const packet_t packet );

/*@}*/

int packet_translate( int phone, packet_ref packet, packet_id_t packet_id ){
	if( ! packet ) return EINVAL;
	* packet = pm_find( packet_id );
	return ( * packet ) ? EOK : ENOENT;
}

packet_t packet_get_4( int phone, size_t max_content, size_t addr_len, size_t max_prefix, size_t max_suffix ){
	return packet_get( addr_len, max_prefix, max_content, max_suffix );
}

packet_t packet_get_1( int phone, size_t content ){
	return packet_get( DEFAULT_ADDR_LEN, DEFAULT_PREFIX, content, DEFAULT_SUFFIX );
}

void pq_release( int phone, packet_id_t packet_id ){
	( void ) packet_release_wrapper( packet_id );
}

int	packet_server_message( ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count ){
	packet_t packet;

	* answer_count = 0;
	switch( IPC_GET_METHOD( * call )){
		case IPC_M_PHONE_HUNGUP:
			return EOK;
		case NET_PACKET_CREATE_1:
			packet = packet_get( DEFAULT_ADDR_LEN, DEFAULT_PREFIX, IPC_GET_CONTENT( call ), DEFAULT_SUFFIX );
			if( ! packet ) return ENOMEM;
			* answer_count = 2;
			IPC_SET_ARG1( * answer, packet->packet_id );
			IPC_SET_ARG2( * answer, packet->length );
			return EOK;
		case NET_PACKET_CREATE_4:
			packet = packet_get( IPC_GET_ADDR_LEN( call ), IPC_GET_PREFIX( call ), IPC_GET_CONTENT( call ), IPC_GET_SUFFIX( call ));
			if( ! packet ) return ENOMEM;
			* answer_count = 2;
			IPC_SET_ARG1( * answer, packet->packet_id );
			IPC_SET_ARG2( * answer, packet->length );
			return EOK;
		case NET_PACKET_GET:
			packet = pm_find( IPC_GET_ID( call ));
			if( ! packet_is_valid( packet )) return ENOENT;
			return packet_reply( packet );
		case NET_PACKET_GET_SIZE:
			packet = pm_find( IPC_GET_ID( call ));
			if( ! packet_is_valid( packet )) return ENOENT;
			IPC_SET_ARG1( * answer, packet->length );
			* answer_count = 1;
			return EOK;
		case NET_PACKET_RELEASE:
			return packet_release_wrapper( IPC_GET_ID( call ));
	}
	return ENOTSUP;
}

int packet_release_wrapper( packet_id_t packet_id ){
	packet_t	packet;

	packet = pm_find( packet_id );
	if( ! packet_is_valid( packet )) return ENOENT;
	fibril_mutex_lock( & ps_globals.lock );
	pq_destroy( packet, packet_release );
	fibril_mutex_unlock( & ps_globals.lock );
	return EOK;
}

void packet_release( packet_t packet ){
	int index;

	// remove debug dump
//	printf( "packet %d released\n", packet->packet_id );
	for( index = 0; ( index < FREE_QUEUES_COUNT - 1 ) && ( packet->length > ps_globals.sizes[ index ] ); ++ index );
	ps_globals.free[ index ] = pq_add( ps_globals.free[ index ], packet, packet->length, packet->length );
	assert( ps_globals.free[ index ] );
}

packet_t packet_get( size_t addr_len, size_t max_prefix, size_t max_content, size_t max_suffix ){
	int index;
	packet_t packet;
	size_t length;

	length = ALIGN_UP( sizeof( struct packet ) + 2 * addr_len + max_prefix + max_content + max_suffix, PAGE_SIZE );
	fibril_mutex_lock( & ps_globals.lock );
	for( index = 0; index < FREE_QUEUES_COUNT - 1; ++ index ){
		if( length <= ps_globals.sizes[ index ] ){
			packet = ps_globals.free[ index ];
			while( packet_is_valid( packet ) && ( packet->length < length )){
				packet = pm_find( packet->next );
			}
			if( packet_is_valid( packet )){
				if( packet == ps_globals.free[ index ] ){
					ps_globals.free[ index ] = pq_detach( packet );
				}else{
					pq_detach( packet );
				}
				packet_init( packet, addr_len, max_prefix, max_content, max_suffix );
				fibril_mutex_unlock( & ps_globals.lock );
				// remove debug dump
//				printf( "packet %d got\n", packet->packet_id );
				return packet;
			}
		}
	}
	packet = packet_create( length, addr_len, max_prefix, max_content, max_suffix );
	fibril_mutex_unlock( & ps_globals.lock );
	// remove debug dump
//	printf( "packet %d created\n", packet->packet_id );
	return packet;
}

packet_t packet_create( size_t length, size_t addr_len, size_t max_prefix, size_t max_content, size_t max_suffix ){
	ERROR_DECLARE;

	packet_t	packet;

	// already locked
	packet = ( packet_t ) mmap( NULL, length, PROTO_READ | PROTO_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0 );
	if( packet == MAP_FAILED ) return NULL;
	++ ps_globals.count;
	packet->packet_id = ps_globals.count;
	packet->length = length;
	packet_init( packet, addr_len, max_prefix, max_content, max_suffix );
	packet->magic_value = PACKET_MAGIC_VALUE;
	if( ERROR_OCCURRED( pm_add( packet ))){
		munmap( packet, packet->length );
		return NULL;
	}
	return packet;
}

void packet_init( packet_t packet, size_t addr_len, size_t max_prefix, size_t max_content, size_t max_suffix ){
	// clear the packet content
	bzero((( void * ) packet ) + sizeof( struct packet ), packet->length - sizeof( struct packet ));
	// clear the packet header
	packet->order = 0;
	packet->metric = 0;
	packet->previous = 0;
	packet->next = 0;
	packet->addr_len = 0;
	packet->src_addr = sizeof( struct packet );
	packet->dest_addr = packet->src_addr + addr_len;
	packet->max_prefix = max_prefix;
	packet->max_content = max_content;
	packet->data_start = packet->dest_addr + addr_len + packet->max_prefix;
	packet->data_end = packet->data_start;
}

int packet_reply( const packet_t packet ){
	ipc_callid_t	callid;
	size_t			size;

	if( ! packet_is_valid( packet )) return EINVAL;
	if( async_share_in_receive( & callid, & size ) <= 0 ) return EINVAL;
	if( size != packet->length ) return ENOMEM;
	return async_share_in_finalize( callid, packet, PROTO_READ | PROTO_WRITE );
}

/** @}
 */
