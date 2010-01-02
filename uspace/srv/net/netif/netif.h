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
 *  Network interface module skeleton.
 *  The skeleton has to be part of each network interface module.
 *  The skeleton can be also part of the module bundled with the network interface layer.
 */

#ifndef __NET_NETIF_H__
#define __NET_NETIF_H__

#include <async.h>
#include <fibril_synch.h>

#include <ipc/ipc.h>

#include "../err.h"

#include "../include/device.h"

#include "../structures/packet/packet.h"

/** Network interface module skeleton global data.
 */
typedef struct netif_globals	netif_globals_t;

/** Type definition of the device specific data.
 *  @see netif_device
 */
typedef struct netif_device	device_t;

/** Type definition of the device specific data pointer.
 *  @see netif_device
 */
typedef device_t *			device_ref;

/** Device map.
 *  Maps device identifiers to the network interface device specific data.
 *  @see device.h
 */
DEVICE_MAP_DECLARE( device_map, device_t );

/** Network interface device specific data.
 */
struct	netif_device{
	/** Device identifier.
	 */
	device_id_t	device_id;
	/** Receiving network interface layer phone.
	 */
	int		nil_phone;
	/** Actual device state.
	 */
	device_state_t	state;
	/** Driver specific data.
	 */
	void *		specific;
};

/** Network interface module skeleton global data.
 */
struct	netif_globals{
	/** Networking module phone.
	 */
	int		net_phone;
	/**	Device map.
	 */
	device_map_t	device_map;
	/** Safety lock.
	 */
	fibril_rwlock_t	lock;
};

/**	Finds the device specific data.
 *  @param[in] device_id The device identifier.
 *  @param[out] device The device specific data.
 *  @returns EOK on success.
 *  @returns ENOENT if device is not found.
 *  @returns EPERM if the device is not initialized.
 */
int	find_device( device_id_t device_id, device_ref * device );

/** Clears the usage statistics.
 *  @param[in] stats The usage statistics.
 */
void	null_device_stats( device_stats_ref stats );

// prepared for future optimalizations
/** Releases the given packet.
 *  @param[in] packet_id The packet identifier.
 */
void	netif_pq_release( packet_id_t packet_id );

/** Allocates new packet to handle the given content size.
 *  @param[in] content The minimum content size.
 *  @returns The allocated packet.
 *  @returns NULL if there is an error.
 */
packet_t netif_packet_get_1( size_t content );

/** Processes the netif module messages.
 *  @param[in] callid The message identifier.
 *  @param[in] call The message parameters.
 *  @param[out] answer The message answer parameters.
 *  @param[out] answer_count The last parameter for the actual answer in the answer parameter.
 *  @returns EOK on success.
 *  @returns ENOTSUP if the message is not known.
 *  @returns Other error codes as defined for each specific module message function.
 *  @see netif_interface.h
 *  @see IS_NET_NETIF_MESSAGE()
 */
int	netif_message( ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count );

/** Initializes the netif module.
 *  The function has to be defined in each module.
 *  @param[in] client_connection The client connection functio to be registered.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for each specific module message function.
 */
int	netif_init_module( async_client_conn_t client_connection );

/** Starts and maintains the netif module until terminated.
 *  @returns EOK after the module is terminated.
 */
int netif_run_module( void );

#endif

/** @}
 */

