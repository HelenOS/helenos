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

/** @addtogroup eth
 *  @{
 */

/** @file
 * Ethernet module.
 */

#ifndef NET_ETH_H_
#define NET_ETH_H_

#include <async.h>
#include <fibril_synch.h>
#include <ipc/loc.h>
#include <ipc/services.h>
#include <net/device.h>
#include <adt/measured_strings.h>

/** Ethernet address length. */
#define ETH_ADDR  6

/** Ethernet header preamble value. */
#define ETH_PREAMBLE  0x55

/** Ethernet header start of frame value. */
#define ETH_SFD  0xD5

/** IEEE 802.2 unordered information control field. */
#define IEEE_8023_2_UI  0x03

/** Type definition of the Ethernet header IEEE 802.3 + 802.2 + SNAP extensions.
 * @see eth_header_snap
 */
typedef struct eth_header_snap eth_header_snap_t;

/** Type definition of the Ethernet header IEEE 802.3 + 802.2 + SNAP extensions.
 * @see eth_header_lsap
 */
typedef struct eth_header_lsap eth_header_lsap_t;

/** Type definition of the Ethernet header LSAP extension.
 * @see eth_ieee_lsap
 */
typedef struct eth_ieee_lsap eth_ieee_lsap_t;

/** Type definition of the Ethernet header SNAP extension.
 * @see eth_snap
 */
typedef struct eth_snap eth_snap_t;

/** Type definition of the Ethernet header preamble.
 * @see preamble
 */
typedef struct eth_preamble eth_preamble_t;

/** Type definition of the Ethernet header.
 * @see eth_header
 */
typedef struct eth_header eth_header_t;

/** Ethernet header Link Service Access Point extension. */
struct eth_ieee_lsap {
	/**
	 * Destination Service Access Point identifier.
	 * The possible values are assigned by an IEEE committee.
	 */
	uint8_t dsap;
	
	/**
	 * Source Service Access Point identifier.
	 * The possible values are assigned by an IEEE committee.
	 */
	uint8_t ssap;
	
	/**
	 * Control parameter.
	 * The possible values are assigned by an IEEE committee.
	 */
	uint8_t ctrl;
} __attribute__ ((packed));

/** Ethernet header SNAP extension. */
struct eth_snap {
	/** Protocol identifier or organization code. */
	uint8_t protocol[3];
	
	/**
	 * Ethernet protocol identifier in the network byte order (big endian).
	 * @see ethernet_protocols.h
	 */
	uint16_t ethertype;
} __attribute__ ((packed));

/** Ethernet header preamble.
 *
 * Used for dummy devices.
 */
struct eth_preamble {
	/**
	 * Controlling preamble used for the frame transmission synchronization.
	 * All should be set to ETH_PREAMBLE.
	 */
	uint8_t preamble[7];
	
	/**
	 * Start of Frame Delimiter used for the frame transmission
	 * synchronization.
	 * Should be set to ETH_SFD.
	 */
	uint8_t sfd;
} __attribute__ ((packed));

/** Ethernet header. */
struct eth_header {
	/** Destination host Ethernet address (MAC address). */
	uint8_t destination_address[ETH_ADDR];
	/** Source host Ethernet address (MAC address). */
	uint8_t source_address[ETH_ADDR];
	
	/**
	 * Ethernet protocol identifier in the network byte order (big endian).
	 * @see ethernet_protocols.h
	 */
	uint16_t ethertype;
} __attribute__ ((packed));

/** Ethernet header IEEE 802.3 + 802.2 extension. */
struct eth_header_lsap {
	/** Ethernet header. */
	eth_header_t header;
	
	/**
	 * LSAP extension.
	 * If DSAP and SSAP are set to ETH_LSAP_SNAP the SNAP extension is being
	 * used.
	 * If DSAP and SSAP fields are equal to ETH_RAW the raw Ethernet packet
	 * without any extensions is being used and the frame content starts
	 * rigth after the two fields.
	 */
	eth_ieee_lsap_t lsap;
} __attribute__ ((packed));

/** Ethernet header IEEE 802.3 + 802.2 + SNAP extensions. */
struct eth_header_snap {
	/** Ethernet header. */
	eth_header_t header;
	
	/**
	 * LSAP extension.
	 * If DSAP and SSAP are set to ETH_LSAP_SNAP the SNAP extension is being
	 * used.
	 * If DSAP and SSAP fields are equal to ETH_RAW the raw Ethernet packet
	 * without any extensions is being used and the frame content starts
	 * rigth after the two fields.
	 */
	eth_ieee_lsap_t lsap;
	
	/** SNAP extension. */
	eth_snap_t snap;
} __attribute__ ((packed));

/** Ethernet Frame Check Sequence. */
typedef uint32_t eth_fcs_t;

/** Type definition of the Ethernet global data.
 * @see eth_globals
 */
typedef struct eth_globals eth_globals_t;

/** Type definition of the Ethernet device specific data.
 * @see eth_device
 */
typedef struct eth_device eth_device_t;

/** Type definition of the Ethernet protocol specific data.
 * @see eth_proto
 */
typedef struct eth_proto eth_proto_t;

/** Ethernet device map.
 * Maps devices to the Ethernet device specific data.
 * @see device.h
 */
DEVICE_MAP_DECLARE(eth_devices, eth_device_t);

/** Ethernet protocol map.
 * Maps protocol identifiers to the Ethernet protocol specific data.
 * @see int_map.h
 */
INT_MAP_DECLARE(eth_protos, eth_proto_t);

/** Ethernet device specific data. */
struct eth_device {
	/** Device identifier. */
	nic_device_id_t device_id;
	/** Device handle */
	service_id_t sid;
	/** Driver session. */
	async_sess_t *sess;
	/** Maximal transmission unit. */
	size_t mtu;
	
	/**
	 * Various device flags.
	 * @see ETH_DUMMY
	 * @see ETH_MODE_MASK
	 */
	int flags;
	
	/** Actual device hardware address. */
	nic_address_t addr;
};

/** Ethernet protocol specific data. */
struct eth_proto {
	/** Protocol service. */
	services_t service;
	/** Protocol identifier. */
	int protocol;
	/** Protocol module session. */
	async_sess_t *sess;
};

/** Ethernet global data. */
struct eth_globals {
	/** Networking module session. */
	async_sess_t *net_sess;
	/** Safety lock for devices. */
	fibril_rwlock_t devices_lock;
	/** All known Ethernet devices. */
	eth_devices_t devices;
	/** Safety lock for protocols. */
	fibril_rwlock_t protos_lock;
	
	/**
	 * Protocol map.
	 * Service map for each protocol.
	 */
	eth_protos_t protos;
	
	/** Broadcast device hardware address. */
	uint8_t broadcast_addr[ETH_ADDR];
};

#endif

/** @}
 */
