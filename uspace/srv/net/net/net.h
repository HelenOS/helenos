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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Networking subsystem central module.
 */

#ifndef __NET_NET_H__
#define __NET_NET_H__

#include <ipc/ipc.h>

#include "../include/device.h"

#include "../structures/char_map.h"
#include "../structures/generic_char_map.h"
#include "../structures/measured_strings.h"
#include "../structures/module_map.h"
#include "../structures/packet/packet.h"

/** @name Modules definitions
 */
/*@{*/

/** Loopback network interface module name.
 */
#define LO_NAME				"lo"

/** Loopback network interface module full path filename.
 */
#define LO_FILENAME			"/srv/lo"

/** DP8390 network interface module name.
 */
#define DP8390_NAME			"dp8390"

/** DP8390 network interface module full path filename.
 */
#define DP8390_FILENAME		"/srv/dp8390"

/** Ethernet module name.
 */
#define ETHERNET_NAME		"ethernet"

/** Ethernet module full path filename.
 */
#define ETHERNET_FILENAME	"/srv/eth"

/** Ethernet module name.
 */
#define NILDUMMY_NAME		"nildummy"

/** Ethernet module full path filename.
 */
#define NILDUMMY_FILENAME	"/srv/nildummy"

/** IP module name.
 */
#define IP_NAME				"ip"

/** IP module full path filename.
 */
#define IP_FILENAME			"/srv/ip"

/*@}*/

/** @name Configuration setting names definitions
 */
/*@{*/

/** Network interface name configuration label.
 */
#define CONF_NAME			"NAME"

/** Network interface module name configuration label.
 */
#define CONF_NETIF			"NETIF"

/** Network interface layer module name configuration label.
 */
#define CONF_NIL			"NIL"

/** Internet protocol module name configuration label.
 */
#define CONF_IL				"IL"

/** Interrupt number configuration label.
 */
#define CONF_IRQ			"IRQ"

/** Device input/output address configuration label.
 */
#define CONF_IO				"IO"

/** Maximum transmission unit configuration label.
 */
#define CONF_MTU			"MTU"

/*@}*/

/** Configuration directory.
 */
#define CONF_DIR			"/cfg/net"

/** General configuration file.
 */
#define CONF_GENERAL_FILE	"general"

/** Type definition of the network interface specific data.
 *  @see netif
 */
typedef struct netif	netif_t;

/** Type definition of the network interface specific data pointer.
 *  @see netif
 */
typedef netif_t *		netif_ref;

/** Type definition of the networking module global data.
 *  @see net_globals
 */
typedef struct net_globals	net_globals_t;

/** Present network interfaces.
 *  Maps devices to the networking device specific data.
 *  @see device.h
 */
DEVICE_MAP_DECLARE(netifs, netif_t)

/** Configuration settings.
 *  Maps setting names to the values.
 *  @see generic_char_map.h
 */
GENERIC_CHAR_MAP_DECLARE(measured_strings, measured_string_t)

/** Present network interface device.
 */
struct netif{
	/** System-unique network interface identifier.
	 */
	device_id_t id;
	/** Serving network interface driver module index.
	 */
	module_ref driver;
	/** Serving link layer module index.
	 */
	module_ref nil;
	/** Serving internet layer module index.
	 */
	module_ref il;
	/** System-unique network interface name.
	 */
	char * name;
	/** Configuration.
	 */
	measured_strings_t configuration;
};

/** Networking module global variables.
 */
struct net_globals{
	/** Present network interfaces.
	 */
	netifs_t netifs;
	/** Network interface structure indices by names.
	 */
	char_map_t netif_names;
	/** Available modules.
	 */
	modules_t modules;
	/** Global configuration.
	 */
	measured_strings_t configuration;
};

/** Adds the configured setting to the configuration map.
 *  @param[in] configuration The configuration map.
 *  @param[in] name The setting name.
 *  @param[in] value The setting value.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left.
 */
int add_configuration(measured_strings_ref configuration, const char * name, const char * value);

/** Processes the networking message.
 *  @param[in] callid The message identifier.
 *  @param[in] call The message parameters.
 *  @param[out] answer The message answer parameters.
 *  @param[out] answer_count The last parameter for the actual answer in the answer parameter.
 *  @returns EOK on success.
 *  @returns ENOTSUP if the message is not known.
 *  @see net_interface.h
 *  @see IS_NET_NET_MESSAGE()
 */
int net_message(ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count);

/** Initializes the networking module for the chosen subsystem build type.
 *  @param[in] client_connection The client connection processing function. The module skeleton propagates its own one.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left.
 */
int net_initialize_build(async_client_conn_t client_connection);

/** Processes the module message.
 *  Distributes the message to the right bundled module.
 *  @param[in] callid The message identifier.
 *  @param[in] call The message parameters.
 *  @param[out] answer The message answer parameters.
 *  @param[out] answer_count The last parameter for the actual answer in the answer parameter.
 *  @returns EOK on success.
 *  @returns ENOTSUP if the message is not known.
 *  @returns Other error codes as defined for each bundled module message function.
 */
int module_message(ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count);

#endif

/** @}
 */
