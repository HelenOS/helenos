/*
 * Copyright (c) 2019 Jiri Svoboda
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

#include <inet/endpoint.h>
#include <pcut/pcut.h>
#include <str.h>
#include "../msg.h"
#include "../pdu.h"

PCUT_INIT;

PCUT_TEST_SUITE(pdu);

/** Test creating and deleting dummy PDU */
PCUT_TEST(new_delete)
{
	udp_pdu_t *pdu;

	pdu = udp_pdu_new();
	udp_pdu_delete(pdu);
}

/** Test encoding and decoding back IPv4 PDU */
PCUT_TEST(encode_decode_v4)
{
	inet_ep2_t epp;
	udp_msg_t *msg;
	inet_ep2_t depp;
	udp_msg_t *dmsg;
	udp_pdu_t *pdu;
	const char *msgstr = "Hello";
	errno_t rc;

	inet_ep2_init(&epp);
	inet_addr(&epp.local.addr, 192, 168, 0, 1);
	epp.local.port = 1;
	inet_addr(&epp.remote.addr, 192, 168, 0, 2);
	epp.remote.port = 2;

	msg = udp_msg_new();
	PCUT_ASSERT_NOT_NULL(msg);
	msg->data_size = str_size(msgstr) + 1;
	msg->data = str_dup(msgstr);

	rc = udp_pdu_encode(&epp, msg, &pdu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	rc = udp_pdu_decode(pdu, &depp, &dmsg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Compare epp and depp. Local and remote should be swapped. */
	PCUT_ASSERT_TRUE(inet_addr_compare(&epp.local.addr, &depp.remote.addr));
	PCUT_ASSERT_EQUALS(epp.local.port, depp.remote.port);
	PCUT_ASSERT_TRUE(inet_addr_compare(&epp.remote.addr, &depp.local.addr));
	PCUT_ASSERT_EQUALS(epp.remote.port, depp.local.port);

	/* Compare msg and dmsg */
	PCUT_ASSERT_EQUALS(msg->data_size, dmsg->data_size);
	PCUT_ASSERT_TRUE(memcmp(msg->data, dmsg->data, dmsg->data_size) == 0);

	udp_pdu_delete(pdu);
	udp_msg_delete(msg);
	udp_msg_delete(dmsg);
}

PCUT_TEST(encode_decode_v6)
{
	inet_ep2_t epp;
	udp_msg_t *msg;
	inet_ep2_t depp;
	udp_msg_t *dmsg;
	udp_pdu_t *pdu;
	const char *msgstr = "Hello";
	errno_t rc;

	inet_ep2_init(&epp);
	inet_addr6(&epp.local.addr, 0xfc00, 0, 0, 0, 0, 0, 0, 1);
	epp.local.port = 1;
	inet_addr6(&epp.remote.addr, 0xfc00, 0, 0, 0, 0, 0, 0, 2);
	epp.remote.port = 2;

	msg = udp_msg_new();
	PCUT_ASSERT_NOT_NULL(msg);
	msg->data_size = str_size(msgstr) + 1;
	msg->data = str_dup(msgstr);

	rc = udp_pdu_encode(&epp, msg, &pdu);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	rc = udp_pdu_decode(pdu, &depp, &dmsg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Compare epp and depp. Local and remote should be swapped. */
	PCUT_ASSERT_TRUE(inet_addr_compare(&epp.local.addr, &depp.remote.addr));
	PCUT_ASSERT_EQUALS(epp.local.port, depp.remote.port);
	PCUT_ASSERT_TRUE(inet_addr_compare(&epp.remote.addr, &depp.local.addr));
	PCUT_ASSERT_EQUALS(epp.remote.port, depp.local.port);

	/* Compare msg and dmsg */
	PCUT_ASSERT_EQUALS(msg->data_size, dmsg->data_size);
	PCUT_ASSERT_TRUE(memcmp(msg->data, dmsg->data, dmsg->data_size) == 0);

	udp_pdu_delete(pdu);
	udp_msg_delete(msg);
	udp_msg_delete(dmsg);
}

PCUT_EXPORT(pdu);
