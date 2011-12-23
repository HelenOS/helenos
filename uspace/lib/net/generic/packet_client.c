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
 * Packet client implementation.
 */

#include <errno.h>
#include <mem.h>
#include <unistd.h>
#include <sys/mman.h>
#include <packet_client.h>
#include <packet_remote.h>

#include <net/packet.h>
#include <net/packet_header.h>

/** Copies the specified data to the beginning of the actual packet content.
 *
 * Pushes the content end if needed.
 *
 * @param[in] packet	The packet to be filled.
 * @param[in] data	The data to be copied.
 * @param[in] length	The length of the copied data.
 * @return		EOK on success.
 * @return		EINVAL if the packet is not valid.
 * @return		ENOMEM if there is not enough memory left.
 */
int packet_copy_data(packet_t *packet, const void *data, size_t length)
{
	if (!packet_is_valid(packet))
		return EINVAL;

	if (packet->data_start + length >= packet->length)
		return ENOMEM;

	memcpy((void *) packet + packet->data_start, data, length);
	if (packet->data_start + length > packet->data_end) 
		packet->data_end = packet->data_start + length;

	return EOK;
}

/** Allocates the specified space right before the actual packet content and
 * returns its pointer.
 *
 * @param[in] packet	The packet to be used.
 * @param[in] length	The space length to be allocated at the beginning of the
 *			packet content.
 * @return		The pointer to the allocated memory.
 * @return		NULL if there is not enough memory left.
 */
void *packet_prefix(packet_t *packet, size_t length)
{
	if ((!packet_is_valid(packet)) ||
	    (packet->data_start - sizeof(packet_t) -
	    2 * (packet->dest_addr - packet->src_addr) < length)) {
		return NULL;
	}

	packet->data_start -= length;
	return (void *) packet + packet->data_start;
}

/** Allocates the specified space right after the actual packet content and
 * returns its pointer.
 *
 * @param[in] packet	The packet to be used.
 * @param[in] length	The space length to be allocated at the end of the
 *			 packet content.
 * @return		The pointer to the allocated memory.
 * @return		NULL if there is not enough memory left.
 */
void *packet_suffix(packet_t *packet, size_t length)
{
	if ((!packet_is_valid(packet)) ||
	    (packet->data_end + length >= packet->length)) {
		return NULL;
	}

	packet->data_end += length;
	return (void *) packet + packet->data_end - length;
}

/** Trims the actual packet content by the specified prefix and suffix lengths.
 *
 * @param[in] packet	The packet to be trimmed.
 * @param[in] prefix	The prefix length to be removed from the beginning of
 *			the packet content.
 * @param[in] suffix	The suffix length to be removed from the end of the
 *			packet content.
 * @return		EOK on success.
 * @return		EINVAL if the packet is not valid.
 * @return		ENOMEM if there is not enough memory left.
 */
int packet_trim(packet_t *packet, size_t prefix, size_t suffix)
{
	if (!packet_is_valid(packet))
		return EINVAL;

	if (prefix + suffix > PACKET_DATA_LENGTH(packet))
		return ENOMEM;

	packet->data_start += prefix;
	packet->data_end -= suffix;
	return EOK;
}

/** Returns the packet identifier.
 *
 * @param[in] packet	The packet.
 * @return		The packet identifier.
 * @return		Zero if the packet is not valid.
 */
packet_id_t packet_get_id(const packet_t *packet)
{
	return packet_is_valid(packet) ? packet->packet_id : 0;
}

/** Returns the stored packet addresses and their length.
 *
 * @param[in] packet	The packet.
 * @param[out] src	The source address. May be NULL if not desired.
 * @param[out] dest	The destination address. May be NULL if not desired.
 * @return		The stored addresses length.
 * @return		Zero if the addresses are not present.
 * @return		EINVAL if the packet is not valid.
 */
