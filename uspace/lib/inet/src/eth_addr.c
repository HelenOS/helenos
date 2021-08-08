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

/** @addtogroup libc
 * @{
 */
/**
 * @file
 * @brief
 */

#include <inet/eth_addr.h>
#include <mem.h>

const eth_addr_t eth_addr_broadcast = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

void eth_addr_encode(eth_addr_t *addr, void *buf)
{
	uint8_t *bp = (uint8_t *)buf;

	memcpy(bp, &addr->b[0], ETH_ADDR_SIZE);
}

void eth_addr_decode(const void *buf, eth_addr_t *addr)
{
	const uint8_t *bp = (uint8_t *)buf;

	memcpy(&addr->b[0], bp, ETH_ADDR_SIZE);
}

/** Compare ethernet addresses.
 *
 * @return Non-zero if equal, zero if not equal.
 */
int eth_addr_compare(const eth_addr_t *a, const eth_addr_t *b)
{
	return memcmp(a->b, b->b, ETH_ADDR_SIZE) == 0;
}

/** @}
 */
