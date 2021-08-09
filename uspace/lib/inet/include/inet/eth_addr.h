/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup libinet
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef LIBINET_INET_ETH_ADDR_H
#define LIBINET_INET_ETH_ADDR_H

#include <stdint.h>

#define ETH_ADDR_SIZE 6
#define ETH_ADDR_STR_SIZE (6 * 2 + 5)

#define ETH_ADDR_INITIALIZER(aa, bb, cc, dd, ee, ff) \
    { .a = ((uint64_t)(aa) << 40) | ((uint64_t)(bb) << 32) | \
    ((uint64_t)(cc) << 24) | ((uint64_t)(dd) << 16) | \
    ((uint64_t)(ee) << 8) | (ff) }

/** Ethernet address.
 *
 * Defined as a structure. This provides strong type checking.
 *
 * Since the structure is not opaque, this allows eth_addr_t to be
 * allocated statically and copied around using the assignment operator.
 *
 * It is stored in the lower 48 bits of a 64-bit integer. This is an internal
 * representation that allows simple and efficient operation. Most CPUs
 * will be much faster (and we will need less instructions) operating
 * on a single 64-bit integer than on six individual 8-bit integers.
 *
 * Kind reader will appreciate the cleverness and elegance of this
 * representation.
 */
typedef struct {
	uint64_t a;
} eth_addr_t;

/** Ethernet address in the form of a string */
typedef struct {
	char str[ETH_ADDR_STR_SIZE + 1];
} eth_addr_str_t;

extern const eth_addr_t eth_addr_broadcast;

extern void eth_addr_encode(eth_addr_t *, uint8_t *);
extern void eth_addr_decode(const uint8_t *, eth_addr_t *);

extern int eth_addr_compare(const eth_addr_t *, const eth_addr_t *);
extern void eth_addr_format(eth_addr_t *, eth_addr_str_t *);

#endif

/** @}
 */
