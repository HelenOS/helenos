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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Wrapper for the bundled networking and TCP/IP stact modules.
 *  Distributes messages and initializes all module parts.
 */

#include <string.h>

#include <ipc/ipc.h>

#include "../messages.h"

#include "../include/ip_interface.h"

#include "../structures/measured_strings.h"
#include "../structures/module_map.h"
#include "../structures/packet/packet_server.h"

#include "../il/arp/arp_module.h"
#include "../il/ip/ip_module.h"
#include "../tl/icmp/icmp_module.h"
#include "../tl/udp/udp_module.h"
#include "../tl/tcp/tcp_module.h"

#include "net.h"

/** Networking module global data.
 */
extern net_globals_t	net_globals;

int module_message( ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count ){
	if(( IPC_GET_METHOD( * call ) == IPC_M_CONNECT_TO_ME )
	|| IS_NET_IL_MESSAGE( call )
	|| IS_NET_TL_MESSAGE( call )
	|| IS_NET_SOCKET_MESSAGE( call )){
		switch( IPC_GET_TARGET( call )){
			case SERVICE_IP:
				return ip_message( callid, call, answer, answer_count );
			case SERVICE_ARP:
				return arp_message( callid, call, answer, answer_count );
			case SERVICE_ICMP:
				return icmp_message( callid, call, answer, answer_count );
			case SERVICE_UDP:
				return udp_message( callid, call, answer, answer_count );
			case SERVICE_TCP:
				return tcp_message( callid, call, answer, answer_count );
			default:
				return EINVAL;
		}
	}else if( IS_NET_IP_MESSAGE( call )){
		return ip_message( callid, call, answer, answer_count );
	}else if( IS_NET_ARP_MESSAGE( call )){
		return arp_message( callid, call, answer, answer_count );
	}else if( IS_NET_ICMP_MESSAGE( call )){
		return icmp_message( callid, call, answer, answer_count );
	}else if( IS_NET_UDP_MESSAGE( call )){
		return udp_message( callid, call, answer, answer_count );
	}else if( IS_NET_TCP_MESSAGE( call )){
		return tcp_message( callid, call, answer, answer_count );
	}else{
		if( IS_NET_PACKET_MESSAGE( call )){
			return packet_server_message( callid, call, answer, answer_count );
		}else{
			return net_message( callid, call, answer, answer_count );
		}
	}
}

int net_initialize_build( async_client_conn_t client_connection ){
	ERROR_DECLARE;

	ipcarg_t	phonehash;

	ERROR_PROPAGATE( REGISTER_ME( SERVICE_IP, & phonehash ));
	ERROR_PROPAGATE( add_module( NULL, & net_globals.modules, IP_NAME, IP_FILENAME, SERVICE_IP, task_get_id(), ip_connect_module ));
	ERROR_PROPAGATE( ip_initialize( client_connection ));
	ERROR_PROPAGATE( REGISTER_ME( SERVICE_ARP, & phonehash ));
	ERROR_PROPAGATE( arp_initialize( client_connection ));
	ERROR_PROPAGATE( REGISTER_ME( SERVICE_ICMP, & phonehash ));
	ERROR_PROPAGATE( icmp_initialize( client_connection ));
	ERROR_PROPAGATE( REGISTER_ME( SERVICE_UDP, & phonehash ));
	ERROR_PROPAGATE( udp_initialize( client_connection ));
	ERROR_PROPAGATE( REGISTER_ME( SERVICE_TCP, & phonehash ));
	ERROR_PROPAGATE( tcp_initialize( client_connection ));
	return EOK;
}

/** @}
 */
