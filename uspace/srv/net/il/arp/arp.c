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

/** @addtogroup arp
 *  @{
 */

/** @file
 *  ARP module implementation.
 *  @see arp.h
 */

#include <async.h>
#include <malloc.h>
#include <mem.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <string.h>
#include <task.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include "../../err.h"
#include "../../messages.h"
#include "../../modules.h"

#include "../../include/byteorder.h"
#include "../../include/device.h"
#include "../../include/arp_interface.h"
#include "../../include/nil_interface.h"
#include "../../include/protocol_map.h"

#include "../../structures/measured_strings.h"
#include "../../structures/packet/packet.h"
#include "../../structures/packet/packet_client.h"

#include "../il_messages.h"

#include "arp.h"
#include "arp_header.h"
#include "arp_oc.h"
#include "arp_module.h"
#include "arp_messages.h"

/** ARP global data.
 */
arp_globals_t	arp_globals;

/** Creates new protocol specific data.
 *  Allocates and returns the needed memory block as the proto parameter.
 *  @param[out] proto The allocated protocol specific data.
 *  @param[in] service The protocol module service.
 *  @param[in] address The actual protocol device address.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left.
 */
int	arp_proto_create( arp_proto_ref * proto, services_t service, measured_string_ref address );

/** Clears the device specific data.
 *  @param[in] device The device specific data.
 */
void	arp_clear_device( arp_device_ref device );

/** @name Message processing functions
 */
/*@{*/

/** Registers the device.
 *  Creates new device entry in the cache or updates the protocol address if the device with the device identifier and the driver service exists.
 *  @param[in] device_id The device identifier.
 *  @param[in] service The device driver service.
 *  @param[in] protocol The protocol service.
 *  @param[in] address The actual device protocol address.
 *  @returns EOK on success.
 *  @returns EEXIST if another device with the same device identifier and different driver service exists.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the measured_strings_return() function.
 */
int	arp_device_message( device_id_t device_id, services_t service, services_t protocol, measured_string_ref address );

/** Returns the hardware address for the given protocol address.
 *  Sends the ARP request packet if the hardware address is not found in the cache.
 *  @param[in] device_id The device identifier.
 *  @param[in] protocol The protocol service.
 *  @param[in] target The target protocol address.
 *  @returns The hardware address of the target.
 *  @returns NULL if the target parameter is NULL.
 *  @returns NULL if the device is not found.
 *  @returns NULL if the device packet is too small to send a&nbsp;request.
 *  @returns NULL if the hardware address is not found in the cache.
 */
measured_string_ref	arp_translate_message( device_id_t device_id, services_t protocol, measured_string_ref target );

/** Processes the received ARP packet.
 *  Updates the source hardware address if the source entry exists or the packet is targeted to my protocol address.
 *  Responses to the ARP request if the packet is the ARP request and is targeted to my address.
 *  @param[in] device_id The source device identifier.
 *  @param[in,out] packet The received packet.
 *  @returns EOK on success and the packet is no longer needed.
 *  @returns 1 on success and the packet has been reused.
 *  @returns EINVAL if the packet is too small to carry an ARP packet.
 *  @returns EINVAL if the received address lengths differs from the registered values.
 *  @returns ENOENT if the device is not found in the cache.
 *  @returns ENOENT if the protocol for the device is not found in the cache.
 *  @returns ENOMEM if there is not enough memory left.
 */
int	arp_receive_message( device_id_t device_id, packet_t packet );

/** Updates the device content length according to the new MTU value.
 *  @param[in] device_id The device identifier.
 *  @param[in] mtu The new mtu value.
 *  @returns ENOENT if device is not found.
 *  @returns EOK on success.
 */
int	arp_mtu_changed_message( device_id_t device_id, size_t mtu );

/*@}*/

DEVICE_MAP_IMPLEMENT( arp_cache, arp_device_t )

INT_MAP_IMPLEMENT( arp_protos, arp_proto_t )

GENERIC_CHAR_MAP_IMPLEMENT( arp_addr, measured_string_t )

task_id_t arp_task_get_id( void ){
	return task_get_id();
}

int arp_clear_device_req( int arp_phone, device_id_t device_id ){
	arp_device_ref	device;

	fibril_rwlock_write_lock( & arp_globals.lock );
	device = arp_cache_find( & arp_globals.cache, device_id );
	if( ! device ){
		fibril_rwlock_write_unlock( & arp_globals.lock );
		return ENOENT;
	}
	arp_clear_device( device );
	printf( "Device %d cleared\n", device_id );
	fibril_rwlock_write_unlock( & arp_globals.lock );
	return EOK;
}

