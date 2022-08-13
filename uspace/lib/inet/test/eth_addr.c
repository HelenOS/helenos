/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
