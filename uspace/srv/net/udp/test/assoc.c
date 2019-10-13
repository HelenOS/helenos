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

#include <errno.h>
#include <inet/endpoint.h>
#include <io/log.h>
#include <pcut/pcut.h>
#include <str.h>

#include "../assoc.h"
#include "../msg.h"

PCUT_INIT;

PCUT_TEST_SUITE(assoc);

static void test_recv_msg(void *, inet_ep2_t *, udp_msg_t *);

static udp_assoc_cb_t test_assoc_cb = {
	.recv_msg = test_recv_msg
};

PCUT_TEST_BEFORE
{
	errno_t rc;

	/* We will be calling functions that perform logging */
	rc = log_init("test-udp");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = udp_assocs_init();
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

PCUT_TEST_AFTER
{
	udp_assocs_fini();
}

/** Test creating and deleting association */
PCUT_TEST(new_delete)
{
	udp_assoc_t *assoc;
	inet_ep2_t epp;

	inet_ep2_init(&epp);
	assoc = udp_assoc_new(&epp, &test_assoc_cb, NULL);
	PCUT_ASSERT_NOT_NULL(assoc);

	udp_assoc_delete(assoc);
}

/** Test adding, removing association */
PCUT_TEST(add_remove)
{
	udp_assoc_t *assoc;
	inet_ep2_t epp;
	errno_t rc;

	inet_ep2_init(&epp);

	assoc = udp_assoc_new(&epp, &test_assoc_cb, NULL);
	PCUT_ASSERT_NOT_NULL(assoc);

	rc = udp_assoc_add(assoc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	udp_assoc_remove(assoc);
	udp_assoc_delete(assoc);
}

/** Test adding, removing a reference to association */
PCUT_TEST(addref_delref)
{
	udp_assoc_t *assoc;
	inet_ep2_t epp;

	inet_ep2_init(&epp);

	assoc = udp_assoc_new(&epp, &test_assoc_cb, NULL);
	PCUT_ASSERT_NOT_NULL(assoc);

	udp_assoc_addref(assoc);
	udp_assoc_delref(assoc);

	udp_assoc_delete(assoc);
}

/** Test setting IP link in association */
PCUT_TEST(set_iplink)
{
	udp_assoc_t *assoc;
	inet_ep2_t epp;

	inet_ep2_init(&epp);

	assoc = udp_assoc_new(&epp, &test_assoc_cb, NULL);
	PCUT_ASSERT_NOT_NULL(assoc);

	udp_assoc_set_iplink(assoc, 42);
	PCUT_ASSERT_INT_EQUALS(42, assoc->ident.local_link);

	udp_assoc_delete(assoc);
 }

PCUT_TEST(send_recv)
{
}

/** Test udp_assoc_received() */
PCUT_TEST(received)
{
	udp_assoc_t *assoc;
	inet_ep2_t epp;
	errno_t rc;
	udp_msg_t *msg;
	const char *msgstr = "Hello";
	bool received;

	msg = udp_msg_new();
	PCUT_ASSERT_NOT_NULL(msg);
	msg->data_size = str_size(msgstr) + 1;
	msg->data = str_dup(msgstr);

	inet_ep2_init(&epp);
	inet_addr(&epp.remote.addr, 127, 0, 0, 1);
	epp.remote.port = 1;
	inet_addr(&epp.local.addr, 127, 0, 0, 1);
	epp.local.port = 2;

	assoc = udp_assoc_new(&epp, &test_assoc_cb, (void *) &received);
	PCUT_ASSERT_NOT_NULL(assoc);

	rc = udp_assoc_add(assoc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	received = false;
	udp_assoc_received(&epp, msg);
	PCUT_ASSERT_TRUE(received);

	udp_assoc_remove(assoc);
	udp_assoc_delete(assoc);
}

/** Test udp_assoc_reset() */
PCUT_TEST(reset)
{
	udp_assoc_t *assoc;
	inet_ep2_t epp;

	inet_ep2_init(&epp);
	assoc = udp_assoc_new(&epp, &test_assoc_cb, NULL);
	PCUT_ASSERT_NOT_NULL(assoc);

	udp_assoc_reset(assoc);
	udp_assoc_delete(assoc);
}

static void test_recv_msg(void *arg, inet_ep2_t *epp, udp_msg_t *msg)
{
	bool *received = (bool *) arg;

	*received = true;
}

PCUT_EXPORT(assoc);