int arp_clear_address_req( int arp_phone, device_id_t device_id, services_t protocol, measured_string_ref address ){
	arp_device_ref	device;
	arp_proto_ref	proto;

	fibril_rwlock_write_lock( & arp_globals.lock );
	device = arp_cache_find( & arp_globals.cache, device_id );
	if( ! device ){
		fibril_rwlock_write_unlock( & arp_globals.lock );
		return ENOENT;
	}
	proto = arp_protos_find( & device->protos, protocol );
	if( ! proto ){
		fibril_rwlock_write_unlock( & arp_globals.lock );
		return ENOENT;
	}
	arp_addr_exclude( & proto->addresses, address->value, address->length );
	fibril_rwlock_write_unlock( & arp_globals.lock );
	return EOK;
}

int arp_clean_cache_req( int arp_phone ){
	int				count;
	arp_device_ref	device;

	fibril_rwlock_write_lock( & arp_globals.lock );
	for( count = arp_cache_count( & arp_globals.cache ) - 1; count >= 0; -- count ){
		device = arp_cache_get_index( & arp_globals.cache, count );
		if( device ){
			arp_clear_device( device );
			if( device->addr_data ) free( device->addr_data );
			if( device->broadcast_data ) free( device->broadcast_data );
		}
	}
	arp_cache_clear( & arp_globals.cache );
	fibril_rwlock_write_unlock( & arp_globals.lock );
	printf( "Cache cleaned\n" );
	return EOK;
}

int arp_device_req( int arp_phone, device_id_t device_id, services_t protocol, services_t netif, measured_string_ref address ){
	ERROR_DECLARE;

	measured_string_ref tmp;

	// copy the given address for exclusive use
	tmp = measured_string_copy( address );
	if( ERROR_OCCURRED( arp_device_message( device_id, netif, protocol, tmp ))){
		free( tmp->value );
		free( tmp );
	}
	return ERROR_CODE;
}

int arp_translate_req( int arp_phone, device_id_t device_id, services_t protocol, measured_string_ref address, measured_string_ref * translation, char ** data ){
	measured_string_ref	tmp;

	fibril_rwlock_read_lock( & arp_globals.lock );
	tmp = arp_translate_message( device_id, protocol, address );
	if( tmp ){
		* translation = measured_string_copy( tmp );
		fibril_rwlock_read_unlock( & arp_globals.lock );
		if( * translation ){
			* data = ( ** translation ).value;
			return EOK;
		}else{
			return ENOMEM;
		}
	}else{
		fibril_rwlock_read_unlock( & arp_globals.lock );
		return ENOENT;
	}
}

int arp_initialize( async_client_conn_t client_connection ){
	ERROR_DECLARE;

	fibril_rwlock_initialize( & arp_globals.lock );
	fibril_rwlock_write_lock( & arp_globals.lock );
	arp_globals.client_connection = client_connection;
	ERROR_PROPAGATE( arp_cache_initialize( & arp_globals.cache ));
	fibril_rwlock_write_unlock( & arp_globals.lock );
	return EOK;
}

int arp_proto_create( arp_proto_ref * proto, services_t service, measured_string_ref address ){
	ERROR_DECLARE;

	* proto = ( arp_proto_ref ) malloc( sizeof( arp_proto_t ));
	if( !( * proto )) return ENOMEM;
	( ** proto ).service = service;
	( ** proto ).addr = address;
	( ** proto ).addr_data = address->value;
	if( ERROR_OCCURRED( arp_addr_initialize( &( ** proto).addresses ))){
		free( * proto );
		return ERROR_CODE;
	}
	return EOK;
}

