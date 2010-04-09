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
 *  Packet client.
 *  The hosting module has to be compiled with both the packet.c and the packet_client.c source files.
 *  To function correctly, initialization of the packet map by the pm_init() function has to happen at the first place.
 *  The module should not send the packet messages to the packet server but use the functions provided.
 *  The packet map should be released by the pm_destroy() function during the module termination.
 *  The packets and the packet queues can't be locked at all.
 *  The processing modules should process them sequentially -&nbsp;by passing the packets to the next module and stopping using the passed ones.
 *  @see packet.h
 */

#ifndef __NET_PACKET_CLIENT_H__
#define __NET_PACKET_CLIENT_H__

#include <packet/packet.h>

/** @name Packet client interface
 */
/*@{*/

/** Allocates the specified type right before the actual packet content and returns its pointer.
 *  The wrapper of the packet_prepend() function.
 *  @param[in] packet The packet to be used.
 *  @param[in] type The type to be allocated at the beginning of the packet content.
 *  @returns The typed pointer to the allocated memory.
 *  @returns NULL if the packet is not valid.
 *  @returns NULL if there is not enough memory left.
 */
#define PACKET_PREFIX(packet, type)	(type *) packet_prefix((packet), sizeof(type))

/** Allocates the specified type right after the actual packet content and returns its pointer.
 *  The wrapper of the packet_append() function.
 *  @param[in] packet The packet to be used.
 *  @param[in] type The type to be allocated at the end of the packet content.
 *  @returns The typed pointer to the allocated memory.
 *  @returns NULL if the packet is not valid.
 *  @returns NULL if there is not enough memory left.
 */
#define PACKET_SUFFIX(packet, type)	(type *) packet_suffix((packet), sizeof(type))

/** Trims the actual packet content by the specified prefix and suffix types.
 *  The wrapper of the packet_trim() function.
 *  @param[in] packet The packet to be trimmed.
 *  @param[in] prefix The type of the prefix to be removed from the beginning of the packet content.
 *  @param[in] suffix The type of the suffix to be removed from the end of the packet content.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet is not valid.
 *  @returns ENOMEM if there is not enough memory left.
 */
#define PACKET_TRIM(packet, prefix, suffix)	packet_trim((packet), sizeof(prefix), sizeof(suffix))

/** Allocates the specified space right before the actual packet content and returns its pointer.
 *  @param[in] packet The packet to be used.
 *  @param[in] length The space length to be allocated at the beginning of the packet content.
 *  @returns The pointer to the allocated memory.
 *  @returns NULL if there is not enough memory left.
 */
extern void * packet_prefix(packet_t packet, size_t length);

/** Allocates the specified space right after the actual packet content and returns its pointer.
 *  @param[in] packet The packet to be used.
 *  @param[in] length The space length to be allocated at the end of the packet content.
 *  @returns The pointer to the allocated memory.
 *  @returns NULL if there is not enough memory left.
 */
extern void * packet_suffix(packet_t packet, size_t length);

/** Trims the actual packet content by the specified prefix and suffix lengths.
 *  @param[in] packet The packet to be trimmed.
 *  @param[in] prefix The prefix length to be removed from the beginning of the packet content.
 *  @param[in] suffix The suffix length to be removed from the end of the packet content.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet is not valid.
 *  @returns ENOMEM if there is not enough memory left.
 */
extern int packet_trim(packet_t packet, size_t prefix, size_t suffix);

/** Copies the specified data to the beginning of the actual packet content.
 *  Pushes the content end if needed.
 *  @param[in] packet The packet to be filled.
 *  @param[in] data The data to be copied.
 *  @param[in] length The length of the copied data.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet is not valid.
 *  @returns ENOMEM if there is not enough memory left.
 */
extern int packet_copy_data(packet_t packet, const void * data, size_t length);

/** Returns the packet identifier.
 *  @param[in] packet The packet.
 *  @returns The packet identifier.
 *  @returns Zero (0) if the packet is not valid.
 */
extern packet_id_t packet_get_id(const packet_t packet);

/** Returns the packet content length.
 *  @param[in] packet The packet.
 *  @returns The packet content length in bytes.
 *  @returns Zero (0) if the packet is not valid.
 */
extern size_t packet_get_data_length(const packet_t packet);

/** Returns the pointer to the beginning of the packet content.
 *  @param[in] packet The packet.
 *  @returns The pointer to the beginning of the packet content.
 *  @returns NULL if the packet is not valid.
 */
extern void * packet_get_data(const packet_t packet);

/** Returns the stored packet addresses and their length.
 *  @param[in] packet The packet.
 *  @param[out] src The source address. May be NULL if not desired.
 *  @param[out] dest The destination address. May be NULL if not desired.
 *  @returns The stored addresses length.
 *  @returns Zero (0) if the addresses are not present.
 *  @returns EINVAL if the packet is not valid.
 */
extern int packet_get_addr(const packet_t packet, uint8_t ** src, uint8_t ** dest);

/** Sets the packet addresses.
 *  @param[in] packet The packet.
 *  @param[in] src The new source address. May be NULL.
 *  @param[in] dest The new destination address. May be NULL.
 *  @param[in] addr_len The addresses length.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet is not valid.
 *  @returns ENOMEM if there is not enough memory left.
 */
extern int packet_set_addr(packet_t packet, const uint8_t * src, const uint8_t * dest, size_t addr_len);

/** Translates the packet identifier to the packet reference.
 *  Tries to find mapping first.
 *  Contacts the packet server to share the packet if the mapping is not present.
 *  @param[in] phone The packet server module phone.
 *  @param[out] packet The packet reference.
 *  @param[in] packet_id The packet identifier.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet parameter is NULL.
 *  @returns Other error codes as defined for the NET_PACKET_GET_SIZE message.
 *  @returns Other error codes as defined for the packet_return() function.
 */
extern int packet_translate_local(int phone, packet_ref packet, packet_id_t packet_id);

/** Obtains the packet of the given dimensions.
 *  Contacts the packet server to return the appropriate packet.
 *  @param[in] phone The packet server module phone.
 *  @param[in] addr_len The source and destination addresses maximal length in bytes.
 *  @param[in] max_prefix The maximal prefix length in bytes.
 *  @param[in] max_content The maximal content length in bytes.
 *  @param[in] max_suffix The maximal suffix length in bytes.
 *  @returns The packet reference.
 *  @returns NULL on error.
 */
extern packet_t packet_get_4_local(int phone, size_t max_content, size_t addr_len, size_t max_prefix, size_t max_suffix);

/** Obtains the packet of the given content size.
 *  Contacts the packet server to return the appropriate packet.
 *  @param[in] phone The packet server module phone.
 *  @param[in] content The maximal content length in bytes.
 *  @returns The packet reference.
 *  @returns NULL on error.
 */
extern packet_t packet_get_1_local(int phone, size_t content);

/** Releases the packet queue.
 *  All packets in the queue are marked as free for use.
 *  The packet queue may be one packet only.
 *  The module should not use the packets after this point until they are received or obtained again.
 *  @param[in] phone The packet server module phone.
 *  @param[in] packet_id The packet identifier.
 */
extern void pq_release_local(int phone, packet_id_t packet_id);

/** Returns the packet copy.
 *  Copies the addresses, data, order and metric values.
 *  Does not copy the queue placement.
 *  @param[in] phone The packet server module phone.
 *  @param[in] packet The original packet.
 *  @returns The packet copy.
 *  @returns NULL on error.
 */
extern packet_t packet_get_copy(int phone, packet_t packet);

/*@}*/

#endif

/** @}
 */
