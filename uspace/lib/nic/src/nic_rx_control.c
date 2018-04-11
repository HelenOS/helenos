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
 * @brief Incoming packets (frames) filtering functions
 */

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <mem.h>
#include <nic/nic.h>
#include "nic_rx_control.h"

/**
 * Initializes the receive control structure
 *
 * @param rxc
 *
 * @return EOK		On success
 * @return ENOMEM	On not enough memory
 * @return EINVAL	Internal error, should not happen
 */
errno_t nic_rxc_init(nic_rxc_t *rxc)
{
	memset(rxc, 0, sizeof(nic_rxc_t));
	errno_t rc;
	rc = nic_addr_db_init(&rxc->blocked_sources, ETH_ADDR);
	if (rc != EOK) {
		return rc;
	}
	rc = nic_addr_db_init(&rxc->unicast_addrs, ETH_ADDR);
	if (rc != EOK) {
		return rc;
	}
	rc = nic_addr_db_init(&rxc->multicast_addrs, ETH_ADDR);
	if (rc != EOK) {
		return rc;
	}
	rxc->block_sources = false;
	rxc->unicast_mode = NIC_UNICAST_DEFAULT;
	rxc->multicast_mode = NIC_MULTICAST_BLOCKED;
	rxc->broadcast_mode = NIC_BROADCAST_ACCEPTED;

	/* Default NIC behavior */
	rxc->unicast_exact = true;
	rxc->multicast_exact = false;
	rxc->vlan_exact = true;
	return EOK;
}

/** Reinitialize the structure.
 *
 * @param filters
 */
errno_t nic_rxc_clear(nic_rxc_t *rxc)
{
	nic_addr_db_destroy(&rxc->unicast_addrs);
	nic_addr_db_destroy(&rxc->multicast_addrs);
	nic_addr_db_destroy(&rxc->blocked_sources);
	return nic_rxc_init(rxc);
}

/** Set the NIC's address that should be used as the default address during
 * the checks.
 *
 * @param rxc
 * @param prev_addr Previously used default address. Can be NULL
 *                  if this is the first call after filters' initialization.
 * @param curr_addr The new default address.
 *
 * @return EOK On success
 *
 */
errno_t nic_rxc_set_addr(nic_rxc_t *rxc, const nic_address_t *prev_addr,
    const nic_address_t *curr_addr)
{
	if (prev_addr != NULL) {
		errno_t rc = nic_addr_db_remove(&rxc->unicast_addrs,
		    (const uint8_t *) &prev_addr->address);
		if (rc != EOK)
			return rc;
	}

	return nic_addr_db_insert(&rxc->unicast_addrs,
	    (const uint8_t *) &curr_addr->address);
}

/* Helper structure */
typedef struct {
	size_t max_count;
	nic_address_t *address_list;
	size_t address_count;
} nic_rxc_add_addr_t;

/** Helper function */
static void nic_rxc_add_addr(const uint8_t *addr, void *arg)
{
	nic_rxc_add_addr_t *hs = (nic_rxc_add_addr_t *) arg;
	if (hs->address_count < hs->max_count && hs->address_list != NULL) {
		memcpy(&hs->address_list[hs->address_count].address, addr, ETH_ADDR);
	}
	hs->address_count++;
}

/**
 * Queries the current mode of unicast frames receiving.
 *
 * @param rxc
 * @param mode			The new unicast mode
 * @param max_count		Max number of addresses that can be written into the
 * 						address_list.
 * @param address_list	List of MAC addresses or NULL.
 * @param address_count Number of accepted addresses (can be > max_count)
 */
void nic_rxc_unicast_get_mode(const nic_rxc_t *rxc, nic_unicast_mode_t *mode,
    size_t max_count, nic_address_t *address_list, size_t *address_count)
{
	*mode = rxc->unicast_mode;
	if (rxc->unicast_mode == NIC_UNICAST_LIST) {
		nic_rxc_add_addr_t hs = {
			.max_count = max_count,
			.address_list = address_list,
			.address_count = 0
		};
		nic_addr_db_foreach(&rxc->unicast_addrs, nic_rxc_add_addr, &hs);
		if (address_count) {
			*address_count = hs.address_count;
		}
	}
}

/**
 * Sets the current mode of unicast frames receiving.
 *
 * @param rxc
 * @param mode			The current unicast mode
 * @param address_list	List of MAC addresses or NULL.
 * @param address_count Number of addresses in the list
 *
 * @return EOK		On success
 * @return EINVAL	If any of the MAC addresses is not a unicast address.
 * @return ENOMEM	If there was not enough memory
 */
