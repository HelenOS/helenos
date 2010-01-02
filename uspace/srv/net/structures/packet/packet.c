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
 *  Packet map and queue implementation.
 *  This file has to be compiled with both the packet server and the client.
 */

#include <errno.h>
#include <malloc.h>
#include <mem.h>
#include <fibril_synch.h>
//#include <stdio.h>
#include <unistd.h>

#include <sys/mman.h>

#include "../../err.h"

#include "../generic_field.h"

#include "packet.h"
#include "packet_header.h"

/** Packet map page size.
 */
#define PACKET_MAP_SIZE	100

/** Returns the packet map page index.
 *  @param[in] packet_id The packet identifier.
 */
#define PACKET_MAP_PAGE( packet_id )	((( packet_id ) - 1 ) / PACKET_MAP_SIZE )

/** Returns the packet index in the corresponding packet map page.
 *  @param[in] packet_id The packet identifier.
 */
#define PACKET_MAP_INDEX( packet_id )	((( packet_id ) - 1 ) % PACKET_MAP_SIZE )

/** Type definition of the packet map page.
 */
typedef packet_t packet_map_t[ PACKET_MAP_SIZE ];
/** Type definition of the packet map page pointer.
 */
typedef packet_map_t * packet_map_ref;

/** Packet map.
 *  Maps packet identifiers to the packet references.
 *  @see generic_field.h
 */
GENERIC_FIELD_DECLARE( gpm, packet_map_t );

/** Releases the packet.
 *  @param[in] packet The packet to be released.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet is not valid.
 */
int packet_destroy( packet_t packet );

/** Packet map global data.
 */
static struct{
	/** Safety lock.
	 */
	fibril_rwlock_t	lock;
	/** Packet map.
	 */
	gpm_t	packet_map;
} pm_globals;

GENERIC_FIELD_IMPLEMENT( gpm, packet_map_t );

int packet_destroy( packet_t packet ){
	if( ! packet_is_valid( packet )) return EINVAL;
	return munmap( packet, packet->length );
}

int pm_init( void ){
	ERROR_DECLARE;

	fibril_rwlock_initialize( & pm_globals.lock );
	fibril_rwlock_write_lock( & pm_globals.lock );
	ERROR_PROPAGATE( gpm_initialize( & pm_globals.packet_map ));
	fibril_rwlock_write_unlock( & pm_globals.lock );
	return EOK;
}

packet_t pm_find( packet_id_t packet_id ){
	packet_map_ref map;
	packet_t packet;

	if( ! packet_id ) return NULL;
	fibril_rwlock_read_lock( & pm_globals.lock );
	if( packet_id > PACKET_MAP_SIZE * gpm_count( & pm_globals.packet_map )){
		fibril_rwlock_read_unlock( & pm_globals.lock );
		return NULL;
	}
	map = gpm_get_index( & pm_globals.packet_map, PACKET_MAP_PAGE( packet_id ));
	if( ! map ){
		fibril_rwlock_read_unlock( & pm_globals.lock );
		return NULL;
	}
	packet = ( * map )[ PACKET_MAP_INDEX( packet_id ) ];
	fibril_rwlock_read_unlock( & pm_globals.lock );
	return packet;
}

int pm_add( packet_t packet ){
	ERROR_DECLARE;

	packet_map_ref map;

	if( ! packet_is_valid( packet )) return EINVAL;
	fibril_rwlock_write_lock( & pm_globals.lock );
	if( PACKET_MAP_PAGE( packet->packet_id ) < gpm_count( & pm_globals.packet_map )){
		map = gpm_get_index( & pm_globals.packet_map, PACKET_MAP_PAGE( packet->packet_id ));
	}else{
		do{
			map = ( packet_map_ref ) malloc( sizeof( packet_map_t ));
			if( ! map ){
				fibril_rwlock_write_unlock( & pm_globals.lock );
				return ENOMEM;
			}
			bzero( map, sizeof( packet_map_t ));
			if(( ERROR_CODE = gpm_add( & pm_globals.packet_map, map )) < 0 ){
				fibril_rwlock_write_unlock( & pm_globals.lock );
				free( map );
				return ERROR_CODE;
			}
		}while( PACKET_MAP_PAGE( packet->packet_id ) >= gpm_count( & pm_globals.packet_map ));
	}
	( * map )[ PACKET_MAP_INDEX( packet->packet_id ) ] = packet;
	fibril_rwlock_write_unlock( & pm_globals.lock );
	return EOK;
}

