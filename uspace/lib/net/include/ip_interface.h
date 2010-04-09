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

#ifndef __NET_IP_INTERFACE_H__
#define __NET_IP_INTERFACE_H__

#include <async.h>
#include <ipc/services.h>

#include <net_device.h>
#include <packet/packet.h>

#include <in.h>
#include <ip_codes.h>
#include <socket_codes.h>

#ifdef CONFIG_IL_TL_BUNDLE

#include <ip_local.h>

#define ip_received_error_msg  ip_received_error_msg_local
#define ip_set_gateway_req     ip_set_gateway_req_local
#define ip_packet_size_req     ip_packet_size_req_local
#define ip_device_req          ip_device_req_local
#define ip_add_route_req       ip_add_route_req_local
#define ip_send_msg            ip_send_msg_local
#define ip_get_route_req       ip_get_route_req_local

#else

#include <ip_remote.h>

#define ip_received_error_msg  ip_received_error_msg_remote
#define ip_set_gateway_req     ip_set_gateway_req_remote
#define ip_packet_size_req     ip_packet_size_req_remote
#define ip_device_req          ip_device_req_remote
#define ip_add_route_req       ip_add_route_req_remote
#define ip_send_msg            ip_send_msg_remote
#define ip_get_route_req       ip_get_route_req_remote

#endif

/** @name IP module interface
 *  This interface is used by other modules.
 */
/*@{*/

/** The transport layer notification function type definition.
 *  Notifies the transport layer modules about the received packet/s.
 *  @param[in] device_id The device identifier.
 *  @param[in] packet The received packet or the received packet queue.
 *  @param[in] receiver The receiving module service.
 *  @param[in] error The packet error reporting service. Prefixes the received packet.
 *  @returns EOK on success.
 */
typedef int	(*tl_received_msg_t)(device_id_t device_id, packet_t packet, services_t receiver, services_t error);

/** Creates bidirectional connection with the ip module service and registers the message receiver.
 *  @param[in] service The IP module service.
 *  @param[in] protocol The transport layer protocol.
 *  @param[in] me The requesting module service.
 *  @param[in] receiver The message receiver. Used for remote connection.
 *  @param[in] tl_received_msg The message processing function. Used if bundled together.
 *  @returns The phone of the needed service.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the bind_service() function.
 */
extern int ip_bind_service(services_t service, int protocol, services_t me, async_client_conn_t receiver, tl_received_msg_t tl_received_msg);

/** Connects to the IP module.
 *  @param service The IP module service. Ignored parameter.
 *  @returns The IP module phone on success.
 *  @returns 0 if called by the bundle module.
 */
extern int ip_connect_module(services_t service);

/*@}*/

#endif

/** @}
 */
