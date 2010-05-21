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

/** @addtogroup net_nil
 *  @{
 */

#ifndef __NET_NIL_INTERFACE_H__
#define __NET_NIL_INTERFACE_H__

#include <async.h>
#include <errno.h>

#include <ipc/ipc.h>

#include <net_messages.h>
#include <adt/measured_strings.h>
#include <packet/packet.h>
#include <nil_messages.h>
#include <net_device.h>

#define nil_bind_service(service, device_id, me, receiver) \
	bind_service(service, device_id, me, 0, receiver)

#define nil_packet_size_req(nil_phone, device_id, packet_dimension) \
	generic_packet_size_req_remote(nil_phone, NET_NIL_PACKET_SPACE, device_id, \
	    packet_dimension)

#define nil_get_addr_req(nil_phone, device_id, address, data) \
	generic_get_addr_req(nil_phone, NET_NIL_ADDR, device_id, address, data)

#define nil_get_broadcast_addr_req(nil_phone, device_id, address, data) \
	generic_get_addr_req(nil_phone, NET_NIL_BROADCAST_ADDR, device_id, \
	    address, data)

#define nil_send_msg(nil_phone, device_id, packet, sender) \
	generic_send_msg_remote(nil_phone, NET_NIL_SEND, device_id, \
	    packet_get_id(packet), sender, 0)

#define nil_device_req(nil_phone, device_id, mtu, netif_service) \
	generic_device_req_remote(nil_phone, NET_NIL_DEVICE, device_id, mtu, \
	    netif_service)


#ifdef CONFIG_NETIF_NIL_BUNDLE

#include <nil_local.h>
#include <packet/packet_server.h>

#define nil_device_state_msg  nil_device_state_msg_local
#define nil_received_msg      nil_received_msg_local

#else /* CONFIG_NETIF_NIL_BUNDLE */

#include <nil_remote.h>
#include <packet/packet_server.h>

#define nil_device_state_msg  nil_device_state_msg_remote
#define nil_received_msg      nil_received_msg_remote

#endif /* CONFIG_NETIF_NIL_BUNDLE */

#endif

/** @}
 */