errno_t nic_rxc_unicast_set_mode(nic_rxc_t *rxc, nic_unicast_mode_t mode,
    const nic_address_t *address_list, size_t address_count)
{
	if (mode == NIC_UNICAST_LIST && address_list == NULL) {
		return EINVAL;
	} else if (mode != NIC_UNICAST_LIST && address_list != NULL) {
		return EINVAL;
	}

	if (rxc->unicast_mode == NIC_UNICAST_LIST) {
		nic_addr_db_clear(&rxc->unicast_addrs);
	}
	rxc->unicast_mode = mode;
	size_t i;
	for (i = 0; i < address_count; ++i) {
		errno_t rc = nic_addr_db_insert(&rxc->unicast_addrs,
		    (const uint8_t *) &address_list[i].address);
		if (rc == ENOMEM) {
			return ENOMEM;
		}
	}
	return EOK;
}

/**
 * Queries the current mode of multicast frames receiving.
 *
 * @param rxc
 * @param mode			The current multicast mode
 * @param max_count		Max number of addresses that can be written into the
 * 						address_list.
 * @param address_list	List of MAC addresses or NULL.
 * @param address_count Number of accepted addresses (can be > max_count)
 */
void nic_rxc_multicast_get_mode(const nic_rxc_t *rxc,
    nic_multicast_mode_t *mode, size_t max_count, nic_address_t *address_list,
    size_t *address_count)
{
	*mode = rxc->multicast_mode;
	if (rxc->multicast_mode == NIC_MULTICAST_LIST) {
		nic_rxc_add_addr_t hs = {
			.max_count = max_count,
			.address_list = address_list,
			.address_count = 0
		};
		nic_addr_db_foreach(&rxc->multicast_addrs, nic_rxc_add_addr, &hs);
		if (address_count) {
			*address_count = hs.address_count;
		}
	}
}

/**
 * Sets the current mode of multicast frames receiving.
 *
 * @param rxc
 * @param mode			The new multicast mode
 * @param address_list	List of MAC addresses or NULL.
 * @param address_count Number of addresses in the list
 *
 * @return EOK		On success
 * @return EINVAL	If any of the MAC addresses is not a multicast address.
 * @return ENOMEM	If there was not enough memory
 */
errno_t nic_rxc_multicast_set_mode(nic_rxc_t *rxc, nic_multicast_mode_t mode,
    const nic_address_t *address_list, size_t address_count)
{
	if (mode == NIC_MULTICAST_LIST && address_list == NULL)
		return EINVAL;
	else if (mode != NIC_MULTICAST_LIST && address_list != NULL)
		return EINVAL;

	if (rxc->multicast_mode == NIC_MULTICAST_LIST)
		nic_addr_db_clear(&rxc->multicast_addrs);

	rxc->multicast_mode = mode;
	size_t i;
	for (i = 0; i < address_count; ++i) {
		errno_t rc = nic_addr_db_insert(&rxc->multicast_addrs,
		    (const uint8_t *)&address_list[i].address);
		if (rc == ENOMEM) {
			return ENOMEM;
		}
	}
	return EOK;
}

/**
 * Queries the current mode of broadcast frames receiving.
 *
 * @param rxc
 * @param mode			The new broadcast mode
 */
void nic_rxc_broadcast_get_mode(const nic_rxc_t *rxc, nic_broadcast_mode_t *mode)
{
	*mode = rxc->broadcast_mode;
}

/**
 * Sets the current mode of broadcast frames receiving.
 *
 * @param rxc
 * @param mode			The new broadcast mode
 *
 * @return EOK		On success
 */
errno_t nic_rxc_broadcast_set_mode(nic_rxc_t *rxc, nic_broadcast_mode_t mode)
{
	rxc->broadcast_mode = mode;
	return EOK;
}

/**
 * Queries the current blocked source addresses.
 *
 * @param rxc
 * @param max_count		Max number of addresses that can be written into the
 * 						address_list.
 * @param address_list	List of MAC addresses or NULL.
 * @param address_count Number of blocked addresses (can be > max_count)
 */
