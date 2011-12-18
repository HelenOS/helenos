/*
 * Copyright (c) 2009 Lukas Mejdrech
 * Copyright (c) 2011 Radim Vansa
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

/** @addtogroup nildummy
 * @{
 */

/** @file
 * Dummy network interface layer module.
 */

#ifndef NET_NILDUMMY_H_
#define NET_NILDUMMY_H_

#include <async.h>
#include <fibril_synch.h>
#include <ipc/services.h>
#include <ipc/devman.h>
#include <net/device.h>

/** Type definition of the dummy nil global data.
 *
 * @see nildummy_globals
 *
 */
typedef struct nildummy_globals nildummy_globals_t;

/** Type definition of the dummy nil device specific data.
 *
 * @see nildummy_device
 *
 */
typedef struct nildummy_device nildummy_device_t;

/** Type definition of the dummy nil protocol specific data.
 *
 * @see nildummy_proto
 *
 */
typedef struct nildummy_proto nildummy_proto_t;

/** Dummy nil device map.
 *
 * Map devices to the dummy nil device specific data.
 * @see device.h
 *
 */
DEVICE_MAP_DECLARE(nildummy_devices, nildummy_device_t);

/** Dummy nil device specific data. */
struct nildummy_device {
	/** Device identifier. */
	nic_device_id_t device_id;
	/** Device driver handle. */
	devman_handle_t handle;
	/** Driver session. */
	async_sess_t *sess;
	
	/** Maximal transmission unit. */
	size_t mtu;
	
	/** Actual device hardware address. */
	nic_address_t addr;
	/** Actual device hardware address length. */
	size_t addr_len;
};

/** Dummy nil protocol specific data. */
struct nildummy_proto {
	/** Protocol service. */
	services_t service;
	
	/** Protocol module session. */
	async_sess_t *sess;
};

/** Dummy nil global data. */
struct nildummy_globals {
	/** Networking module session. */
	async_sess_t *net_sess;
	
	/** Lock for devices. */
	fibril_rwlock_t devices_lock;
	
	/** All known Ethernet devices. */
	nildummy_devices_t devices;
	
	/** Safety lock for protocols. */
	fibril_rwlock_t protos_lock;
	
	/** Default protocol. */
	nildummy_proto_t proto;
};

#endif

/** @}
 */
