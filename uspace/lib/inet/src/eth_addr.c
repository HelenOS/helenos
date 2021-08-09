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

#include <inet/eth_addr.h>
#include <mem.h>
#include <stdio.h>

/** Ethernet broadcast address */
const eth_addr_t eth_addr_broadcast =
    ETH_ADDR_INITIALIZER(0xff, 0xff, 0xff, 0xff, 0xff, 0xff);

/** Encode Ethernet address to buffer.
 *
 * Encode Ethernet address as a sequence of ETH_ADDR_SIZE bytes into a buffer.
 *
 * @param addr Ethernet address
 * @param buf Buffer (ETH_ADDR_SIZE bytes in size) to store bytes
 */
void eth_addr_encode(eth_addr_t *addr, uint8_t *buf)
{
	uint64_t a;
	int i;

	a = addr->a;

	for (i = 0; i < ETH_ADDR_SIZE; i++)
		buf[i] = (a >> (40 - 8 * i)) & 0xff;
}

/** Decode Ethernet address from buffer.
 *
 * Decode Ethernet address from a buffer containing a sequence of
 * ETH_ADDR_SIZE bytes.
 *
 * @param buf Buffer (ETH_ADDR_SIZE bytes in size)
 * @param addr Place to store Ethernet address
 */
void eth_addr_decode(const uint8_t *buf, eth_addr_t *addr)
{
	uint64_t a;
	int i;

	a = 0;
	for (i = 0; i < ETH_ADDR_SIZE; i++)
		a |= (uint64_t)buf[i] << (40 - 8 * i);

	addr->a = a;
}

/** Compare Ethernet addresses.
 *
 * @param a First address
 * @param b Second address,
 * @return -1, 0, 1 iff @a a is less than, equal to or greater than @a b,
 *         respectively.
 */
int eth_addr_compare(const eth_addr_t *a, const eth_addr_t *b)
{
	if (a->a < b->a)
		return -1;
	else if (a->a == b->a)
		return 0;
	else
		return 1;
}

/** Format Ethernet address as a string.
 *
 * @param addr Ethernet address
 * @param saddr Structure for storing string representation
 *        of @a addr. The caller can access it as @a saddr->str.
 */
void eth_addr_format(eth_addr_t *addr, eth_addr_str_t *saddr)
{
	int i;

	snprintf(saddr->str, 3, "%02x",
	    (unsigned)((addr->a >> 40) & 0xff));
	for (i = 1; i < ETH_ADDR_SIZE; i++) {
		snprintf(saddr->str + 2 + 3 * (i - 1), 4, ":%02x",
		    (unsigned)((addr->a >> (40 - i * 8)) & 0xff));
	}
}

/** @}
 */
