/*
 * Copyright (c) 2017 Jiri Svoboda
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

#include "../conn.h"

PCUT_INIT

PCUT_TEST_SUITE(conn);

PCUT_TEST_BEFORE
{
	int rc;

	/* We will be calling functions that perform logging */
	rc = log_init("test-tcp");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = tcp_conns_init();
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
}

PCUT_TEST_AFTER
{
	tcp_conns_fini();
}

/** Test creating and deleting connection */
PCUT_TEST(new_delete)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;

	inet_ep2_init(&epp);
	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	tcp_conn_lock(conn);
	tcp_conn_reset(conn);
	tcp_conn_unlock(conn);
	tcp_conn_delete(conn);
}

/** Test adding, finding and removing a connection */
PCUT_TEST(add_find_remove)
{
	tcp_conn_t *conn, *cfound;
	inet_ep2_t epp;
	int rc;

	inet_ep2_init(&epp);

	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	rc = tcp_conn_add(conn);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/* Find the connection */
	cfound = tcp_conn_find_ref(&conn->ident);
	PCUT_ASSERT_EQUALS(conn, cfound);
	tcp_conn_delref(cfound);

	/* We should have been assigned a port address, should not match */
	cfound = tcp_conn_find_ref(&epp);
	PCUT_ASSERT_EQUALS(NULL, cfound);

	tcp_conn_lock(conn);
	tcp_conn_reset(conn);
	tcp_conn_unlock(conn);
	tcp_conn_delete(conn);
}

PCUT_EXPORT(conn);
