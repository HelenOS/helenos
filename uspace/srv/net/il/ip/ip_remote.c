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

/** @addtogroup ip
 *  @{
 */

/** @file
 *  IP interface implementation for standalone remote modules.
 *  @see ip_interface.h
 *  @see il_interface.h
 */

#include <ipc/services.h>

#include "../../messages.h"
#include "../../modules.h"

#include "../../include/device.h"
#include "../../include/inet.h"
#include "../../include/ip_interface.h"

#include "../../structures/packet/packet_client.h"

#include "../il_messages.h"

#include "ip_messages.h"

int ip_device_req( int ip_phone, device_id_t device_id, services_t service ){
	return generic_device_req( ip_phone, NET_IL_DEVICE, device_id, 0, service );
}

int ip_send_msg( int ip_phone, device_id_t device_id, packet_t packet, services_t sender, services_t error ){
	return generic_send_msg( ip_phone, NET_IL_SEND, device_id, packet_get_id( packet ), sender, error );
}

int ip_connect_module( services_t service ){
	return connect_to_service( SERVICE_IP );
}

int ip_add_route_req( int ip_phone, device_id_t device_id, in_addr_t address, in_addr_t netmask, in_addr_t gateway ){
	return ( int ) async_req_4_0( ip_phone, NET_IP_ADD_ROUTE, ( ipcarg_t ) device_id, ( ipcarg_t ) gateway.s_addr, ( ipcarg_t ) address.s_addr, ( ipcarg_t ) netmask.s_addr );
}

int ip_set_gateway_req( int ip_phone, device_id_t device_id, in_addr_t gateway ){
	return ( int ) async_req_2_0( ip_phone, NET_IP_SET_GATEWAY, ( ipcarg_t ) device_id, ( ipcarg_t ) gateway.s_addr );
}

int ip_packet_size_req( int ip_phone, device_id_t device_id, packet_dimension_ref packet_dimension ){
	return generic_packet_size_req( ip_phone, NET_IL_PACKET_SPACE, device_id, packet_dimension );
}

int ip_bind_service( services_t service, int protocol, services_t me, async_client_conn_t receiver, tl_received_msg_t tl_received_msg ){
	return ( int ) bind_service( service, ( ipcarg_t ) protocol, me, service, receiver );
}

int ip_received_error_msg( int ip_phone, device_id_t device_id, packet_t packet, services_t target, services_t error ){
	return generic_received_msg( ip_phone, NET_IP_RECEIVED_ERROR, device_id, packet_get_id( packet ), target, error );
}

int ip_get_route_req( int ip_phone, ip_protocol_t protocol, const struct sockaddr * destination, socklen_t addrlen, device_id_t * device_id, ip_pseudo_header_ref * header, size_t * headerlen ){
	aid_t			message_id;
	ipcarg_t		result;
	ipc_call_t		answer;

	if( !( destination && ( addrlen > 0 ))) return EINVAL;
	if( !( device_id && header && headerlen )) return EBADMEM;
	* header = NULL;
	message_id = async_send_1( ip_phone, NET_IP_GET_ROUTE, ( ipcarg_t ) protocol, & answer );
	if(( async_data_write_start( ip_phone, destination, addrlen ) == EOK )
	&& ( async_data_read_start( ip_phone, headerlen, sizeof( * headerlen )) == EOK )
	&& ( * headerlen > 0 )){
		* header = ( ip_pseudo_header_ref ) malloc( * headerlen );
		if( * header ){
			if( async_data_read_start( ip_phone, * header, * headerlen ) != EOK ){
				free( * header );
			}
		}
	}
	async_wait_for( message_id, & result );
	if(( result != EOK ) && ( * header )){
		free( * header );
	}else{
		* device_id = IPC_GET_DEVICE( & answer );
	}
	return ( int ) result;
}

/** @}
 */
