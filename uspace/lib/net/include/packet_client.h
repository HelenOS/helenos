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
 *  @{
 */

/** @file
 * Packet client.
 *
 * To function correctly, initialization of the packet map by the pm_init()
 * function has to happen at the first place. The module should not send the
 * packet messages to the packet server but use the functions provided. The
 * packet map should be released by the pm_destroy() function during the module
 * termination. The packets and the packet queues can't be locked at all. The
 * processing modules should process them sequentially - by passing the packets
 * to the next module and stopping using the passed ones.
 *
 * @see packet.h
 */

#ifndef LIBNET_PACKET_CLIENT_H_
#define LIBNET_PACKET_CLIENT_H_

#include <net/packet.h>
#include <async.h>

/** @name Packet client interface */
/*@{*/

/** Allocates the specified type right before the actual packet content and
 * returns its pointer.
 *
 * The wrapper of the packet_prepend() function.
 *
 * @param[in] packet	The packet to be used.
 * @param[in] type	The type to be allocated at the beginning of the packet
 *			content.
 * @return		The typed pointer to the allocated memory.
 * @return		NULL if the packet is not valid.
 * @return		NULL if there is not enough memory left.
 */
#define PACKET_PREFIX(packet, type) \
	(type *) packet_prefix((packet), sizeof(type))

/** Allocates the specified type right after the actual packet content and
 * returns its pointer.
 *
 * The wrapper of the packet_append() function.
 *
 * @param[in] packet	The packet to be used.
 * @param[in] type	The type to be allocated at the end of the packet
 *			content.
 * @return		The typed pointer to the allocated memory.
 * @return		NULL if the packet is not valid.
 * @return		NULL if there is not enough memory left.
 */
#define PACKET_SUFFIX(packet, type) \
	(type *) packet_suffix((packet), sizeof(type))

/** Trims the actual packet content by the specified prefix and suffix types.
 *
 * The wrapper of the packet_trim() function.
 *
 * @param[in] packet	The packet to be trimmed.
 * @param[in] prefix	The type of the prefix to be removed from the beginning
 *			of the packet content.
 * @param[in] suffix	The type of the suffix to be removed from the end of
 *			the packet content.
 * @return		EOK on success.
 * @return		EINVAL if the packet is not valid.
 * @return		ENOMEM if there is not enough memory left.
 */
#define PACKET_TRIM(packet, prefix, suffix) \
	packet_trim((packet), sizeof(prefix), sizeof(suffix))

extern void *packet_prefix(packet_t *, size_t);
extern void *packet_suffix(packet_t *, size_t);
extern int packet_trim(packet_t *, size_t, size_t);
extern int packet_copy_data(packet_t *, const void *, size_t);
extern packet_id_t packet_get_id(const packet_t *);
extern size_t packet_get_data_length(const packet_t *);
extern void *packet_get_data(const packet_t *);
extern int packet_get_addr(const packet_t *, uint8_t **, uint8_t **);
extern int packet_set_addr(packet_t *, const uint8_t *, const uint8_t *, size_t);
extern packet_t *packet_get_copy(async_sess_t *, packet_t *);

/*@}*/

#endif

/** @}
 */
