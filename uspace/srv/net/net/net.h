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

/** @addtogroup net
 * @{
 */

#ifndef NET_NET_H_
#define NET_NET_H_

#include <ipc/loc.h>
#include <net/device.h>
#include <adt/char_map.h>
#include <adt/generic_char_map.h>
#include <adt/measured_strings.h>
#include <adt/module_map.h>
#include <net/packet.h>

#define NAME  "net"

#define ETHERNET_FILENAME  "/srv/eth"
#define ETHERNET_NAME      "eth"

#define IP_FILENAME  "/srv/ip"
#define IP_NAME      "ip"

#define NILDUMMY_FILENAME  "/srv/nildummy"
#define NILDUMMY_NAME      "nildummy"

/** @}
 */

/** @name Configuration setting names definitions
 * @{
 */

#define CONF_IL     "IL"     /**< Internet protocol module name configuration label. */
#define CONF_IO     "IO"     /**< Device input/output address configuration label. */
#define CONF_IRQ    "IRQ"    /**< Interrupt number configuration label. */
#define CONF_MTU    "MTU"    /**< Maximum transmission unit configuration label. */
#define CONF_NAME   "NAME"   /**< Network interface name configuration label. */
#define CONF_HWPATH "HWPATH" /**< Network interface hardware pathname label. */
#define CONF_NIL    "NIL"    /**< Network interface layer module name configuration label. */

/** @}
 */

#define CONF_DIR           "/cfg/net"  /**< Configuration directory. */
#define CONF_GENERAL_FILE  "general"   /**< General configuration file. */
#define CONF_EXT           ".nic"      /**< Extension for NIC's configuration files. */

/** Configuration settings.
 *
 * Maps setting names to the values.
 * @see generic_char_map.h
 *
 */
GENERIC_CHAR_MAP_DECLARE(measured_strings, measured_string_t);

/** Present network interface device.
 *
 */
typedef struct {
	/** System-unique network interface name. */
	uint8_t *name;
	/** System-unique network interface identifier. */
	nic_device_id_t id;
	/** Configuration. */
	measured_strings_t configuration;
	
	/** Serving network interface driver module index. */
	service_id_t sid;    /**< Service ID */
	async_sess_t *sess;  /**< Driver session. */
	
	module_t *nil;  /**< Serving link layer module index. */
	module_t *il;   /**< Serving internet layer module index. */
	
	link_t netif_list;
} netif_t;

/** Present network interfaces.
 *
 * Maps devices to the networking device specific data.
 * @see device.h
 *
 */
DEVICE_MAP_DECLARE(netifs, netif_t);

/** Networking module global data.
 *
 */
typedef struct {
	measured_strings_t configuration;  /**< Global configuration. */
	modules_t modules;                 /**< Available modules. */
	
	/** Network interface structure indices by hardware path. */
	char_map_t netif_hwpaths;
	
	/** Present network interfaces. */
	netifs_t netifs;
} net_globals_t;

#endif

/** @}
 */