int packet_get_addr(const packet_t *packet, uint8_t **src, uint8_t **dest)
{
	if (!packet_is_valid(packet))
		return EINVAL;
	if (!packet->addr_len)
		return 0;
	if (src)
		*src = (void *) packet + packet->src_addr;
	if (dest)
		*dest = (void *) packet + packet->dest_addr;

	return packet->addr_len;
}

/** Returns the packet content length.
 *
 * @param[in] packet	The packet.
 * @return		The packet content length in bytes.
 * @return		Zero if the packet is not valid.
 */
size_t packet_get_data_length(const packet_t *packet)
{
	if (!packet_is_valid(packet))
		return 0;

	return PACKET_DATA_LENGTH(packet);
}

/** Returns the pointer to the beginning of the packet content.
 *
 * @param[in] packet	The packet.
 * @return		The pointer to the beginning of the packet content.
 * @return		NULL if the packet is not valid.
 */
void *packet_get_data(const packet_t *packet)
{
	if (!packet_is_valid(packet))
		return NULL;

	return (void *) packet + packet->data_start;
}

/** Sets the packet addresses.
 *
 * @param[in] packet	The packet.
 * @param[in] src	The new source address. May be NULL.
 * @param[in] dest	The new destination address. May be NULL.
 * @param[in] addr_len	The addresses length.
 * @return		EOK on success.
 * @return		EINVAL if the packet is not valid.
 * @return		ENOMEM if there is not enough memory left.
 */
int
packet_set_addr(packet_t *packet, const uint8_t *src, const uint8_t *dest,
    size_t addr_len)
{
	size_t padding;
	size_t allocated;

	if (!packet_is_valid(packet))
		return EINVAL;

	allocated = PACKET_MAX_ADDRESS_LENGTH(packet);
	if (allocated < addr_len)
		return ENOMEM;

	padding = allocated - addr_len;
	packet->addr_len = addr_len;

	if (src) {
		memcpy((void *) packet + packet->src_addr, src, addr_len);
		if (padding)
			bzero((void *) packet + packet->src_addr + addr_len,
			    padding);
	} else {
		bzero((void *) packet + packet->src_addr, allocated);
	}

	if (dest) {
		memcpy((void *) packet + packet->dest_addr, dest, addr_len);
		if (padding)
			bzero((void *) packet + packet->dest_addr + addr_len,
			    padding);
	} else {
		bzero((void *) packet + packet->dest_addr, allocated);
	}

	return EOK;
}

/** Return the packet copy.
 *
 * Copy the addresses, data, order and metric values.
 * Queue placement is not copied.
 *
 * @param[in] sess   Packet server module session.
 * @param[in] packet Original packet.
 *
 * @return Packet copy.
 * @return NULL on error.
 *
 */
packet_t *packet_get_copy(async_sess_t *sess, packet_t *packet)
{
	if (!packet_is_valid(packet))
		return NULL;
	
	/* Get a new packet */
	packet_t *copy = packet_get_4_remote(sess, PACKET_DATA_LENGTH(packet),
	    PACKET_MAX_ADDRESS_LENGTH(packet), packet->max_prefix,
	    PACKET_MIN_SUFFIX(packet));
	
	if (!copy)
		return NULL;
	
	/* Get addresses */
	uint8_t *src = NULL;
	uint8_t *dest = NULL;
	size_t addrlen = packet_get_addr(packet, &src, &dest);
	
	/* Copy data */
	if ((packet_copy_data(copy, packet_get_data(packet),
	    PACKET_DATA_LENGTH(packet)) == EOK) &&
	    /* Copy addresses if present */
	    ((addrlen <= 0) ||
	    (packet_set_addr(copy, src, dest, addrlen) == EOK))) {
		copy->order = packet->order;
		copy->metric = packet->metric;
		return copy;
	} else {
		pq_release_remote(sess, copy->packet_id);
		return NULL;
	}
}

/** @}
 */
