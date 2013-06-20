/*
 * Copyright (c) 2013 Jiri Svoboda
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
 * @{
 */
/** @file Internet address parsing and formatting.
 */

#include <errno.h>
#include <unistd.h>
#include <inet/addr2.h>
#include <net/socket_codes.h>
#include <net/inet.h>
#include <stdio.h>

// TODO temporarily
#include <assert.h>

static inet2_addr_t inet2_addr_any = {
	.family = AF_INET,
	.addr = {0, 0, 0, 0}
};

static inet2_addr_t inet2_addr6_any = {
	.family = AF_INET6,
	.addr = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/** Parse network address family.
 *
 * @param text Network address in common notation.
 * @param af   Place to store network address family.
 *
 * @return EOK on success, EINVAL if input is not in valid format.
 *
 */
int inet2_addr_family(const char *text, uint16_t *af)
{
	char *dot = str_chr(text, '.');
	if (dot != NULL) {
		*af = AF_INET;
		return EOK;
	}
	
	char *collon = str_chr(text, ':');
	if (collon != NULL) {
		*af = AF_INET6;
		return EOK;
	}
	
	*af = AF_NONE;
	return EINVAL;
}

/** Parse node address.
 *
 * @param text Network address in common notation.
 * @param addr Place to store node address.
 *
 * @return EOK on success, EINVAL if input is not in valid format.
 *
 */
int inet2_addr_parse(const char *text, inet2_addr_t *addr)
{
	int rc = inet2_addr_family(text, &addr->family);
	if (rc != EOK)
		return rc;
	
	rc = inet_pton(addr->family, text, addr->addr);
	if (rc != EOK)
		return rc;
	
	return EOK;
}

/** Parse network address.
 *
 * @param text  Network address in common notation.
 * @param naddr Place to store network address.
 *
 * @return EOK on success, EINVAL if input is not in valid format.
 *
 */
int inet2_naddr_parse(const char *text, inet2_naddr_t *naddr)
{
	char *slash = str_chr(text, '/');
	if (slash == NULL)
		return EINVAL;
	
	*slash = 0;
	
	int rc = inet2_addr_family(text, &naddr->family);
	if (rc != EOK)
		return rc;
	
	rc = inet_pton(naddr->family, text, naddr->addr);
	*slash = '/';
	
	if (rc != EOK)
		return rc;
	
	slash++;
	
	switch (naddr->family) {
	case AF_INET:
		naddr->prefix = strtoul(slash, &slash, 10);
		if (naddr->prefix > 32)
			return EINVAL;
		break;
	case AF_INET6:
		naddr->prefix = strtoul(slash, &slash, 10);
		if (naddr->prefix > 128)
			return EINVAL;
		break;
	default:
		return ENOTSUP;
	}
	
	return EOK;
}

/** Format node address.
 *
 * @param addr Node address.
 * @param bufp Place to store pointer to formatted string.
 *
 * @return EOK on success.
 * @return ENOMEM if out of memory.
 * @return ENOTSUP on unsupported address family.
 *
 */
int inet2_addr_format(inet2_addr_t *addr, char **bufp)
{
	int rc;
	
	switch (addr->family) {
	case AF_NONE:
		rc = asprintf(bufp, "none");
		break;
	case AF_INET:
		rc = asprintf(bufp, "%u.%u.%u.%u", addr->addr[0],
		    addr->addr[1], addr->addr[2], addr->addr[3]);
		break;
	case AF_INET6:
		// FIXME TODO
		break;
	default:
		return ENOTSUP;
	}
	
	if (rc < 0)
		return ENOMEM;
	
	return EOK;
}

/** Format network address.
 *
 * @param naddr Network address.
 * @param bufp  Place to store pointer to formatted string.
 *
 * @return EOK on success.
 * @return ENOMEM if out of memory.
 * @return ENOTSUP on unsupported address family.
 *
 */
int inet2_naddr_format(inet2_naddr_t *naddr, char **bufp)
{
	int rc;
	
	switch (naddr->family) {
	case AF_NONE:
		rc = asprintf(bufp, "none");
		break;
	case AF_INET:
		rc = asprintf(bufp, "%u.%u.%u.%u/%u", naddr->addr[0],
		    naddr->addr[1], naddr->addr[2], naddr->addr[3],
		    naddr->prefix);
		break;
	case AF_INET6:
		// FIXME TODO
		break;
	default:
		return ENOTSUP;
	}
	
	if (rc < 0)
		return ENOMEM;
	
	return EOK;
}

/** Create packed IPv4 address
 *
 * Convert an IPv4 address to a packed 32 bit representation.
 *
 * @param addr   Source address.
 * @param packed Place to store the packed 32 bit representation.
 *
 * @return EOK on success.
 * @return EINVAL if addr is not an IPv4 address.
 *
 */
int inet2_addr_pack(inet2_addr_t *addr, uint32_t *packed)
{
	if (addr->family != AF_INET)
		return EINVAL;
	
	*packed = (addr->addr[0] << 24) | (addr->addr[1] << 16) |
	    (addr->addr[2] << 8) | addr->addr[3];
	return EOK;
}

/** Create packed IPv4 address
 *
 * Convert an IPv4 address to a packed 32 bit representation.
 *
 * @param naddr  Source address.
 * @param packed Place to store the packed 32 bit representation.
 * @param prefix Place to store the number of valid bits.
 *
 * @return EOK on success.
 * @return EINVAL if addr is not an IPv4 address.
 *
 */
int inet2_naddr_pack(inet2_naddr_t *naddr, uint32_t *packed, uint8_t *prefix)
{
	if (naddr->family != AF_INET)
		return EINVAL;
	
	*packed = (naddr->addr[0] << 24) | (naddr->addr[1] << 16) |
	    (naddr->addr[2] << 8) | naddr->addr[3];
	*prefix = naddr->prefix;
	
	return EOK;
}

void inet2_addr_unpack(uint32_t packed, inet2_addr_t *addr)
{
	addr->family = AF_INET;
	addr->addr[0] = (packed >> 24) & 0xff;
	addr->addr[1] = (packed >> 16) & 0xff;
	addr->addr[2] = (packed >> 8) & 0xff;
	addr->addr[3] = packed & 0xff;
}

void inet2_naddr_unpack(uint32_t packed, uint8_t prefix, inet2_naddr_t *naddr)
{
	naddr->family = AF_INET;
	naddr->addr[0] = (packed >> 24) & 0xff;
	naddr->addr[1] = (packed >> 16) & 0xff;
	naddr->addr[2] = (packed >> 8) & 0xff;
	naddr->addr[3] = packed & 0xff;
	naddr->prefix = prefix;
}

int inet2_addr_sockaddr_in(inet2_addr_t *addr, sockaddr_in_t *sockaddr_in)
{
	uint32_t packed;
	int rc = inet2_addr_pack(addr, &packed);
	if (rc != EOK)
		return rc;
	
	sockaddr_in->sin_family = AF_INET;
	sockaddr_in->sin_addr.s_addr = host2uint32_t_be(packed);
	return EOK;
}

void inet2_naddr_addr(inet2_naddr_t *naddr, inet2_addr_t *addr)
{
	addr->family = naddr->family;
	memcpy(addr->addr, naddr->addr, INET2_ADDR_SIZE);
}

void inet2_addr(inet2_addr_t *addr, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
	addr->family = AF_INET;
	addr->addr[0] = a;
	addr->addr[1] = b;
	addr->addr[2] = c;
	addr->addr[3] = d;
}

void inet2_naddr(inet2_naddr_t *naddr, uint8_t a, uint8_t b, uint8_t c, uint8_t d,
    uint8_t prefix)
{
	naddr->family = AF_INET;
	naddr->addr[0] = a;
	naddr->addr[1] = b;
	naddr->addr[2] = c;
	naddr->addr[3] = d;
	naddr->prefix = prefix;
}

void inet2_addr_empty(inet2_addr_t *addr)
{
	addr->family = 0;
	memset(addr->addr, 0, INET2_ADDR_SIZE);
}

void inet2_naddr_empty(inet2_naddr_t *naddr)
{
	naddr->family = 0;
	memset(naddr->addr, 0, INET2_ADDR_SIZE);
	naddr->prefix = 0;
}

int inet2_addr_compare(inet2_addr_t *a, inet2_addr_t *b)
{
	if (a->family != b->family)
		return 0;
	
	switch (a->family) {
	case AF_INET:
		return ((a->addr[0] == b->addr[0]) && (a->addr[1] == b->addr[1]) &&
		    (a->addr[2] == b->addr[2]) && (a->addr[3] == b->addr[3]));
	case AF_INET6:
		// FIXME TODO
		return 0;
	default:
		return 0;
	}
}

int inet2_addr_is_any(inet2_addr_t *addr)
{
	return ((addr->family == 0) ||
	    (inet2_addr_compare(addr, &inet2_addr_any)) ||
	    (inet2_addr_compare(addr, &inet2_addr6_any)));
}

void inet_inet2(inet_addr_t *addr, inet2_addr_t *addr2)
{
	// TODO temporarily
	inet2_addr_unpack(addr->ipv4, addr2);
}

void inet2_inet(inet2_addr_t *addr2, inet_addr_t *addr)
{
	// TODO temporarily
	assert(addr2->family == AF_INET);
	inet2_addr_pack(addr2, &addr->ipv4);
}

/** @}
 */
