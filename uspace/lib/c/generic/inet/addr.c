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

#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <inet/addr.h>
#include <stdio.h>
#include <malloc.h>
#include <bitops.h>

#define INET_PREFIXSTRSIZE  5

#define INET6_ADDRSTRLEN (8 * 4 + 7 + 1)

#if !(defined(__BE__) ^ defined(__LE__))
	#error The architecture must be either big-endian or little-endian.
#endif

const addr32_t addr32_broadcast_all_hosts = 0xffffffff;

const addr48_t addr48_broadcast = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static const addr48_t inet_addr48_solicited_node = {
	0x33, 0x33, 0xff, 0, 0, 0
};

static const inet_addr_t inet_addr_any_addr = {
	.version = ip_v4,
	.addr = 0
};

static const inet_addr_t inet_addr_any_addr6 = {
	.version = ip_v6,
	.addr6 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

void addr48(const addr48_t src, addr48_t dst)
{
	memcpy(dst, src, 6);
}

void addr128(const addr128_t src, addr128_t dst)
{
	memcpy(dst, src, 16);
}

/** Compare addr48.
  *
  * @return Non-zero if equal, zero if not equal.
  */
int addr48_compare(const addr48_t a, const addr48_t b)
{
	return memcmp(a, b, 6) == 0;
}

/** Compare addr128.
  *
  * @return Non-zero if equal, zero if not equal.
  */
int addr128_compare(const addr128_t a, const addr128_t b)
{
	return memcmp(a, b, 16) == 0;
}

/** Compute solicited node MAC multicast address from target IPv6 address
 *
 * @param ip  Target IPv6 address
 * @param mac Solicited MAC address to be assigned
 *
 */
void addr48_solicited_node(const addr128_t ip, addr48_t mac)
{
	memcpy(mac, inet_addr48_solicited_node, 3);
	memcpy(mac + 3, ip + 13, 3);
}

void host2addr128_t_be(const addr128_t host, addr128_t be)
{
	memcpy(be, host, 16);
}

void addr128_t_be2host(const addr128_t be, addr128_t host)
{
	memcpy(host, be, 16);
}

void inet_addr(inet_addr_t *addr, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
	addr->version = ip_v4;
	addr->addr = ((addr32_t) a << 24) | ((addr32_t) b << 16) |
	    ((addr32_t) c << 8) | ((addr32_t) d);
}

void inet_naddr(inet_naddr_t *naddr, uint8_t a, uint8_t b, uint8_t c, uint8_t d,
    uint8_t prefix)
{
	naddr->version = ip_v4;
	naddr->addr = ((addr32_t) a << 24) | ((addr32_t) b << 16) |
	    ((addr32_t) c << 8) | ((addr32_t) d);
	naddr->prefix = prefix;
}

void inet_addr6(inet_addr_t *addr, uint16_t a, uint16_t b, uint16_t c,
    uint16_t d, uint16_t e, uint16_t f, uint16_t g, uint16_t h)
{
	addr->version = ip_v6;
	addr->addr6[0] = (a >> 8) & 0xff;
	addr->addr6[1] = a & 0xff;
	addr->addr6[2] = (b >> 8) & 0xff;
	addr->addr6[3] = b & 0xff;
	addr->addr6[4] = (c >> 8) & 0xff;
	addr->addr6[5] = c & 0xff;
	addr->addr6[6] = (d >> 8) & 0xff;
	addr->addr6[7] = d & 0xff;
	addr->addr6[8] = (e >> 8) & 0xff;
	addr->addr6[9] = e & 0xff;
	addr->addr6[10] = (f >> 8) & 0xff;
	addr->addr6[11] = f & 0xff;
	addr->addr6[12] = (g >> 8) & 0xff;
	addr->addr6[13] = g & 0xff;
	addr->addr6[14] = (h >> 8) & 0xff;
	addr->addr6[15] = h & 0xff;
}

void inet_naddr6(inet_naddr_t *naddr, uint16_t a, uint16_t b, uint16_t c,
    uint16_t d, uint16_t e, uint16_t f, uint16_t g, uint16_t h, uint8_t prefix)
{
	naddr->version = ip_v6;
	naddr->addr6[0] = (a >> 8) & 0xff;
	naddr->addr6[1] = a & 0xff;
	naddr->addr6[2] = (b >> 8) & 0xff;
	naddr->addr6[3] = b & 0xff;
	naddr->addr6[4] = (c >> 8) & 0xff;
	naddr->addr6[5] = c & 0xff;
	naddr->addr6[6] = (d >> 8) & 0xff;
	naddr->addr6[7] = d & 0xff;
	naddr->addr6[8] = (e >> 8) & 0xff;
	naddr->addr6[9] = e & 0xff;
	naddr->addr6[10] = (f >> 8) & 0xff;
	naddr->addr6[11] = f & 0xff;
	naddr->addr6[12] = (g >> 8) & 0xff;
	naddr->addr6[13] = g & 0xff;
	naddr->addr6[14] = (h >> 8) & 0xff;
	naddr->addr6[15] = h & 0xff;
	naddr->prefix = prefix;
}

void inet_naddr_addr(const inet_naddr_t *naddr, inet_addr_t *addr)
{
	addr->version = naddr->version;
	memcpy(addr->addr6, naddr->addr6, 16);
}

void inet_addr_naddr(const inet_addr_t *addr, uint8_t prefix,
    inet_naddr_t *naddr)
{
	naddr->version = addr->version;
	memcpy(naddr->addr6, addr->addr6, 16);
	naddr->prefix = prefix;
}

void inet_addr_any(inet_addr_t *addr)
{
	addr->version = ip_any;
	memset(addr->addr6, 0, 16);
}

void inet_naddr_any(inet_naddr_t *naddr)
{
	naddr->version = ip_any;
	memset(naddr->addr6, 0, 16);
	naddr->prefix = 0;
}

int inet_addr_compare(const inet_addr_t *a, const inet_addr_t *b)
{
	if (a->version != b->version)
		return 0;

	switch (a->version) {
	case ip_v4:
		return (a->addr == b->addr);
	case ip_v6:
		return addr128_compare(a->addr6, b->addr6);
	default:
		return 0;
	}
}

int inet_addr_is_any(const inet_addr_t *addr)
{
	return ((addr->version == ip_any) ||
	    (inet_addr_compare(addr, &inet_addr_any_addr)) ||
	    (inet_addr_compare(addr, &inet_addr_any_addr6)));
}

int inet_naddr_compare(const inet_naddr_t *naddr, const inet_addr_t *addr)
{
	if (naddr->version != addr->version)
		return 0;
	
	switch (naddr->version) {
	case ip_v4:
		return (naddr->addr == addr->addr);
	case ip_v6:
		return addr128_compare(naddr->addr6, addr->addr6);
	default:
		return 0;
	}
}

int inet_naddr_compare_mask(const inet_naddr_t *naddr, const inet_addr_t *addr)
{
	if (naddr->version != addr->version)
		return 0;

	switch (naddr->version) {
	case ip_v4:
		if (naddr->prefix > 32)
			return 0;

		addr32_t mask =
		    BIT_RANGE(addr32_t, 31, 31 - (naddr->prefix - 1));
		return ((naddr->addr & mask) == (addr->addr & mask));
	case ip_v6:
		if (naddr->prefix > 128)
			return 0;
		
		size_t pos = 0;
		for (size_t i = 0; i < 16; i++) {
			/* Further bits do not matter */
			if (naddr->prefix < pos)
				break;
			
			if (naddr->prefix - pos > 8) {
				/* Comparison without masking */
				if (naddr->addr6[i] != addr->addr6[i])
					return 0;
			} else {
				/* Comparison with masking */
				uint8_t mask =
				    BIT_RANGE(uint8_t, 8, 8 - (naddr->prefix - pos - 1));
				if ((naddr->addr6[i] & mask) != (addr->addr6[i] & mask))
					return 0;
			}
			
			pos += 8;
		}
		
		return 1;
	default:
		return 0;
	}
}

static int inet_addr_parse_v4(const char *str, inet_addr_t *raddr,
    int *prefix)
{
	uint32_t a = 0;
	uint8_t b;
	char *cur = (char *)str;
	size_t i = 0;

	while (i < 4) {
		int rc = str_uint8_t(cur, (const char **)&cur, 10, false, &b);
		if (rc != EOK)
			return rc;

		a = (a << 8) + b;

		i++;

		if (*cur == 0)
			break;

		if (*cur != '.')
			return EINVAL;

		if (i < 4)
			cur++;
	}

	if (prefix != NULL) {
		*prefix = strtoul(cur, &cur, 10);
		if (*prefix > 32)
			return EINVAL;
	}

	if (i != 4 || (*cur != 0))
		return EINVAL;

	raddr->version = ip_v4;
	raddr->addr = a;

	return EOK;
}

static int inet_addr_parse_v6(const char *str, inet_addr_t *raddr, int *prefix)
{
	/* XXX */
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
int inet_addr_parse(const char *text, inet_addr_t *addr)
{
	int rc;

	rc = inet_addr_parse_v4(text, addr, NULL);
	if (rc == EOK)
		return EOK;

	rc = inet_addr_parse_v6(text, addr, NULL);
	if (rc == EOK)
		return EOK;

	return EINVAL;
}

/** Parse network address.
 *
 * @param text  Network address in common notation.
 * @param naddr Place to store network address.
 *
 * @return EOK on success, EINVAL if input is not in valid format.
 *
 */
int inet_naddr_parse(const char *text, inet_naddr_t *naddr)
{
	int rc;
	inet_addr_t addr;
	int prefix;

	rc = inet_addr_parse_v4(text, &addr, &prefix);
	if (rc == EOK) {
		inet_addr_naddr(&addr, prefix, naddr);
		return EOK;
	}

	rc = inet_addr_parse_v6(text, &addr, &prefix);
	if (rc == EOK) {
		inet_addr_naddr(&addr, prefix, naddr);
		return EOK;
	}

	return EINVAL;
}

static int inet_ntop6(const uint8_t *data, char *address, size_t length)
{
	/* Check output buffer size */
	if (length < INET6_ADDRSTRLEN)
		return ENOMEM;
	
	/* Find the longest zero subsequence */
	
	uint16_t zeroes[8];
	uint16_t bioctets[8];
	
	for (size_t i = 8; i > 0; i--) {
		size_t j = i - 1;
		
		bioctets[j] = (data[j << 1] << 8) | data[(j << 1) + 1];
		
		if (bioctets[j] == 0) {
			zeroes[j] = 1;
			if (j < 7)
				zeroes[j] += zeroes[j + 1];
		} else
			zeroes[j] = 0;
	}
	
	size_t wildcard_pos = (size_t) -1;
	size_t wildcard_size = 0;
	
	for (size_t i = 0; i < 8; i++) {
		if (zeroes[i] > wildcard_size) {
			wildcard_pos = i;
			wildcard_size = zeroes[i];
		}
	}
	
	char *cur = address;
	size_t rest = length;
	bool tail_zero = false;
	int ret;
	
	for (size_t i = 0; i < 8; i++) {
		if ((i == wildcard_pos) && (wildcard_size > 1)) {
			ret = snprintf(cur, rest, ":");
			i += wildcard_size - 1;
			tail_zero = true;
		} else if (i == 0) {
			ret = snprintf(cur, rest, "%" PRIx16, bioctets[i]);
			tail_zero = false;
		} else {
			ret = snprintf(cur, rest, ":%" PRIx16, bioctets[i]);
			tail_zero = false;
		}
		
		if (ret < 0)
			return EINVAL;
		
		cur += ret;
		rest -= ret;
	}
	
	if (tail_zero) {
		ret = snprintf(cur, rest, ":");
		if (ret < 0)
			return EINVAL;
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
int inet_addr_format(const inet_addr_t *addr, char **bufp)
{
	int rc = 0;
	
	switch (addr->version) {
	case ip_any:
		rc = asprintf(bufp, "none");
		break;
	case ip_v4:
		rc = asprintf(bufp, "%u.%u.%u.%u", (addr->addr >> 24) & 0xff,
		    (addr->addr >> 16) & 0xff, (addr->addr >> 8) & 0xff,
		    addr->addr & 0xff);
		break;
	case ip_v6:
		*bufp = (char *) malloc(INET6_ADDRSTRLEN);
		if (*bufp == NULL)
			return ENOMEM;
		
		return inet_ntop6(addr->addr6, *bufp, INET6_ADDRSTRLEN);
	default:
		asprintf(bufp, "<ver=%d>", addr->version);
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
int inet_naddr_format(const inet_naddr_t *naddr, char **bufp)
{
	int rc = 0;
	char prefix[INET_PREFIXSTRSIZE];
	
	switch (naddr->version) {
	case ip_any:
		rc = asprintf(bufp, "none");
		break;
	case ip_v4:
		rc = asprintf(bufp, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8
		    "/%" PRIu8, (naddr->addr >> 24) & 0xff,
		    (naddr->addr >> 16) & 0xff, (naddr->addr >> 8) & 0xff,
		    naddr->addr & 0xff, naddr->prefix);
		break;
	case ip_v6:
		*bufp = (char *) malloc(INET6_ADDRSTRLEN + INET_PREFIXSTRSIZE);
		if (*bufp == NULL)
			return ENOMEM;
		
		rc = inet_ntop6(naddr->addr6, *bufp,
		    INET6_ADDRSTRLEN + INET_PREFIXSTRSIZE);
		if (rc != EOK) {
			free(*bufp);
			return rc;
		}
		
		rc = snprintf(prefix, INET_PREFIXSTRSIZE, "/%" PRIu8,
		    naddr->prefix);
		if (rc < 0) {
			free(*bufp);
			return ENOMEM;
		}
		
		str_append(*bufp, INET6_ADDRSTRLEN + INET_PREFIXSTRSIZE, prefix);
		
		break;
	default:
		return ENOTSUP;
	}
	
	if (rc < 0)
		return ENOMEM;
	
	return EOK;
}

ip_ver_t inet_addr_get(const inet_addr_t *addr, addr32_t *v4, addr128_t *v6)
{
	switch (addr->version) {
	case ip_v4:
		if (v4 != NULL)
			*v4 = addr->addr;
		break;
	case ip_v6:
		if (v6 != NULL)
			memcpy(*v6, addr->addr6, 16);
		break;
	default:
		assert(false);
		break;
	}

	return addr->version;
}

ip_ver_t inet_naddr_get(const inet_naddr_t *naddr, addr32_t *v4, addr128_t *v6,
    uint8_t *prefix)
{
	switch (naddr->version) {
	case ip_v4:
		if (v4 != NULL)
			*v4 = naddr->addr;
		if (prefix != NULL)
			*prefix = naddr->prefix;
		break;
	case ip_v6:
		if (v6 != NULL)
			memcpy(*v6, naddr->addr6, 16);
		if (prefix != NULL)
			*prefix = naddr->prefix;
		break;
	default:
		assert(false);
		break;
	}

	return naddr->version;
}

void inet_addr_set(addr32_t v4, inet_addr_t *addr)
{
	addr->version = ip_v4;
	addr->addr = v4;
}

void inet_naddr_set(addr32_t v4, uint8_t prefix, inet_naddr_t *naddr)
{
	naddr->version = ip_v4;
	naddr->addr = v4;
	naddr->prefix = prefix;
}

void inet_addr_set6(addr128_t v6, inet_addr_t *addr)
{
	addr->version = ip_v6;
	memcpy(addr->addr6, v6, 16);
}

void inet_naddr_set6(addr128_t v6, uint8_t prefix, inet_naddr_t *naddr)
{
	naddr->version = ip_v6;
	memcpy(naddr->addr6, v6, 16);
	naddr->prefix = prefix;
}

/** @}
 */
