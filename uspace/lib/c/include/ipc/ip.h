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
 * @{
 */

/** @file
 * IP module messages.
 * @see ip_interface.h
 */

#ifndef LIBC_IP_MESSAGES_H_
#define LIBC_IP_MESSAGES_H_

#include <ipc/net.h>
#include <net/in.h>
#include <net/ip_codes.h>

/** IP module messages. */
typedef enum {
	/** New device message.
	 * @see ip_device_req()
	 */
	NET_IP_DEVICE = NET_IP_FIRST,
	
	/** Adds the routing entry.
	 * @see ip_add_route()
	 */
	NET_IP_ADD_ROUTE,
	
	/** Gets the actual route information.
	 * @see ip_get_route()
	 */
	NET_IP_GET_ROUTE,
	
	/** Processes the received error notification.
	 * @see ip_received_error_msg()
	 */
	NET_IP_RECEIVED_ERROR,
	
	/** Sets the default gateway.
	 * @see ip_set_default_gateway()
	 */
	NET_IP_SET_GATEWAY,
	
	/** Packet size message.
	 * @see ip_packet_size_req()
	 */
	NET_IP_PACKET_SPACE,
	
	/** Packet send message.
	 * @see ip_send_msg()
	 */
	NET_IP_SEND
} ip_messages;

/** @name IP specific message parameters definitions */
/*@{*/

/** Return the address message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define IP_GET_ADDRESS(call) \
	({ \
		in_addr_t addr; \
		addr.s_addr = IPC_GET_ARG3(call); \
		addr; \
	})

/** Return the gateway message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define IP_GET_GATEWAY(call) \
	({ \
		in_addr_t addr; \
		addr.s_addr = IPC_GET_ARG2(call); \
		addr; \
	})

/** Set the header length in the message answer.
 *
 * @param[out] answer Message answer structure.
 *
 */
#define IP_SET_HEADERLEN(answer, value)  IPC_SET_ARG2(answer, (sysarg_t) (value))

/** Return the network mask message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define IP_GET_NETMASK(call) \
	({ \
		in_addr_t addr; \
		addr.s_addr = IPC_GET_ARG4(call); \
		addr; \
	})

/** Return the protocol message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define IP_GET_PROTOCOL(call)  ((ip_protocol_t) IPC_GET_ARG1(call))

/*@}*/

#endif

/** @}
 */
