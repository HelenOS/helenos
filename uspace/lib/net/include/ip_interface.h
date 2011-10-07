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

/** @addtogroup libnet 
 * @{
 */

#ifndef LIBNET_IP_INTERFACE_H_
#define LIBNET_IP_INTERFACE_H_

#include <net/socket_codes.h>
#include <ipc/services.h>
#include <net/device.h>
#include <net/packet.h>
#include <net/in.h>
#include <net/ip_codes.h>
#include <ip_remote.h>
#include <async.h>

#define ip_received_error_msg  ip_received_error_msg_remote
#define ip_set_gateway_req     ip_set_gateway_req_remote
#define ip_packet_size_req     ip_packet_size_req_remote
#define ip_device_req          ip_device_req_remote
#define ip_add_route_req       ip_add_route_req_remote
#define ip_send_msg            ip_send_msg_remote
#define ip_get_route_req       ip_get_route_req_remote

/** @name IP module interface
 * This interface is used by other modules.
 */
/*@{*/

/** The transport layer notification function type definition.
 *
 * Notify the transport layer modules about the received packet/s.
 *
 * @param[in] device_id Device identifier.
 * @param[in] packet    Received packet or the received packet queue.
 * @param[in] receiver  Receiving module service.
 * @param[in] error     Packet error reporting service. Prefixes the
 *                      received packet.
 *
 * @return EOK on success.
 *
 */
typedef int (*tl_received_msg_t)(nic_device_id_t device_id, packet_t *packet,
    services_t receiver, services_t error);

extern async_sess_t *ip_bind_service(services_t, int, services_t, async_client_conn_t);
extern async_sess_t *ip_connect_module(services_t);

/*@}*/

#endif

/** @}
 */