int arp_device_message( device_id_t device_id, services_t service, services_t protocol, measured_string_ref address ){
	ERROR_DECLARE;

	arp_device_ref	device;
	arp_proto_ref	proto;
	int				index;
	hw_type_t		hardware;

	fibril_rwlock_write_lock( & arp_globals.lock );
	// an existing device?
	device = arp_cache_find( & arp_globals.cache, device_id );
	if( device ){
		if( device->service != service ){
			printf( "Device %d already exists\n", device->device_id );
			fibril_rwlock_write_unlock( & arp_globals.lock );
			return EEXIST;
		}
		proto = arp_protos_find( & device->protos, protocol );
		if( proto ){
			free( proto->addr );
			free( proto->addr_data );
			proto->addr = address;
			proto->addr_data = address->value;
		}else{
			if( ERROR_OCCURRED( arp_proto_create( & proto, protocol, address ))){
				fibril_rwlock_write_unlock( & arp_globals.lock );
				return ERROR_CODE;
			}
			index = arp_protos_add( & device->protos, proto->service, proto );
			if( index < 0 ){
				fibril_rwlock_write_unlock( & arp_globals.lock );
				free( proto );
				return index;
			}
			printf( "New protocol added:\n\tdevice id\t= %d\n\tproto\t= %d", device_id, protocol );
		}
	}else{
		hardware = hardware_map( service );
		if( ! hardware ) return ENOENT;
		// create a new device
		device = ( arp_device_ref ) malloc( sizeof( arp_device_t ));
		if( ! device ){
			fibril_rwlock_write_unlock( & arp_globals.lock );
			return ENOMEM;
		}
		device->hardware = hardware;
		device->device_id = device_id;
		if( ERROR_OCCURRED( arp_protos_initialize( & device->protos ))
		|| ERROR_OCCURRED( arp_proto_create( & proto, protocol, address ))){
			fibril_rwlock_write_unlock( & arp_globals.lock );
			free( device );
			return ERROR_CODE;
		}
		index = arp_protos_add( & device->protos, proto->service, proto );
		if( index < 0 ){
			fibril_rwlock_write_unlock( & arp_globals.lock );
			arp_protos_destroy( & device->protos );
			free( device );
			return index;
		}
		device->service = service;
		// bind the new one
		device->phone = nil_bind_service( device->service, ( ipcarg_t ) device->device_id, SERVICE_ARP, arp_globals.client_connection );
		if( device->phone < 0 ){
			fibril_rwlock_write_unlock( & arp_globals.lock );
			arp_protos_destroy( & device->protos );
			free( device );
			return EREFUSED;
		}
		// get packet dimensions
		if( ERROR_OCCURRED( nil_packet_size_req( device->phone, device_id, & device->addr_len, & device->prefix, & device->content, & device->suffix ))){
			fibril_rwlock_write_unlock( & arp_globals.lock );
			arp_protos_destroy( & device->protos );
			free( device );
			return ERROR_CODE;
		}
		// get hardware address
		if( ERROR_OCCURRED( nil_get_addr_req( device->phone, device_id, & device->addr, & device->addr_data ))){
			fibril_rwlock_write_unlock( & arp_globals.lock );
			arp_protos_destroy( & device->protos );
			free( device );
			return ERROR_CODE;
		}
		// get broadcast address
		if( ERROR_OCCURRED( nil_get_broadcast_addr_req( device->phone, device_id, & device->broadcast_addr, & device->broadcast_data ))){
			fibril_rwlock_write_unlock( & arp_globals.lock );
			free( device->addr );
			free( device->addr_data );
			arp_protos_destroy( & device->protos );
			free( device );
			return ERROR_CODE;
		}
		if( ERROR_OCCURRED( arp_cache_add( & arp_globals.cache, device->device_id, device ))){
			fibril_rwlock_write_unlock( & arp_globals.lock );
			free( device->addr );
			free( device->addr_data );
			free( device->broadcast_addr );
			free( device->broadcast_data );
			arp_protos_destroy( & device->protos );
			free( device );
			return ERROR_CODE;
		}
		printf( "New device registered:\n\tid\t= %d\n\ttype\t= 0x%x\n\tservice\t= %d\n\tproto\t= %d\n", device->device_id, device->hardware, device->service, protocol );
	}
	fibril_rwlock_write_unlock( & arp_globals.lock );
	return EOK;
}

