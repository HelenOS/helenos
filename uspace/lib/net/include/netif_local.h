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
 * @{
 */

/** @file
 * Network interface module skeleton.
 * The skeleton has to be part of each network interface module.
 * The skeleton can be also part of the module bundled with the network interface layer.
 */

#ifndef __NET_NETIF_LOCAL_H__
#define __NET_NETIF_LOCAL_H__

#include <async.h>
#include <fibril_synch.h>
#include <ipc/ipc.h>
#include <ipc/services.h>

#include <adt/measured_strings.h>
#include <net_err.h>
#include <net_device.h>
#include <packet/packet.h>

/** Network interface device specific data.
 *
 */
typedef struct {
	device_id_t device_id;  /**< Device identifier. */
	int nil_phone;          /**< Receiving network interface layer phone. */
	device_state_t state;   /**< Actual device state. */
	void *specific;         /**< Driver specific data. */
} netif_device_t;

/** Device map.
 *
 * Maps device identifiers to the network interface device specific data.
 * @see device.h
 *
 */
DEVICE_MAP_DECLARE(netif_device_map, netif_device_t);

/** Network interface module skeleton global data.
 *
 */
typedef struct {
	int net_phone;                  /**< Networking module phone. */
	netif_device_map_t device_map;  /**< Device map. */
	fibril_rwlock_t lock;           /**< Safety lock. */
} netif_globals_t;

extern netif_globals_t netif_globals;

/** Initialize the specific module.
 *
 * This function has to be implemented in user code.
 *
 */
extern int netif_initialize(void);

/** Probe the existence of the device.
 *
 * This has to be implemented in user code.
 *
 * @param[in] device_id The device identifier.
 * @param[in] irq       The device interrupt number.
 * @param[in] io        The device input/output address.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device() function.
 * @return Other error codes as defined for the specific module message
 *         implementation.
 *
 */
extern int netif_probe_message(device_id_t device_id, int irq, uintptr_t io);

/** Send the packet queue.
 *
 * This has to be implemented in user code.
 *
 * @param[in] device_id The device identifier.
 * @param[in] packet    The packet queue.
 * @param[in] sender    The sending module service.
 *
 * @return EOK on success.
 * @return EFORWARD if the device is not active (in the NETIF_ACTIVE state).
 * @return Other error codes as defined for the find_device() function.
 * @return Other error codes as defined for the specific module message
 *         implementation.
 *
 */
extern int netif_send_message(device_id_t device_id, packet_t packet,
    services_t sender);

/** Start the device.
 *
 * This has to be implemented in user code.
 *
 * @param[in] device The device structure.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device() function.
 * @return Other error codes as defined for the specific module message
 *         implementation.
 *
 */
extern int netif_start_message(netif_device_t *device);

/** Stop the device.
 *
 * This has to be implemented in user code.
 *
 * @param[in] device The device structure.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device() function.
 * @return Other error codes as defined for the specific module message
 *         implementation.
 *
 */
extern int netif_stop_message(netif_device_t *device);

/** Return the device local hardware address.
 *
 * This has to be implemented in user code.
 *
 * @param[in]  device_id The device identifier.
 * @param[out] address   The device local hardware address.
 *
 * @return EOK on success.
 * @return EBADMEM if the address parameter is NULL.
 * @return ENOENT if there no such device.
 * @return Other error codes as defined for the find_device() function.
 * @return Other error codes as defined for the specific module message
 *         implementation.
 *
 */
extern int netif_get_addr_message(device_id_t device_id,
    measured_string_ref address);

/** Process the netif driver specific message.
 *
 * This function is called for uncommon messages received by the netif
 * skeleton. This has to be implemented in user code.
 *
 * @param[in]  callid       The message identifier.
 * @param[in]  call         The message parameters.
 * @param[out] answer       The message answer parameters.
 * @param[out] answer_count The last parameter for the actual answer in
 *                          the answer parameter.
 *
 * @return EOK on success.
 * @return ENOTSUP if the message is not known.
 * @return Other error codes as defined for the specific module message
 *         implementation.
 *
 */
extern int netif_specific_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, int *answer_count);

/** Return the device usage statistics.
 *
 * This has to be implemented in user code.
 *
 * @param[in]  device_id The device identifier.
 * @param[out] stats     The device usage statistics.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device() function.
 * @return Other error codes as defined for the specific module message
 *         implementation.
 *
 */
extern int netif_get_device_stats(device_id_t device_id,
    device_stats_ref stats);

extern int netif_get_addr_req_local(int, device_id_t, measured_string_ref *,
    char **);
extern int netif_probe_req_local(int, device_id_t, int, int);
extern int netif_send_msg_local(int, device_id_t, packet_t, services_t);
extern int netif_start_req_local(int, device_id_t);
extern int netif_stop_req_local(int, device_id_t);
extern int netif_stats_req_local(int, device_id_t, device_stats_ref);
extern int netif_bind_service_local(services_t, device_id_t, services_t,
    async_client_conn_t);

extern int find_device(device_id_t, netif_device_t **);
extern void null_device_stats(device_stats_ref);
extern void netif_pq_release(packet_id_t);
extern packet_t netif_packet_get_1(size_t);
extern int netif_init_module(async_client_conn_t);

extern int netif_module_message_standalone(const char *, ipc_callid_t,
    ipc_call_t *, ipc_call_t *, int *);
extern int netif_module_start_standalone(async_client_conn_t);

#endif

/** @}
 */
