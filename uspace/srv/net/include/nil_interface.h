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

/** @file
 *  Network interface layer module interface.
 *  The same interface is used for standalone remote device modules as well as for bundle device modules.
 *  The standalone remote device modules have to be compiled with the nil_remote.c source file.
 *  The bundle device modules with the appropriate network interface layer source file (eth.c etc.).
 *  The upper layers cannot be bundled with the network interface layer.
 */

#ifndef __NET_NIL_INTERFACE_H__
#define __NET_NIL_INTERFACE_H__

#include <async.h>
#include <errno.h>

#include <ipc/ipc.h>

#include "../messages.h"

#include "../structures/measured_strings.h"
#include "../structures/packet/packet.h"

#include "../nil/nil_messages.h"

#include "device.h"

/** @name Network interface layer module interface
 *  This interface is used by other modules.
 */
/*@{*/

/** Returns the device local hardware address.
 *  @param[in] nil_phone The network interface layer phone.
 *  @param[in] device_id The device identifier.
 *  @param[out] address The device local hardware address.
 *  @param[out] data The address data.
 *  @returns EOK on success.
 *  @returns EBADMEM if the address parameter and/or the data parameter is NULL.
 *  @returns ENOENT if there no such device.
 *  @returns Other error codes as defined for the generic_get_addr_req() function.
 */
#define nil_get_addr_req(nil_phone, device_id, address, data)	\
	generic_get_addr_req(nil_phone, NET_NIL_ADDR, device_id, address, data)

/** Returns the device broadcast hardware address.
 *  @param[in] nil_phone The network interface layer phone.
 *  @param[in] device_id The device identifier.
 *  @param[out] address The device broadcast hardware address.
 *  @param[out] data The address data.
 *  @returns EOK on success.
 *  @returns EBADMEM if the address parameter is NULL.
 *  @returns ENOENT if there no such device.
 *  @returns Other error codes as defined for the generic_get_addr_req() function.
 */
#define nil_get_broadcast_addr_req(nil_phone, device_id, address, data)	\
	generic_get_addr_req(nil_phone, NET_NIL_BROADCAST_ADDR, device_id, address, data)

/** Sends the packet queue.
 *  @param[in] nil_phone The network interface layer phone.
 *  @param[in] device_id The device identifier.
 *  @param[in] packet The packet queue.
 *  @param[in] sender The sending module service.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the generic_send_msg() function.
 */
#define nil_send_msg(nil_phone, device_id, packet, sender)	\
	generic_send_msg(nil_phone, NET_NIL_SEND, device_id, packet_get_id(packet), sender, 0)

/** Returns the device packet dimension for sending.
 *  @param[in] nil_phone The network interface layer phone.
 *  @param[in] device_id The device identifier.
 *  @param[out] packet_dimension The packet dimensions.
 *  @returns EOK on success.
 *  @returns ENOENT if there is no such device.
 *  @returns Other error codes as defined for the generic_packet_size_req() function.
 */
#define nil_packet_size_req(nil_phone, device_id, packet_dimension)	\
	generic_packet_size_req(nil_phone, NET_NIL_PACKET_SPACE, device_id, packet_dimension)

/** Registers new device or updates the MTU of an existing one.
 *  @param[in] nil_phone The network interface layer phone.
 *  @param[in] device_id The new device identifier.
 *  @param[in] mtu The device maximum transmission unit.
 *  @param[in] netif_service The device driver service.
 *  @returns EOK on success.
 *  @returns EEXIST if the device with the different service exists.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the generic_device_req() function.
 */
#define nil_device_req(nil_phone, device_id, mtu, netif_service)	\
	generic_device_req(nil_phone, NET_NIL_DEVICE, device_id, mtu, netif_service)

/** Notifies the network interface layer about the device state change.
 *  @param[in] nil_phone The network interface layer phone.
 *  @param[in] device_id The device identifier.
 *  @param[in] state The new device state.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for each specific module device state function.
 */
int nil_device_state_msg(int nil_phone, device_id_t device_id, int state);

/** Passes the packet queue to the network interface layer.
 *  Processes and redistributes the received packet queue to the registered upper layers.
 *  @param[in] nil_phone The network interface layer phone.
 *  @param[in] device_id The source device identifier.
 *  @param[in] packet The received packet or the received packet queue.
 *  @param target The target service. Ignored parameter.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for each specific module received function.
 */
int nil_received_msg(int nil_phone, device_id_t device_id, packet_t packet, services_t target);

/** Creates bidirectional connection with the network interface layer module and registers the message receiver.
 *  @param[in] service The network interface layer module service.
 *  @param[in] device_id The device identifier.
 *  @param[in] me The requesting module service.
 *  @param[in] receiver The message receiver.
 *  @returns The phone of the needed service.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the bind_service() function.
 */
#define	nil_bind_service(service, device_id, me, receiver)	\
	bind_service(service, device_id, me, 0, receiver);
/*@}*/

#endif

/** @}
 */
