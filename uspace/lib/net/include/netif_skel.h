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

/** @file
 * Network interface module skeleton.
 * The skeleton has to be part of each network interface module.
 */

#ifndef NET_NETIF_SKEL_H_
#define NET_NETIF_SKEL_H_

#include <async.h>
#include <fibril_synch.h>
#include <ipc/services.h>

#include <adt/measured_strings.h>
#include <net/device.h>
#include <net/packet.h>

/** Network interface device specific data. */
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

/** Network interface module skeleton global data. */
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
 * @param[in] device_id Device identifier.
 * @param[in] irq       Device interrupt number.
 * @param[in] io        Device input/output address.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device()
 *         function.
 * @return Other error codes as defined for the specific module
 *         message implementation.
 *
 */
extern int netif_probe_message(device_id_t device_id, int irq, void *io);

/** Send the packet queue.
 *
 * This has to be implemented in user code.
 *
 * @param[in] device_id Device identifier.
 * @param[in] packet    Packet queue.
 * @param[in] sender    Sending module service.
 *
 * @return EOK on success.
 * @return EFORWARD if the device is not active (in the
 *         NETIF_ACTIVE state).
 * @return Other error codes as defined for the find_device()
 *         function.
 * @return Other error codes as defined for the specific module
 *         message implementation.
 *
 */
extern int netif_send_message(device_id_t device_id, packet_t *packet,
    services_t sender);

/** Start the device.
 *
 * This has to be implemented in user code.
 *
 * @param[in] device Device structure.
 *
 * @return New network interface state (non-negative values).
 * @return Other error codes as defined for the find_device()
 *         function.
 * @return Other error codes as defined for the specific module
 *         message implementation.
 
 *
 */
extern int netif_start_message(netif_device_t *device);

/** Stop the device.
 *
 * This has to be implemented in user code.
 *
 * @param[in] device Device structure.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device()
 *         function.
 * @return Other error codes as defined for the specific module
 *         message implementation.
 *
 */
extern int netif_stop_message(netif_device_t *device);

/** Return the device local hardware address.
 *
 * This has to be implemented in user code.
 *
 * @param[in] device_id Device identifier.
 * @param[out] address  Device local hardware address.
 *
 * @return EOK on success.
 * @return EBADMEM if the address parameter is NULL.
 * @return ENOENT if there no such device.
 * @return Other error codes as defined for the find_device()
 *         function.
 * @return Other error codes as defined for the specific module
 *         message implementation.
 *
 */
extern int netif_get_addr_message(device_id_t device_id,
    measured_string_t *address);

/** Process the netif driver specific message.
 *
 * This function is called for uncommon messages received by the netif
 * skeleton. This has to be implemented in user code.
 *
 * @param[in]  callid Message identifier.
 * @param[in]  call   Message.
 * @param[out] answer Answer.
 * @param[out] count  Number of answer arguments.
 *
 * @return EOK on success.
 * @return ENOTSUP if the message is not known.
 * @return Other error codes as defined for the specific module
 *         message implementation.
 *
 */
extern int netif_specific_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, size_t *count);

/** Return the device usage statistics.
 *
 * This has to be implemented in user code.
 *
 * @param[in]  device_id Device identifier.
 * @param[out] stats     Device usage statistics.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device()
 *         function.
 * @return Other error codes as defined for the specific module
 *         message implementation.
 *
 */
extern int netif_get_device_stats(device_id_t device_id,
    device_stats_t *stats);

extern int find_device(device_id_t, netif_device_t **);
extern void null_device_stats(device_stats_t *);
extern void netif_pq_release(packet_id_t);
extern packet_t *netif_packet_get_1(size_t);

extern int netif_module_start(void);

#endif

/** @}
 */