measured_string_ref arp_translate_message( device_id_t device_id, services_t protocol, measured_string_ref target ){
	arp_device_ref		device;
	arp_proto_ref		proto;
	measured_string_ref	addr;
	size_t				length;
	packet_t			packet;
	arp_header_ref		header;

	if( ! target ) return NULL;
	device = arp_cache_find( & arp_globals.cache, device_id );
	if( ! device ) return NULL;
	proto = arp_protos_find( & device->protos, protocol );
	if(( ! proto ) || ( proto->addr->length != target->length )) return NULL;
	addr = arp_addr_find( & proto->addresses, target->value, target->length );
	if( addr ) return addr;
	// ARP packet content size = header + ( address + translation ) * 2
	length = 8 + ( CONVERT_SIZE( char, uint8_t, proto->addr->length ) + CONVERT_SIZE( char, uint8_t, device->addr->length )) * 2;
	if( length > device->content ) return NULL;
	packet = packet_get_4( arp_globals.net_phone, device->addr_len, device->prefix, length, device->suffix );
	if( ! packet ) return NULL;
	header = ( arp_header_ref ) packet_suffix( packet, length );
	if( ! header ){
		pq_release( arp_globals.net_phone, packet_get_id( packet ));
		return NULL;
	}
	header->hardware = htons( device->hardware );
	header->hardware_length = ( uint8_t ) device->addr->length;
	header->protocol = htons( protocol_map( device->service, protocol ));
	header->protocol_length = ( uint8_t ) proto->addr->length;
	header->operation = htons( ARPOP_REQUEST );
	length = sizeof( arp_header_t );
	memcpy((( uint8_t * ) header ) + length, device->addr->value, device->addr->length );
	length += device->addr->length;
	memcpy((( uint8_t * ) header ) + length, proto->addr->value, proto->addr->length );
	length += proto->addr->length;
	bzero((( uint8_t * ) header ) + length, device->addr->length );
	length += device->addr->length;
	memcpy((( uint8_t * ) header ) + length, target->value, target->length );
	if( packet_set_addr( packet, ( uint8_t * ) device->addr->value, ( uint8_t * ) device->broadcast_addr->value, CONVERT_SIZE( char, uint8_t, device->addr->length )) != EOK ){
		pq_release( arp_globals.net_phone, packet_get_id( packet ));
		return NULL;
	}
	nil_send_msg( device->phone, device_id, packet, SERVICE_ARP );
	return NULL;
}

int arp_receive_message( device_id_t device_id, packet_t packet ){
	ERROR_DECLARE;

	size_t				length;
	arp_header_ref		header;
	arp_device_ref		device;
	arp_proto_ref		proto;
	measured_string_ref	hw_source;
	uint8_t *			src_hw;
	uint8_t *			src_proto;
	uint8_t *			des_hw;
	uint8_t *			des_proto;

	length = packet_get_data_length( packet );
	if( length <= sizeof( arp_header_t )) return EINVAL;
	device = arp_cache_find( & arp_globals.cache, device_id );
	if( ! device ) return ENOENT;
	header = ( arp_header_ref ) packet_get_data( packet );
	if(( ntohs( header->hardware ) != device->hardware )
	|| ( length < sizeof( arp_header_t ) + header->hardware_length * 2u + header->protocol_length * 2u )){
		return EINVAL;
	}
	proto = arp_protos_find( & device->protos, protocol_unmap( device->service, ntohs( header->protocol )));
	if( ! proto ) return ENOENT;
	src_hw = (( uint8_t * ) header ) + sizeof( arp_header_t );
	src_proto = src_hw + header->hardware_length;
	des_hw = src_proto + header->protocol_length;
	des_proto = des_hw + header->hardware_length;
	hw_source = arp_addr_find( & proto->addresses, ( char * ) src_proto, CONVERT_SIZE( uint8_t, char, header->protocol_length ));
	// exists?
	if( hw_source ){
		if( hw_source->length != CONVERT_SIZE( uint8_t, char, header->hardware_length )){
			return EINVAL;
		}
		memcpy( hw_source->value, src_hw, hw_source->length );
	}
	// is my protocol address?
	if( proto->addr->length != CONVERT_SIZE( uint8_t, char, header->protocol_length )){
		return EINVAL;
	}
	if( ! str_lcmp( proto->addr->value, ( char * ) des_proto, proto->addr->length )){
		// not already upadted?
		if( ! hw_source ){
			hw_source = measured_string_create_bulk(( char * ) src_hw, CONVERT_SIZE( uint8_t, char, header->hardware_length ));
			if( ! hw_source ) return ENOMEM;
			ERROR_PROPAGATE( arp_addr_add( & proto->addresses, ( char * ) src_proto, CONVERT_SIZE( uint8_t, char, header->protocol_length ), hw_source ));
		}
		if( ntohs( header->operation ) == ARPOP_REQUEST ){
			header->operation = htons( ARPOP_REPLY );
			memcpy( des_proto, src_proto, header->protocol_length );
			memcpy( src_proto, proto->addr->value, header->protocol_length );
			memcpy( src_hw, device->addr->value, device->addr_len );
			memcpy( des_hw, hw_source->value, header->hardware_length );
			ERROR_PROPAGATE( packet_set_addr( packet, src_hw, des_hw, header->hardware_length ));
			nil_send_msg( device->phone, device_id, packet, SERVICE_ARP );
			return 1;
		}
	}
	return EOK;
}

