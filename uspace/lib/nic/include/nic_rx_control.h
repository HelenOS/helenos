/*
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

/**
 * @addtogroup libnic
 * @{
 */
/**
 * @file
 * @brief Incoming packets (frames) filtering control structures
 */

#ifndef __NIC_FILTERS_H__
#define __NIC_FILTERS_H__

#ifndef LIBNIC_INTERNAL
#error "This is internal libnic's header, please do not include it"
#endif

#include <adt/hash_table.h>
#include <fibril_synch.h>
#include <nic/nic.h>

#include "nic_addr_db.h"

/**
 * General structure describing receive control.
 * The structure is not synchronized inside, the nic_driver should provide
 * a synchronized facade.
 */
typedef struct nic_rxc {
	/**
	 * Allowed unicast destination MAC addresses
	 */
	nic_addr_db_t unicast_addrs;
	/**
	 * Allowed unicast destination MAC addresses
	 */
	nic_addr_db_t multicast_addrs;
	/**
	 * Single flag if any source is blocked
	 */
	int block_sources;
	/**
	 * Blocked source MAC addresses
	 */
	nic_addr_db_t blocked_sources;
	/**
	 * Selected mode for unicast frames
	 */
	nic_unicast_mode_t unicast_mode;
	/**
	 * Selected mode for multicast frames
	 */
	nic_multicast_mode_t multicast_mode;
	/**
	 * Selected mode for broadcast frames
	 */
	nic_broadcast_mode_t broadcast_mode;
	/**
	 * Mask for VLAN tags. This vector must be at least 512 bytes long.
	 */
	nic_vlan_mask_t *vlan_mask;
	/**
	 * If true, the NIC is receiving only unicast frames which we really want to
	 * receive (the filtering is perfect).
	 */
	int unicast_exact;
	/**
	 * If true, the NIC is receiving only multicast frames which we really want
	 * to receive (the filtering is perfect).
	 */
	int multicast_exact;
	/**
	 * If true, the NIC is receiving only frames with VLAN tags which we really
	 * want to receive (the filtering is perfect).
	 */
	int vlan_exact;
} nic_rxc_t;

#define VLAN_TPID_UPPER 0x81
#define VLAN_TPID_LOWER 0x00

typedef struct vlan_header {
	uint8_t tpid_upper;
	uint8_t tpid_lower;
	uint8_t vid_upper;
	uint8_t vid_lower;
} __attribute__((packed)) vlan_header_t;

extern errno_t nic_rxc_init(nic_rxc_t *rxc);
extern errno_t nic_rxc_clear(nic_rxc_t *rxc);
extern errno_t nic_rxc_set_addr(nic_rxc_t *rxc,
    const nic_address_t *prev_addr, const nic_address_t *curr_addr);
extern bool nic_rxc_check(const nic_rxc_t *rxc,
    const void *data, size_t size, nic_frame_type_t *frame_type);
extern void nic_rxc_hw_filtering(nic_rxc_t *rxc,
    int unicast_exact, int multicast_exact, int vlan_exact);
extern uint64_t nic_rxc_mcast_hash(const nic_address_t *list, size_t count);
extern uint64_t nic_rxc_multicast_get_hash(const nic_rxc_t *rxc);
extern void nic_rxc_unicast_get_mode(const nic_rxc_t *, nic_unicast_mode_t *,
    size_t max_count, nic_address_t *address_list, size_t *address_count);
extern errno_t nic_rxc_unicast_set_mode(nic_rxc_t *rxc, nic_unicast_mode_t mode,
    const nic_address_t *address_list, size_t address_count);
extern void nic_rxc_multicast_get_mode(const nic_rxc_t *,
    nic_multicast_mode_t *, size_t, nic_address_t *, size_t *);
extern errno_t nic_rxc_multicast_set_mode(nic_rxc_t *, nic_multicast_mode_t mode,
    const nic_address_t *address_list, size_t address_count);
extern void nic_rxc_broadcast_get_mode(const nic_rxc_t *,
    nic_broadcast_mode_t *mode);
extern errno_t nic_rxc_broadcast_set_mode(nic_rxc_t *,
    nic_broadcast_mode_t mode);
extern void nic_rxc_blocked_sources_get(const nic_rxc_t *,
    size_t max_count, nic_address_t *address_list, size_t *address_count);
extern errno_t nic_rxc_blocked_sources_set(nic_rxc_t *,
    const nic_address_t *address_list, size_t address_count);
extern errno_t nic_rxc_vlan_get_mask(const nic_rxc_t *rxc, nic_vlan_mask_t *mask);
extern errno_t nic_rxc_vlan_set_mask(nic_rxc_t *rxc, const nic_vlan_mask_t *mask);

#endif

/** @}
 */