void nic_rxc_blocked_sources_get(const nic_rxc_t *rxc,
    size_t max_count, nic_address_t *address_list, size_t *address_count)
{
	nic_rxc_add_addr_t hs = {
		.max_count = max_count,
		.address_list = address_list,
		.address_count = 0
	};
	nic_addr_db_foreach(&rxc->blocked_sources, nic_rxc_add_addr, &hs);
	if (address_count) {
		*address_count = hs.address_count;
	}
}

/**
 * Clears the currently blocked addresses and sets the addresses contained in
 * the list as the set of blocked source addresses (no frame with this source
 * address will be received). Duplicated addresses are ignored.
 *
 * @param rxc
 * @param address_list	List of the blocked addresses. Can be NULL.
 * @param address_count Number of addresses in the list
 *
 * @return EOK		On success
 * @return ENOMEM	If there was not enough memory
 */
errno_t nic_rxc_blocked_sources_set(nic_rxc_t *rxc,
    const nic_address_t *address_list, size_t address_count)
{
	assert((address_count == 0 && address_list == NULL) ||
	    (address_count != 0 && address_list != NULL));

	nic_addr_db_clear(&rxc->blocked_sources);
	rxc->block_sources = (address_count != 0);
	size_t i;
	for (i = 0; i < address_count; ++i) {
		errno_t rc = nic_addr_db_insert(&rxc->blocked_sources,
		    (const uint8_t *) &address_list[i].address);
		if (rc == ENOMEM) {
			return ENOMEM;
		}
	}
	return EOK;
}

/**
 * Query mask used for filtering according to the VLAN tags.
 *
 * @param rxc
 * @param mask		Must be 512 bytes long
 *
 * @return EOK
 * @return ENOENT
 */
errno_t nic_rxc_vlan_get_mask(const nic_rxc_t *rxc, nic_vlan_mask_t *mask)
{
	if (rxc->vlan_mask == NULL) {
		return ENOENT;
	}
	memcpy(mask, rxc->vlan_mask, sizeof (nic_vlan_mask_t));
	return EOK;
}

/**
 * Set mask for filtering according to the VLAN tags.
 *
 * @param rxc
 * @param mask		Must be 512 bytes long
 *
 * @return EOK
 * @return ENOMEM
 */
errno_t nic_rxc_vlan_set_mask(nic_rxc_t *rxc, const nic_vlan_mask_t *mask)
{
	if (mask == NULL) {
		if (rxc->vlan_mask) {
			free(rxc->vlan_mask);
		}
		rxc->vlan_mask = NULL;
		return EOK;
	}
	if (!rxc->vlan_mask) {
		rxc->vlan_mask = malloc(sizeof (nic_vlan_mask_t));
		if (rxc->vlan_mask == NULL) {
			return ENOMEM;
		}
	}
	memcpy(rxc->vlan_mask, mask, sizeof (nic_vlan_mask_t));
	return EOK;
}


/**
 * Check if the frame passes through the receive control.
 *
 * @param rxc
 * @param frame	    The probed frame
 *
 * @return True if the frame passes, false if it does not
 */
bool nic_rxc_check(const nic_rxc_t *rxc, const void *data, size_t size,
    nic_frame_type_t *frame_type)
{
	assert(frame_type != NULL);
	uint8_t *dest_addr = (uint8_t *) data;
	uint8_t *src_addr = dest_addr + ETH_ADDR;

	if (size < 2 * ETH_ADDR)
		return false;

	if (dest_addr[0] & 1) {
		/* Multicast or broadcast */
		if (*(uint32_t *) dest_addr == 0xFFFFFFFF &&
		    *(uint16_t *) (dest_addr + 4) == 0xFFFF) {
			/* It is broadcast */
			*frame_type = NIC_FRAME_BROADCAST;
			if (rxc->broadcast_mode == NIC_BROADCAST_BLOCKED)
				return false;
		} else {
			*frame_type = NIC_FRAME_MULTICAST;
			/* In promiscuous mode the multicast_exact should be set to true */
			if (!rxc->multicast_exact) {
				if (rxc->multicast_mode == NIC_MULTICAST_BLOCKED)
					return false;
				else {
					if (!nic_addr_db_contains(&rxc->multicast_addrs,
					    dest_addr))
						return false;
				}
			}
		}
	} else {
		/* Unicast */
		*frame_type = NIC_FRAME_UNICAST;
		/* In promiscuous mode the unicast_exact should be set to true */
		if (!rxc->unicast_exact) {
			if (rxc->unicast_mode == NIC_UNICAST_BLOCKED)
				return false;
			else {
				if (!nic_addr_db_contains(&rxc->unicast_addrs,
				    dest_addr))
					return false;
			}
		}
	}

	/* Blocked source addresses */
	if (rxc->block_sources) {
		if (nic_addr_db_contains(&rxc->blocked_sources, src_addr))
			return false;
	}

	/* VLAN filtering */
	if (!rxc->vlan_exact && rxc->vlan_mask != NULL) {
		vlan_header_t *vlan_header = (vlan_header_t *)
		    ((uint8_t *) data + 2 * ETH_ADDR);
		if (vlan_header->tpid_upper == VLAN_TPID_UPPER &&
		    vlan_header->tpid_lower == VLAN_TPID_LOWER) {
			int index = ((int) (vlan_header->vid_upper & 0xF) << 5) |
			    (vlan_header->vid_lower >> 3);
			if (!(rxc->vlan_mask->bitmap[index] &
			    (1 << (vlan_header->vid_lower & 0x7))))
				return false;
		}
	}

	return true;
}

