/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_INET_ADDR_H
#define LIBINET_INET_ADDR_H

#include <errno.h>
#include <inet/eth_addr.h>
#include <stdint.h>

typedef uint32_t addr32_t;

typedef uint8_t addr128_t[16];

typedef enum {
	/** Any IP protocol version */
	ip_any,
	/** IPv4 */
	ip_v4,
	/** IPv6 */
	ip_v6
} ip_ver_t;

/** Node address */
typedef struct {
	/** IP version */
	ip_ver_t version;
	union {
		addr32_t addr;
		addr128_t addr6;
	};
} inet_addr_t;

/** Network address */
typedef struct {
	/** IP version */
	ip_ver_t version;

	/** Address */
	union {
		addr32_t addr;
		addr128_t addr6;
	};

	/** Number of valid bits */
	uint8_t prefix;
} inet_naddr_t;

extern const addr32_t addr32_broadcast_all_hosts;

extern void addr128(const addr128_t, addr128_t);

extern int addr128_compare(const addr128_t, const addr128_t);
extern void eth_addr_solicited_node(const addr128_t, eth_addr_t *);

extern void host2addr128_t_be(const addr128_t, addr128_t);
extern void addr128_t_be2host(const addr128_t, addr128_t);

extern void inet_addr(inet_addr_t *, uint8_t, uint8_t, uint8_t, uint8_t);
extern void inet_naddr(inet_naddr_t *, uint8_t, uint8_t, uint8_t, uint8_t,
    uint8_t);

extern void inet_addr6(inet_addr_t *, uint16_t, uint16_t, uint16_t, uint16_t,
    uint16_t, uint16_t, uint16_t, uint16_t);
extern void inet_naddr6(inet_naddr_t *, uint16_t, uint16_t, uint16_t, uint16_t,
    uint16_t, uint16_t, uint16_t, uint16_t, uint8_t);

extern void inet_naddr_addr(const inet_naddr_t *, inet_addr_t *);
extern void inet_addr_naddr(const inet_addr_t *, uint8_t, inet_naddr_t *);

extern void inet_addr_any(inet_addr_t *);
extern void inet_naddr_any(inet_naddr_t *);

extern int inet_addr_compare(const inet_addr_t *, const inet_addr_t *);
extern int inet_addr_is_any(const inet_addr_t *);

extern int inet_naddr_compare(const inet_naddr_t *, const inet_addr_t *);
extern int inet_naddr_compare_mask(const inet_naddr_t *, const inet_addr_t *);

extern errno_t inet_addr_parse(const char *, inet_addr_t *, char **);
extern errno_t inet_naddr_parse(const char *, inet_naddr_t *, char **);

extern errno_t inet_addr_format(const inet_addr_t *, char **);
extern errno_t inet_naddr_format(const inet_naddr_t *, char **);

extern ip_ver_t inet_addr_get(const inet_addr_t *, addr32_t *, addr128_t *);
extern ip_ver_t inet_naddr_get(const inet_naddr_t *, addr32_t *, addr128_t *,
    uint8_t *);

extern void inet_addr_set(addr32_t, inet_addr_t *);
extern void inet_naddr_set(addr32_t, uint8_t, inet_naddr_t *);

extern void inet_addr_set6(addr128_t, inet_addr_t *);
extern void inet_naddr_set6(addr128_t, uint8_t, inet_naddr_t *);

#endif

/** @}
 */