void arp_clear_device( arp_device_ref device ){
	int				count;
	arp_proto_ref	proto;

	for( count = arp_protos_count( & device->protos ) - 1; count >= 0; -- count ){
		proto = arp_protos_get_index( & device->protos, count );
		if( proto ){
			if( proto->addr ) free( proto->addr );
			if( proto->addr_data ) free( proto->addr_data );
			arp_addr_destroy( & proto->addresses );
		}
	}
	arp_protos_clear( & device->protos );
}

int arp_connect_module( services_t service ){
	if( service != SERVICE_ARP ) return EINVAL;
	return EOK;
}

int arp_mtu_changed_message( device_id_t device_id, size_t mtu ){
	arp_device_ref	device;

	fibril_rwlock_write_lock( & arp_globals.lock );
	device = arp_cache_find( & arp_globals.cache, device_id );
	if( ! device ){
		fibril_rwlock_write_unlock( & arp_globals.lock );
		return ENOENT;
	}
	device->content = mtu;
	printf( "arp - device %d changed mtu to %d\n\n", device_id, mtu );
	fibril_rwlock_write_unlock( & arp_globals.lock );
	return EOK;
}

int arp_message( ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count ){
	ERROR_DECLARE;

	measured_string_ref	address;
	measured_string_ref	translation;
	char *				data;
	packet_t			packet;
	packet_t			next;

//	printf( "message %d - %d\n", IPC_GET_METHOD( * call ), NET_ARP_FIRST );
	* answer_count = 0;
	switch( IPC_GET_METHOD( * call )){
		case IPC_M_PHONE_HUNGUP:
			return EOK;
		case NET_ARP_DEVICE:
			ERROR_PROPAGATE( measured_strings_receive( & address, & data, 1 ));
			if( ERROR_OCCURRED( arp_device_message( IPC_GET_DEVICE( call ), IPC_GET_SERVICE( call ), ARP_GET_NETIF( call ), address ))){
				free( address );
				free( data );
			}
			return ERROR_CODE;
		case NET_ARP_TRANSLATE:
			ERROR_PROPAGATE( measured_strings_receive( & address, & data, 1 ));
			fibril_rwlock_read_lock( & arp_globals.lock );
			translation = arp_translate_message( IPC_GET_DEVICE( call ), IPC_GET_SERVICE( call ), address );
			free( address );
			free( data );
			if( ! translation ){
				fibril_rwlock_read_unlock( & arp_globals.lock );
				return ENOENT;
			}
			ERROR_CODE = measured_strings_reply( translation, 1 );
			fibril_rwlock_read_unlock( & arp_globals.lock );
			return ERROR_CODE;
		case NET_ARP_CLEAR_DEVICE:
			return arp_clear_device_req( 0, IPC_GET_DEVICE( call ));
		case NET_ARP_CLEAR_ADDRESS:
			ERROR_PROPAGATE( measured_strings_receive( & address, & data, 1 ));
			arp_clear_address_req( 0, IPC_GET_DEVICE( call ), IPC_GET_SERVICE( call ), address );
			free( address );
			free( data );
			return EOK;
		case NET_ARP_CLEAN_CACHE:
			return arp_clean_cache_req( 0 );
		case NET_IL_DEVICE_STATE:
			// do nothing - keep the cache
			return EOK;
		case NET_IL_RECEIVED:
			if( ! ERROR_OCCURRED( packet_translate( arp_globals.net_phone, & packet, IPC_GET_PACKET( call )))){
				fibril_rwlock_read_lock( & arp_globals.lock );
				do{
					next = pq_detach( packet );
					ERROR_CODE = arp_receive_message( IPC_GET_DEVICE( call ), packet );
					if( ERROR_CODE != 1 ) pq_release( arp_globals.net_phone, packet_get_id( packet ));
					packet = next;
				}while( packet );
				fibril_rwlock_read_unlock( & arp_globals.lock );
			}
			return ERROR_CODE;
		case NET_IL_MTU_CHANGED:
			return arp_mtu_changed_message( IPC_GET_DEVICE( call ), IPC_GET_MTU( call ));
	}
	return ENOTSUP;
}

/** @}
 */
