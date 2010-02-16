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
 *  Packet client implementation.
 */

#include <errno.h>
#include <mem.h>
#include <unistd.h>
//#include <stdio.h>

#include <sys/mman.h>

#include "../../messages.h"

#include "packet.h"
#include "packet_header.h"
#include "packet_client.h"

int packet_copy_data( packet_t packet, const void * data, size_t length ){
	if( ! packet_is_valid( packet )) return EINVAL;
	if( packet->data_start + length >= packet->length ) return ENOMEM;
	memcpy(( void * ) packet + packet->data_start, data, length );
	if( packet->data_start + length > packet->data_end ){
		packet->data_end = packet->data_start + length;
	}
	return EOK;
}

void * packet_prefix( packet_t packet, size_t length ){
	if(( ! packet_is_valid( packet )) || ( packet->data_start - sizeof( struct packet ) - 2 * ( packet->dest_addr - packet->src_addr ) < length )) return NULL;
	packet->data_start -= length;
	return ( void * ) packet + packet->data_start;
}

void * packet_suffix( packet_t packet, size_t length ){
	if(( ! packet_is_valid( packet )) || ( packet->data_end + length >= packet->length )) return NULL;
	packet->data_end += length;
	return ( void * ) packet + packet->data_end - length;
}

int packet_trim( packet_t packet, size_t prefix, size_t suffix ){
	if( ! packet_is_valid( packet )) return EINVAL;
	if( prefix + suffix > PACKET_DATA_LENGTH( packet )) return ENOMEM;
	packet->data_start += prefix;
	packet->data_end -= suffix;
	return EOK;
}

packet_id_t packet_get_id( const packet_t packet ){
	return packet_is_valid( packet ) ? packet->packet_id : 0;
}

int packet_get_addr( const packet_t packet, uint8_t ** src, uint8_t ** dest ){
	if( ! packet_is_valid( packet )) return EINVAL;
	if( ! packet->addr_len ) return 0;
	if( src ) * src = ( void * ) packet + packet->src_addr;
	if( dest ) * dest = ( void * ) packet + packet->dest_addr;
	return packet->addr_len;
}

size_t packet_get_data_length( const packet_t packet ){
	if( ! packet_is_valid( packet )) return 0;
	return PACKET_DATA_LENGTH( packet );
}

void * packet_get_data( const packet_t packet ){
	if( ! packet_is_valid( packet )) return NULL;
	return ( void * ) packet + packet->data_start;
}

int packet_set_addr( packet_t packet, const uint8_t * src, const uint8_t * dest, size_t addr_len ){
	size_t	padding;
	size_t	allocated;

	if( ! packet_is_valid( packet )) return EINVAL;
	allocated = PACKET_MAX_ADDRESS_LENGTH( packet );
	if( allocated < addr_len ) return ENOMEM;
	padding = allocated - addr_len;
	packet->addr_len = addr_len;
	if( src ){
		memcpy(( void * ) packet + packet->src_addr, src, addr_len );
		if( padding ) bzero(( void * ) packet + packet->src_addr + addr_len, padding );
	}else{
		bzero(( void * ) packet + packet->src_addr, allocated );
	}
	if( dest ){
		memcpy(( void * ) packet + packet->dest_addr, dest, addr_len );
		if( padding ) bzero(( void * ) packet + packet->dest_addr + addr_len, padding );
	}else{
		bzero(( void * ) packet + packet->dest_addr, allocated );
	}
	return EOK;
}

packet_t packet_get_copy( int phone, packet_t packet ){
	packet_t	copy;
	uint8_t *	src;
	uint8_t *	dest;
	size_t		addrlen;

	if( ! packet_is_valid( packet )) return NULL;
	// get a new packet
	copy = packet_get_4( phone, PACKET_DATA_LENGTH( packet ), PACKET_MAX_ADDRESS_LENGTH( packet ), packet->max_prefix, PACKET_MIN_SUFFIX( packet ));
	if( ! copy ) return NULL;
	// get addresses
	addrlen = packet_get_addr( packet, & src, & dest );
	// copy data
	if(( packet_copy_data( copy, packet_get_data( packet ), PACKET_DATA_LENGTH( packet )) == EOK )
	// copy addresses if present
	&& (( addrlen <= 0 ) || ( packet_set_addr( copy, src, dest, addrlen ) == EOK ))){
		copy->order = packet->order;
		copy->metric = packet->metric;
		return copy;
	}else{
		pq_release( phone, copy->packet_id );
		return NULL;
	}
}

/** @}
 */
