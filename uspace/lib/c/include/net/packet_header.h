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

/** @addtogroup libc 
 *  @{
 */

/** @file
 * Packet header.
 */

#ifndef LIBC_PACKET_HEADER_H_
#define LIBC_PACKET_HEADER_H_

#include <net/packet.h>

/** Returns the actual packet data length.
 * @param[in] header	The packet header.
 */
#define PACKET_DATA_LENGTH(header) \
	((header)->data_end - (header)->data_start)

/** Returns the maximum packet address length.
 * @param[in] header	The packet header.
 */
#define PACKET_MAX_ADDRESS_LENGTH(header) \
	((header)->dest_addr - (header)->src_addr)

/** Returns the minimum packet suffix.
 *  @param[in] header	The packet header.
 */
#define PACKET_MIN_SUFFIX(header) \
	((header)->length - (header)->data_start - (header)->max_content)

/** Packet integrity check magic value. */
#define PACKET_MAGIC_VALUE	0x11227788

/** Maximum total length of the packet */
#define PACKET_MAX_LENGTH  65536

/** Packet header. */
struct packet {
	/** Packet identifier. */
	packet_id_t packet_id;

	/**
	 * Packet queue sorting value.
	 * The packet queue is sorted the ascending order.
	 */
	size_t order;

	/** Packet metric. */
	size_t metric;
	/** Previous packet in the queue. */
	packet_id_t previous;
	/** Next packet in the queue. */
	packet_id_t next;

	/**
	 * Total length of the packet.
	 * Contains the header, the addresses and the data of the packet.
	 * Corresponds to the mapped sharable memory block.
	 */
	size_t length;

	/** Offload info provided by the NIC */
	uint32_t offload_info;

	/** Mask which bits in offload info are valid */
	uint32_t offload_mask;

	/** Stored source and destination addresses length. */
	size_t addr_len;

	/**
	 * Souce address offset in bytes from the beginning of the packet
	 * header.
	 */
	size_t src_addr;

	/**
	 * Destination address offset in bytes from the beginning of the packet
	 * header.
	 */
	size_t dest_addr;

	/** Reserved data prefix length in bytes. */
	size_t max_prefix;
	/** Reserved content length in bytes. */
	size_t max_content;

	/**
	 * Actual data start offset in bytes from the beginning of the packet
	 * header.
	 */
	size_t data_start;

	/**
	 * Actual data end offset in bytes from the beginning of the packet
	 * header.
	 */
	size_t data_end;

	/** Integrity check magic value. */
	int magic_value;
};

/** Returns whether the packet is valid.
 * @param[in] packet	The packet to be checked.
 * @return		True if the packet is not NULL and the magic value is
 *			correct.
 * @return		False otherwise.
 */
static inline int packet_is_valid(const packet_t *packet)
{
	return packet && (packet->magic_value == PACKET_MAGIC_VALUE);
}

#endif

/** @}
 */
