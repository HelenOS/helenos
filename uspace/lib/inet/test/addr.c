/*
 * Copyright (c) 2024 Jiri Svoboda
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
#include <inet/addr.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(addr);

/** Test inet_addr_parse() with unabbreviated address */
PCUT_TEST(inet_addr_parse_full)
{
	errno_t rc;
	inet_addr_t addr;
	char *endptr;

	rc = inet_addr_parse("1122:3344:5566:7788:99aa:bbcc:ddee:ff00/", &addr,
	    &endptr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS('/', *endptr);

	PCUT_ASSERT_INT_EQUALS(ip_v6, addr.version);

	PCUT_ASSERT_INT_EQUALS(0x11, addr.addr6[0]);
	PCUT_ASSERT_INT_EQUALS(0x22, addr.addr6[1]);
	PCUT_ASSERT_INT_EQUALS(0x33, addr.addr6[2]);
	PCUT_ASSERT_INT_EQUALS(0x44, addr.addr6[3]);
	PCUT_ASSERT_INT_EQUALS(0x55, addr.addr6[4]);
	PCUT_ASSERT_INT_EQUALS(0x66, addr.addr6[5]);
	PCUT_ASSERT_INT_EQUALS(0x77, addr.addr6[6]);
	PCUT_ASSERT_INT_EQUALS(0x88, addr.addr6[7]);

	PCUT_ASSERT_INT_EQUALS(0x99, addr.addr6[8]);
	PCUT_ASSERT_INT_EQUALS(0xaa, addr.addr6[9]);
	PCUT_ASSERT_INT_EQUALS(0xbb, addr.addr6[10]);
	PCUT_ASSERT_INT_EQUALS(0xcc, addr.addr6[11]);
	PCUT_ASSERT_INT_EQUALS(0xdd, addr.addr6[12]);
	PCUT_ASSERT_INT_EQUALS(0xee, addr.addr6[13]);
	PCUT_ASSERT_INT_EQUALS(0xff, addr.addr6[14]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[15]);
}

/** Test inet_addr_parse() with short groups (< 0x1000) */
PCUT_TEST(inet_addr_parse_shortgr)
{
	errno_t rc;
	inet_addr_t addr;
	char *endptr;

	rc = inet_addr_parse("1:22:333:4444:5:66:777:8888/", &addr,
	    &endptr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS('/', *endptr);

	PCUT_ASSERT_INT_EQUALS(ip_v6, addr.version);

	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[0]);
	PCUT_ASSERT_INT_EQUALS(0x01, addr.addr6[1]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[2]);
	PCUT_ASSERT_INT_EQUALS(0x22, addr.addr6[3]);
	PCUT_ASSERT_INT_EQUALS(0x03, addr.addr6[4]);
	PCUT_ASSERT_INT_EQUALS(0x33, addr.addr6[5]);
	PCUT_ASSERT_INT_EQUALS(0x44, addr.addr6[6]);
	PCUT_ASSERT_INT_EQUALS(0x44, addr.addr6[7]);

	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[8]);
	PCUT_ASSERT_INT_EQUALS(0x05, addr.addr6[9]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[10]);
	PCUT_ASSERT_INT_EQUALS(0x66, addr.addr6[11]);
	PCUT_ASSERT_INT_EQUALS(0x07, addr.addr6[12]);
	PCUT_ASSERT_INT_EQUALS(0x77, addr.addr6[13]);
	PCUT_ASSERT_INT_EQUALS(0x88, addr.addr6[14]);
	PCUT_ASSERT_INT_EQUALS(0x88, addr.addr6[15]);
}

/** Test inet_addr_parse() with wildcard at the beginning */
PCUT_TEST(inet_addr_parse_wcbegin)
{
	errno_t rc;
	inet_addr_t addr;
	char *endptr;

	rc = inet_addr_parse("::1234/", &addr, &endptr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS('/', *endptr);

	PCUT_ASSERT_INT_EQUALS(ip_v6, addr.version);

	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[0]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[1]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[2]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[3]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[4]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[5]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[6]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[7]);

	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[8]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[9]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[10]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[11]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[12]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[13]);
	PCUT_ASSERT_INT_EQUALS(0x12, addr.addr6[14]);
	PCUT_ASSERT_INT_EQUALS(0x34, addr.addr6[15]);
}

/** Test inet_addr_parse() with wildcard in the middle */
PCUT_TEST(inet_addr_parse_wcmiddle)
{
	errno_t rc;
	inet_addr_t addr;
	char *endptr;

	rc = inet_addr_parse("1122:3344::5566/", &addr, &endptr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS('/', *endptr);

	PCUT_ASSERT_INT_EQUALS(ip_v6, addr.version);

	PCUT_ASSERT_INT_EQUALS(0x11, addr.addr6[0]);
	PCUT_ASSERT_INT_EQUALS(0x22, addr.addr6[1]);
	PCUT_ASSERT_INT_EQUALS(0x33, addr.addr6[2]);
	PCUT_ASSERT_INT_EQUALS(0x44, addr.addr6[3]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[4]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[5]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[6]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[7]);

	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[8]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[9]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[10]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[11]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[12]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[13]);
	PCUT_ASSERT_INT_EQUALS(0x55, addr.addr6[14]);
	PCUT_ASSERT_INT_EQUALS(0x66, addr.addr6[15]);
}

/** Test inet_addr_parse() with wildcard at the end */
PCUT_TEST(inet_addr_parse_wcend)
{
	errno_t rc;
	inet_addr_t addr;
	char *endptr;

	rc = inet_addr_parse("1234:5678::/", &addr, &endptr);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_EQUALS('/', *endptr);

	PCUT_ASSERT_INT_EQUALS(ip_v6, addr.version);

	PCUT_ASSERT_INT_EQUALS(0x12, addr.addr6[0]);
	PCUT_ASSERT_INT_EQUALS(0x34, addr.addr6[1]);
	PCUT_ASSERT_INT_EQUALS(0x56, addr.addr6[2]);
	PCUT_ASSERT_INT_EQUALS(0x78, addr.addr6[3]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[4]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[5]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[6]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[7]);

	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[8]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[9]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[10]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[11]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[12]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[13]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[14]);
	PCUT_ASSERT_INT_EQUALS(0x00, addr.addr6[15]);
}

PCUT_EXPORT(addr);
