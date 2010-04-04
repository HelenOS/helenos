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
 *  Network interface module interface.
 *  The same interface is used for standalone remote modules as well as for bundle network interface layer modules.
 *  The standalone remote modules have to be compiled with the netif_remote.c source file.
 *  The bundle network interface layer modules are compiled with the netif_nil_bundle.c source file and the choosen network interface layer implementation source file.
 */

#ifndef __NET_NETIF_INTERFACE_H__
#define __NET_NETIF_INTERFACE_H__

#include <ipc/services.h>

#include <net_messages.h>
#include <adt/measured_strings.h>
#include <packet/packet.h>
#include <net_device.h>

/** @name Network interface module interface
 *  This interface is used by other modules.
 */
/*@{*/

/** Returns the device local hardware address.
 *  @param[in] netif_phone The network interface phone.
 *  @param[in] device_id The device identifier.
 *  @param[out] address The device local hardware address.
 *  @param[out] data The address data.
 *  @returns EOK on success.
 *  @returns EBADMEM if the address parameter is NULL.
 *  @returns ENOENT if there no such device.
 *  @returns Other error codes as defined for the netif_get_addr_message() function.
 */
extern int netif_get_addr_req(int netif_phone, device_id_t device_id, measured_string_ref * address, char ** data);

/** Probes the existence of the device.
 *  @param[in] netif_phone The network interface phone.
 *  @param[in] device_id The device identifier.
 *  @param[in] irq The device interrupt number.
 *  @param[in] io The device input/output address.
 *  @returns EOK on success.
 *  @returns Other errro codes as defined for the netif_probe_message().
 */
extern int netif_probe_req(int netif_phone, device_id_t device_id, int irq, int io);

/** Sends the packet queue.
 *  @param[in] netif_phone The network interface phone.
 *  @param[in] device_id The device identifier.
 *  @param[in] packet The packet queue.
 *  @param[in] sender The sending module service.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the generic_send_msg() function.
 */
extern int netif_send_msg(int netif_phone, device_id_t device_id, packet_t packet, services_t sender);

/** Starts the device.
 *  @param[in] netif_phone The network interface phone.
 *  @param[in] device_id The device identifier.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the find_device() function.
 *  @returns Other error codes as defined for the netif_start_message() function.
 */
extern int netif_start_req(int netif_phone, device_id_t device_id);

/** Stops the device.
 *  @param[in] netif_phone The network interface phone.
 *  @param[in] device_id The device identifier.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the find_device() function.
 *  @returns Other error codes as defined for the netif_stop_message() function.
 */
extern int netif_stop_req(int netif_phone, device_id_t device_id);

/** Returns the device usage statistics.
 *  @param[in] netif_phone The network interface phone.
 *  @param[in] device_id The device identifier.
 *  @param[out] stats The device usage statistics.
 *  @returns EOK on success.
 */
extern int netif_stats_req(int netif_phone, device_id_t device_id, device_stats_ref stats);

/** Creates bidirectional connection with the network interface module and registers the message receiver.
 *  @param[in] service The network interface module service.
 *  @param[in] device_id The device identifier.
 *  @param[in] me The requesting module service.
 *  @param[in] receiver The message receiver.
 *  @returns The phone of the needed service.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the bind_service() function.
 */
extern int netif_bind_service(services_t service, device_id_t device_id, services_t me, async_client_conn_t receiver);

/*@}*/

#endif

/** @}
 */
