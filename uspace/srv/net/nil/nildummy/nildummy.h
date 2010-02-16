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

/** @addtogroup nildummy
 *  @{
 */

/** @file
 *  Dummy network interface layer module.
 */

#ifndef __NET_NILDUMMY_H__
#define __NET_NILDUMMY_H__

#include <fibril_synch.h>
#include <ipc/services.h>

#include "../../include/device.h"
#include "../../structures/measured_strings.h"

/** Type definition of the dummy nil global data.
 *  @see nildummy_globals
 */
typedef struct nildummy_globals	nildummy_globals_t;

/** Type definition of the dummy nil device specific data.
 *  @see nildummy_device
 */
typedef struct nildummy_device	nildummy_device_t;

/** Type definition of the dummy nil device specific data pointer.
 *  @see nildummy_device
 */
typedef nildummy_device_t *		nildummy_device_ref;

/** Type definition of the dummy nil protocol specific data.
 *  @see nildummy_proto
 */
typedef struct nildummy_proto	nildummy_proto_t;

/** Type definition of the dummy nil protocol specific data pointer.
 *  @see nildummy_proto
 */
typedef nildummy_proto_t *		nildummy_proto_ref;

/** Dummy nil device map.
 *  Maps devices to the dummy nil device specific data.
 *  @see device.h
 */
DEVICE_MAP_DECLARE( nildummy_devices, nildummy_device_t )

/** Dummy nil device specific data.
 */
struct	nildummy_device{
	/** Device identifier.
	 */
	device_id_t			device_id;
	/** Device driver service.
	 */
	services_t			service;
	/** Driver phone.
	 */
	int					phone;
	/** Maximal transmission unit.
	 */
	size_t				mtu;
	/** Actual device hardware address.
	 */
	measured_string_ref	addr;
	/** Actual device hardware address data.
	 */
	char *				addr_data;
};

/** Dummy nil protocol specific data.
 */
struct nildummy_proto{
	/** Protocol service.
	 */
	services_t	service;
	/** Protocol module phone.
	 */
	int			phone;
};

/** Dummy nil global data.
 */
struct	nildummy_globals{
	/** Networking module phone.
	 */
	int				net_phone;
	/** Safety lock for devices.
	 */
	fibril_rwlock_t		devices_lock;
	/** All known Ethernet devices.
	 */
	nildummy_devices_t	devices;
	/** Safety lock for protocols.
	 */
	fibril_rwlock_t		protos_lock;
	/** Default protocol.
	 */
	nildummy_proto_t	proto;
};

#endif

/** @}
 */
