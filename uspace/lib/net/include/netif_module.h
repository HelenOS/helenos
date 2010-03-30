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
 *  The interface has to be implemented by each network interface module.
 *  The interface is used by the network interface module skeleton.
 */

#ifndef __NET_NETIF_MODULE_H__
#define __NET_NETIF_MODULE_H__

#include <ipc/ipc.h>
#include <ipc/services.h>

#include <adt/measured_strings.h>
#include <packet/packet.h>
#include <net_device.h>

/** Initializes the specific module.
 */
extern int netif_initialize(void);

/** Probes the existence of the device.
 *  @param[in] device_id The device identifier.
 *  @param[in] irq The device interrupt number.
 *  @param[in] io The device input/output address.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the find_device() function.
 *  @returns Other error codes as defined for the specific module message implementation.
 */
extern int netif_probe_message(device_id_t device_id, int irq, uintptr_t io);

/** Sends the packet queue.
 *  @param[in] device_id The device identifier.
 *  @param[in] packet The packet queue.
 *  @param[in] sender The sending module service.
 *  @returns EOK on success.
 *  @returns EFORWARD if the device is not active (in the NETIF_ACTIVE state).
 *  @returns Other error codes as defined for the find_device() function.
 *  @returns Other error codes as defined for the specific module message implementation.
 */
extern int netif_send_message(device_id_t device_id, packet_t packet, services_t sender);

/** Starts the device.
 *  @param[in] device The device structure.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the find_device() function.
 *  @returns Other error codes as defined for the specific module message implementation.
 */
extern int netif_start_message(device_ref device);

/** Stops the device.
 *  @param[in] device The device structure.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the find_device() function.
 *  @returns Other error codes as defined for the specific module message implementation.
 */
extern int netif_stop_message(device_ref device);

/** Returns the device local hardware address.
 *  @param[in] device_id The device identifier.
 *  @param[out] address The device local hardware address.
 *  @returns EOK on success.
 *  @returns EBADMEM if the address parameter is NULL.
 *  @returns ENOENT if there no such device.
 *  @returns Other error codes as defined for the find_device() function.
 *  @returns Other error codes as defined for the specific module message implementation.
 */
extern int netif_get_addr_message(device_id_t device_id, measured_string_ref address);

/** Processes the netif driver specific message.
 *  This function is called for uncommon messages received by the netif skeleton.
 *  @param[in] callid The message identifier.
 *  @param[in] call The message parameters.
 *  @param[out] answer The message answer parameters.
 *  @param[out] answer_count The last parameter for the actual answer in the answer parameter.
 *  @returns EOK on success.
 *  @returns ENOTSUP if the message is not known.
 *  @returns Other error codes as defined for the specific module message implementation.
 */
extern int netif_specific_message(ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count);

/** Returns the device usage statistics.
 *  @param[in] device_id The device identifier.
 *  @param[out] stats The device usage statistics.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the find_device() function.
 *  @returns Other error codes as defined for the specific module message implementation.
 */
extern int netif_get_device_stats(device_id_t device_id, device_stats_ref stats);

#endif

/** @}
 */