/**
 * Set information about current HW filtering.
 *  1 ...	Only those frames we want to receive are passed through HW
 *  0 ...	The HW filtering is imperfect
 * -1 ...	Don't change the setting
 * This function should be called only from the mode change event handler.
 *
 * @param	rxc
 * @param	unicast_exact	Unicast frames
 * @param	mcast_exact		Multicast frames
 * @param	vlan_exact		VLAN tags
 */
void nic_rxc_hw_filtering(nic_rxc_t *rxc,
    int unicast_exact, int multicast_exact, int vlan_exact)
{
	if (unicast_exact >= 0)
		rxc->unicast_exact = unicast_exact;
	if (multicast_exact >= 0)
		rxc->multicast_exact = multicast_exact;
	if (vlan_exact >= 0)
		rxc->vlan_exact = vlan_exact;
}

/** Polynomial used in multicast address hashing */
#define CRC_MCAST_POLYNOMIAL  0x04c11db6

/** Compute the standard hash from MAC
 *
 * Hashing MAC into 64 possible values and using the value as index to
 * 64bit number.
 *
 * The code is copied from qemu-0.13's implementation of ne2000 and rt8139
 * drivers, but according to documentation there it originates in FreeBSD.
 *
 * @param[in] addr The 6-byte MAC address to be hashed
 *
 * @return 64-bit number with only single bit set to 1
 *
 */
static uint64_t multicast_hash(const uint8_t addr[6])
{
	uint32_t crc;
	int carry, i, j;
	uint8_t b;

	crc = 0xffffffff;
	for (i = 0; i < 6; i++) {
		b = addr[i];
		for (j = 0; j < 8; j++) {
			carry = ((crc & 0x80000000L) ? 1 : 0) ^ (b & 0x01);
			crc <<= 1;
			b >>= 1;
			if (carry)
				crc = ((crc ^ CRC_MCAST_POLYNOMIAL) | carry);
		}
	}

	uint64_t one64 = 1;
	return one64 << (crc >> 26);
}


/**
 * Computes hash for the address list based on standard multicast address
 * hashing.
 *
 * @param address_list
 * @param count
 *
 * @return Multicast hash
 *
 * @see multicast_hash
 */
uint64_t nic_rxc_mcast_hash(const nic_address_t *address_list, size_t count)
{
	size_t i;
	uint64_t hash = 0;
	for (i = 0; i < count; ++i) {
		hash |= multicast_hash(address_list[i].address);
	}
	return hash;
}

static void nic_rxc_hash_addr(const uint8_t *address, void *arg)
{
	*((uint64_t *) arg) |= multicast_hash(address);
}

/**
 * Computes hash for multicast addresses currently set up in the RX multicast
 * filtering. For promiscuous mode returns all ones, for blocking all zeroes.
 * Can be called only from the on_*_change handler.
 *
 * @param rxc
 *
 * @return Multicast hash
 *
 * @see multicast_hash
 */
uint64_t nic_rxc_multicast_get_hash(const nic_rxc_t *rxc)
{
	switch (rxc->multicast_mode) {
	case NIC_MULTICAST_UNKNOWN:
	case NIC_MULTICAST_BLOCKED:
		return 0;
	case NIC_MULTICAST_LIST:
		break;
	case NIC_MULTICAST_PROMISC:
		return ~(uint64_t) 0;
	}
	uint64_t hash;
	nic_addr_db_foreach(&rxc->multicast_addrs, nic_rxc_hash_addr, &hash);
	return hash;
}

/** @}
 */
