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

#include <errno.h>
#include <inet/eth_addr.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(eth_addr);

/** Test ETH_ADDR_INITIALIZER constructs correct Ethernet address */
PCUT_TEST(initializer)
{
	eth_addr_t addr = ETH_ADDR_INITIALIZER(0x11, 0x22, 0x33, 0x44, 0x55,
	    0x66);
	PCUT_ASSERT_INT_EQUALS(0x112233445566, addr.a);
}

/** Test eth_addr_decode() correctly decodes from array of bytes */
PCUT_TEST(decode)
{
	eth_addr_t addr;
	uint8_t b[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };

	eth_addr_decode(b, &addr);
	PCUT_ASSERT_INT_EQUALS(0x112233445566, addr.a);
}

/** Test eth_addr_encode() correctly encodes to array of bytes */
PCUT_TEST(encode)
{
	eth_addr_t addr;
	uint8_t b[7] = { 0, 0, 0, 0, 0, 0, 0 };

	addr.a = 0x112233445566;
	eth_addr_encode(&addr, b);

	PCUT_ASSERT_INT_EQUALS(0x11, b[0]);
	PCUT_ASSERT_INT_EQUALS(0x22, b[1]);
	PCUT_ASSERT_INT_EQUALS(0x33, b[2]);
	PCUT_ASSERT_INT_EQUALS(0x44, b[3]);
	PCUT_ASSERT_INT_EQUALS(0x55, b[4]);
	PCUT_ASSERT_INT_EQUALS(0x66, b[5]);
}

/** Test eth_addr_compare() correctly compares two Ethernet addresses */
PCUT_TEST(compare)
{
	eth_addr_t a;
	eth_addr_t b;

	a.a = 1;
	b.a = 2;
	PCUT_ASSERT_INT_EQUALS(-1, eth_addr_compare(&a, &b));

	a.a = 2;
	b.a = 2;
	PCUT_ASSERT_INT_EQUALS(0, eth_addr_compare(&a, &b));

	a.a = 2;
	b.a = 1;
	PCUT_ASSERT_INT_EQUALS(1, eth_addr_compare(&a, &b));
}

/** Tets eth_addr_format() correctly formats Ethernet address to string */
PCUT_TEST(format)
{
	eth_addr_t addr1 = ETH_ADDR_INITIALIZER(0x11, 0x22, 0x33, 0x44, 0x55, 0x66);
	eth_addr_t addr2 = ETH_ADDR_INITIALIZER(0x01, 0x02, 0x03, 0x04, 0x05, 0x06);
	eth_addr_t addr3 = ETH_ADDR_INITIALIZER(0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff);
	eth_addr_str_t saddr;

	eth_addr_format(&addr1, &saddr);
	PCUT_ASSERT_STR_EQUALS("11:22:33:44:55:66", saddr.str);

	eth_addr_format(&addr2, &saddr);
	PCUT_ASSERT_STR_EQUALS("01:02:03:04:05:06", saddr.str);

	eth_addr_format(&addr3, &saddr);
	PCUT_ASSERT_STR_EQUALS("aa:bb:cc:dd:ee:ff", saddr.str);
}

PCUT_EXPORT(eth_addr);
