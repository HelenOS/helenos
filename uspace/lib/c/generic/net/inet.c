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

/** Prints the address into the character buffer.
 *
 *  @param[in] family	The address family.
 *  @param[in] data	The address data.
 *  @param[out] address	The character buffer to be filled.
 *  @param[in] length	The buffer length.
 *  @return		EOK on success.
 *  @return		EINVAL if the data or address parameter is NULL.
 *  @return		ENOMEM if the character buffer is not long enough.
 *  @return		ENOTSUP if the address family is not supported.
 */
int
inet_ntop(uint16_t family, const uint8_t *data, char *address, size_t length)
{
	if ((!data) || (!address))
		return EINVAL;

	switch (family) {
	case AF_INET:
		/* Check output buffer size */
		if (length < INET_ADDRSTRLEN)
			return ENOMEM;
			
		/* Fill buffer with IPv4 address */
		snprintf(address, length, "%hhu.%hhu.%hhu.%hhu",
		    data[0], data[1], data[2], data[3]);

		return EOK;

	case AF_INET6:
		/* Check output buffer size */
		if (length < INET6_ADDRSTRLEN)
			return ENOMEM;
		
		/* Fill buffer with IPv6 address */
		snprintf(address, length,
		    "%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx:%hhx%hhx:"
		    "%hhx%hhx:%hhx%hhx",
		    data[0], data[1], data[2], data[3], data[4], data[5],
		    data[6], data[7], data[8], data[9], data[10], data[11],
		    data[12], data[13], data[14], data[15]);
		
		return EOK;

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
	// FIXME TODO
	return ENOTSUP;
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
