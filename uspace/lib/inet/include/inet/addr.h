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
