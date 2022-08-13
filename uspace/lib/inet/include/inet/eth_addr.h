/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
