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

/** @addtogroup packet
 *  @{
 */

/** @file
 *  Packet map and queue.
 */

#ifndef __NET_PACKET_H__
#define __NET_PACKET_H__

/** Packet identifier type.
 *  Value zero (0) is used as an invalid identifier.
 */
typedef int	packet_id_t;

/** Type definition of the packet.
 *  @see packet
 */
typedef struct packet *	packet_t;

/** Type definition of the packet pointer.
 *  @see packet
 */
typedef packet_t *		packet_ref;

/** Type definition of the packet dimension.
 *  @see packet_dimension
 */
typedef struct packet_dimension	packet_dimension_t;

/** Type definition of the packet dimension pointer.
 *  @see packet_dimension
 */
typedef packet_dimension_t *	packet_dimension_ref;

/** Packet dimension.
 */
struct packet_dimension{
	/** Reserved packet prefix length.
	 */
	size_t prefix;
	/** Maximal packet content length.
	 */
	size_t content;
	/** Reserved packet suffix length.
	 */
	size_t suffix;
	/** Maximal packet address length.
	 */
	size_t addr_len;
};

/** @name Packet management system interface
 */
/*@{*/

/** Finds the packet mapping.
 *  @param[in] packet_id The packet identifier to be found.
 *  @returns The found packet reference.
 *  @returns NULL if the mapping does not exist.
 */
packet_t pm_find(packet_id_t packet_id);

/** Adds the packet mapping.
 *  @param[in] packet The packet to be remembered.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet is not valid.
 *  @returns EINVAL if the packet map is not initialized.
 *  @returns ENOMEM if there is not enough memory left.
 */
int pm_add(packet_t packet);

/** Initializes the packet map.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left.
 */
int pm_init(void);

/** Releases the packet map.
 */
void pm_destroy(void);

/** Add packet to the sorted queue.
 *  The queue is sorted in the ascending order.
 *  The packet is inserted right before the packets of the same order value.
 *  @param[in,out] first The first packet of the queue. Sets the first packet of the queue. The original first packet may be shifted by the new packet.
 *  @param[in] packet The packet to be added.
 *  @param[in] order The packet order value.
 *  @param[in] metric The metric value of the packet.
 *  @returns EOK on success.
 *  @returns EINVAL if the first parameter is NULL.
 *  @returns EINVAL if the packet is not valid.
 */
int pq_add(packet_t * first, packet_t packet, size_t order, size_t metric);

/** Finds the packet with the given order.
 *  @param[in] first The first packet of the queue.
 *  @param[in] order The packet order value.
 *  @returns The packet with the given order.
 *  @returns NULL if the first packet is not valid.
 *  @returns NULL if the packet is not found.
 */
packet_t pq_find(packet_t first, size_t order);

/** Inserts packet after the given one.
 *  @param[in] packet The packet in the queue.
 *  @param[in] new_packet The new packet to be inserted.
 *  @returns EOK on success.
 *  @returns EINVAL if etiher of the packets is invalid.
 */
int pq_insert_after(packet_t packet, packet_t new_packet);

/** Detach the packet from the queue.
 *  @param[in] packet The packet to be detached.
 *  @returns The next packet in the queue. If the packet is the first one of the queue, this becomes the new first one.
 *  @returns NULL if there is no packet left.
 *  @returns NULL if the packet is not valid.
 */
packet_t pq_detach(packet_t packet);

/** Sets the packet order and metric attributes.
 *  @param[in] packet The packet to be set.
 *  @param[in] order The packet order value.
 *  @param[in] metric The metric value of the packet.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet is invalid..
 */
int pq_set_order(packet_t packet, size_t order, size_t metric);

/** Sets the packet order and metric attributes.
 *  @param[in] packet The packet to be set.
 *  @param[out] order The packet order value.
 *  @param[out] metric The metric value of the packet.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet is invalid..
 */
int pq_get_order(packet_t packet, size_t * order, size_t * metric);

/** Releases the whole queue.
 *  Detaches all packets of the queue and calls the packet_release() for each of them.
 *  @param[in] first The first packet of the queue.
 *  @param[in] packet_release The releasing function called for each of the packets after its detachment.
 */
void pq_destroy(packet_t first, void (*packet_release)(packet_t packet));

/** Returns the next packet in the queue.
 *  @param[in] packet The packet queue member.
 *  @returns The next packet in the queue.
 *  @returns NULL if there is no next packet.
 *  @returns NULL if the packet is not valid.
 */
packet_t pq_next(packet_t packet);

/** Returns the previous packet in the queue.
 *  @param[in] packet The packet queue member.
 *  @returns The previous packet in the queue.
 *  @returns NULL if there is no previous packet.
 *  @returns NULL if the packet is not valid.
 */
packet_t pq_previous(packet_t packet);

/*@}*/

#endif

/** @}
 */
