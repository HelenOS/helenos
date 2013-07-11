/*
 * Copyright (c) 2013 Martin Decky
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
 * Internet protocol address conversion functions implementation.
 */

#include <net/socket_codes.h>
#include <net/in.h>
#include <net/in6.h>
#include <net/inet.h>
#include <inet/addr.h>
#include <errno.h>
#include <mem.h>
#include <stdio.h>
#include <str.h>

const in6_addr_t in6addr_any = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

static int inet_ntop4(const uint8_t *data, char *address, size_t length)
{
	/* Check output buffer size */
	if (length < INET_ADDRSTRLEN)
		return ENOMEM;
	
	/* Fill buffer with IPv4 address */
	snprintf(address, length,
	    "%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8,
	    data[0], data[1], data[2], data[3]);
	
	return EOK;
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

/** Prints the address into the character buffer.
 *
 * @param[in]  family  Address family.
 * @param[in]  data    Address data.
 * @param[out] address Character buffer to be filled.
 * @param[in]  length  Buffer length.
 *
 * @return EOK on success.
 * @return EINVAL if the data or address parameter is NULL.
 * @return ENOMEM if the character buffer is not long enough.
 * @return ENOTSUP if the address family is not supported.
 *
 */
int inet_ntop(uint16_t family, const uint8_t *data, char *address, size_t length)
{
	switch (family) {
	case AF_INET:
		return inet_ntop4(data, address, length);
	case AF_INET6:
		return inet_ntop6(data, address, length);
	default:
		return ENOTSUP;
	}
}

static int inet_pton4(const char *address, uint8_t *data)
{
	memset(data, 0, 4);
	
	const char *cur = address;
	size_t i = 0;
	
	while (i < 4) {
		int rc = str_uint8_t(cur, &cur, 10, false, &data[i]);
		if (rc != EOK)
			return rc;
		
		i++;
		
		if (*cur == 0)
			break;
		
		if (*cur != '.')
			return EINVAL;
		
		if (i < 4)
			cur++;
	}
	
	if ((i == 4) && (*cur != 0))
		return EINVAL;
	
	return EOK;
}

static int inet_pton6(const char *address, uint8_t *data)
{
	memset(data, 0, 16);
	
	const char *cur = address;
	size_t i = 0;
	size_t wildcard_pos = (size_t) -1;
	size_t wildcard_size = 0;
	
	/* Handle initial wildcard */
	if ((address[0] == ':') && (address[1] == ':')) {
		cur = address + 2;
		wildcard_pos = 0;
		wildcard_size = 16;
		
		/* Handle empty address */
		if (*cur == 0)
			return EOK;
	}
	
	while (i < 16) {
		uint16_t bioctet;
		int rc = str_uint16_t(cur, &cur, 16, false, &bioctet);
		if (rc != EOK)
			return rc;
		
		data[i] = (bioctet >> 8) & 0xff;
		data[i + 1] = bioctet & 0xff;
		
		if (wildcard_pos != (size_t) -1) {
			if (wildcard_size < 2)
				return EINVAL;
			
			wildcard_size -= 2;
		}
		
		i += 2;
		
		if (*cur == 0)
			break;
		
		if (*cur != ':')
			return EINVAL;
		
		if (i < 16) {
			cur++;
			
			/* Handle wildcard */
			if (*cur == ':') {
				if (wildcard_pos != (size_t) -1)
					return EINVAL;
				
				wildcard_pos = i;
				wildcard_size = 16 - i;
				cur++;
				
				if (*cur == 0)
					break;
			}
		}
	}
	
	if ((i == 16) && (*cur != 0))
		return EINVAL;
	
	/* Create wildcard positions */
	if ((wildcard_pos != (size_t) -1) && (wildcard_size > 0)) {
		size_t wildcard_shift = 16 - wildcard_size;
		
		for (i = wildcard_pos + wildcard_shift; i > wildcard_pos; i--) {
			size_t j = i - 1;
			data[j + wildcard_size] = data[j];
			data[j] = 0;
		}
	}
	
	return EOK;
}

/** Parses the character string into the address.
 *
 * @param[in]  family  The address family.
 * @param[in]  address The character buffer to be parsed.
 * @param[out] data    The address data to be filled.
 *
 * @return EOK on success.
 * @return EINVAL if the data parameter is NULL.
 * @return ENOENT if the address parameter is NULL.
 * @return ENOTSUP if the address family is not supported.
 *
 */
int inet_pton(uint16_t family, const char *address, uint8_t *data)
{
	switch (family) {
	case AF_INET:
		return inet_pton4(address, data);
	case AF_INET6:
		return inet_pton6(address, data);
	default:
		/** Unknown address family */
		return ENOTSUP;
	}
}

/** @}
 */
