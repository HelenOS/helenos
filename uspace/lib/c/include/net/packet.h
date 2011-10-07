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
 *  Packet map and queue.
 */

#ifndef LIBC_PACKET_H_
#define LIBC_PACKET_H_

/** Packet identifier type.
 * Value zero is used as an invalid identifier.
 */
typedef unsigned long packet_id_t;

/** Type definition of the packet.
 * @see packet
 */
typedef struct packet packet_t;

/** Type definition of the packet dimension.
 * @see packet_dimension
 */
typedef struct packet_dimension	packet_dimension_t;

/** Packet dimension. */
struct packet_dimension {
	/** Reserved packet prefix length. */
	size_t prefix;
	/** Maximal packet content length. */
	size_t content;
	/** Reserved packet suffix length. */
	size_t suffix;
	/** Maximal packet address length. */
	size_t addr_len;
};

/** @name Packet management system interface
 */
/*@{*/

extern packet_t *pm_find(packet_id_t);
extern int pm_add(packet_t *);
extern void pm_remove(packet_t *);
extern int pm_init(void);
extern void pm_destroy(void);

extern int pq_add(packet_t **, packet_t *, size_t, size_t);
extern packet_t *pq_find(packet_t *, size_t);
extern int pq_insert_after(packet_t *, packet_t *);
extern packet_t *pq_detach(packet_t *);
extern int pq_set_order(packet_t *, size_t, size_t);
extern int pq_get_order(packet_t *, size_t *, size_t *);
extern void pq_destroy(packet_t *, void (*)(packet_t *));
extern packet_t *pq_next(packet_t *);
extern packet_t *pq_previous(packet_t *);

/*@}*/

#endif

/** @}
 */
