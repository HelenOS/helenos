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

/** @addtogroup netif
 *  @{
 */

/** @file
 *  Network interface module interface implementation for standalone remote modules.
 *  @see netif_interface.h
 */

#include <ipc/services.h>

#include "../modules.h"

#include "../structures/measured_strings.h"
#include "../structures/packet/packet.h"
#include "../structures/packet/packet_client.h"

#include "../include/device.h"
#include "../include/netif_interface.h"

#include "netif_messages.h"

int	netif_get_addr_req( int netif_phone, device_id_t device_id, measured_string_ref * address, char ** data ){
	return generic_get_addr_req( netif_phone, NET_NETIF_GET_ADDR, device_id, address, data );
}

int	netif_probe_req( int netif_phone, device_id_t device_id, int irq, int io ){
	return async_req_3_0( netif_phone, NET_NETIF_PROBE, device_id, irq, io );
}

int	netif_send_msg( int netif_phone, device_id_t device_id, packet_t packet, services_t sender ){
	return generic_send_msg( netif_phone, NET_NETIF_SEND, device_id, packet_get_id( packet ), sender, 0 );
}

int	netif_start_req( int netif_phone, device_id_t device_id ){
	return async_req_1_0( netif_phone, NET_NETIF_START, device_id );
}

int	netif_stop_req( int netif_phone, device_id_t device_id ){
	return async_req_1_0( netif_phone, NET_NETIF_STOP, device_id );
}

int	netif_stats_req( int netif_phone, device_id_t device_id, device_stats_ref stats ){
	aid_t		message_id;
	ipcarg_t	result;

	if( ! stats ) return EBADMEM;
	message_id = async_send_1( netif_phone, NET_NETIF_STATS, ( ipcarg_t ) device_id, NULL );
	async_data_read_start( netif_phone, stats, sizeof( * stats ));
	async_wait_for( message_id, & result );
	return ( int ) result;
}

int netif_bind_service( services_t service, device_id_t device_id, services_t me, async_client_conn_t receiver ){
	return bind_service( service, device_id, me, 0, receiver );
}

/** @}
 */