void pm_destroy( void ){
	int count;
	int index;
	packet_map_ref map;
	packet_t packet;

	fibril_rwlock_write_lock( & pm_globals.lock );
	count = gpm_count( & pm_globals.packet_map );
	while( count > 0 ){
		map = gpm_get_index( & pm_globals.packet_map, count - 1 );
		for( index = PACKET_MAP_SIZE - 1; index >= 0; -- index ){
			packet = ( * map )[ index ];
			if( packet_is_valid( packet )){
				munmap( packet, packet->length );
			}
		}
	}
	gpm_destroy( & pm_globals.packet_map );
	// leave locked
}

packet_t pq_add( packet_t first, packet_t packet, size_t order, size_t metric ){
	packet_t	item;

	if( ! packet_is_valid( packet )) return NULL;
	pq_set_order( packet, order, metric );
	if( packet_is_valid( first )){
		item = first;
		do{
			if( item->order < order ){
				if( item->next ){
					item = pm_find( item->next );
				}else{
					item->next = packet->packet_id;
					packet->previous = item->packet_id;
					return first;
				}
			}else{
				packet->previous = item->previous;
				packet->next = item->packet_id;
				item->previous = packet->packet_id;
				item = pm_find( packet->previous );
				if( item ) item->next = packet->packet_id;
				return item ? first : packet;
			}
		}while( packet_is_valid( item ));
	}
	return packet;
}

packet_t pq_find( packet_t packet, size_t order ){
	packet_t	item;

	if( ! packet_is_valid( packet )) return NULL;
	if( packet->order == order ) return packet;
	item = pm_find( packet->next );
	while( item && ( item != packet )){
		item = pm_find( item->next );
		if( item->order == order ){
			return item;
		}
	}
	return NULL;
}

int	pq_insert_after( packet_t packet, packet_t new_packet ){
	packet_t	item;

	if( !( packet_is_valid( packet ) && packet_is_valid( new_packet ))) return EINVAL;
	new_packet->previous = packet->packet_id;
	new_packet->next = packet->next;
	item = pm_find( packet->next );
	if( item ) item->previous = new_packet->packet_id;
	packet->next = new_packet->packet_id;
	return EOK;
}

packet_t pq_detach( packet_t packet ){
	packet_t next;
	packet_t previous;

	if( ! packet_is_valid( packet )) return NULL;
	next = pm_find( packet->next );
	if( next ){
		next->previous = packet->previous;
		previous = pm_find( next->previous );
		if( previous ){
			previous->next = next->packet_id;
		}
	}
	packet->previous = 0;
	packet->next = 0;
	return next;
}

int pq_set_order( packet_t packet, size_t order, size_t metric ){
	if( ! packet_is_valid( packet )) return EINVAL;
	packet->order = order;
	packet->metric = metric;
	return EOK;
}

int pq_get_order( packet_t packet, size_t * order, size_t * metric ){
	if( ! packet_is_valid( packet )) return EINVAL;
	if( order ) * order = packet->order;
	if( metric ) * metric = packet->metric;
	return EOK;
}

void pq_destroy( packet_t first, void ( * packet_release )( packet_t packet )){
	packet_t	actual;
	packet_t	next;

	actual = first;
	while( packet_is_valid( actual )){
		next = pm_find( actual->next );
		actual->next = 0;
		actual->previous = 0;
		if( packet_release ) packet_release( actual );
		actual = next;
	}
}

packet_t pq_next( packet_t packet ){
	if( ! packet_is_valid( packet )) return NULL;
	return pm_find( packet->next );
}

packet_t pq_previous( packet_t packet ){
	if( ! packet_is_valid( packet )) return NULL;
	return pm_find( packet->previous );
}

/** @}
 */
