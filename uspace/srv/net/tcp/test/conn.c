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
#include "../rqueue.h"
#include "../ucall.h"

PCUT_INIT

PCUT_TEST_SUITE(conn);

static tcp_rqueue_cb_t test_rqueue_cb = {
	.seg_received = tcp_as_segment_arrived
};

PCUT_TEST_BEFORE
{
	int rc;

	/* We will be calling functions that perform logging */
	rc = log_init("test-tcp");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = tcp_conns_init();
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tcp_rqueue_init(&test_rqueue_cb);
	tcp_rqueue_fibril_start();
}

PCUT_TEST_AFTER
{
	tcp_rqueue_fini();
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

/** Test adding, finding and deleting a connection */
PCUT_TEST(add_find_delete)
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

/** Test trying to connect to endpoint that sends RST back */
PCUT_TEST(connect_rst)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	int rc;

	tcp_conn_lb = tcp_lb_segment;

	inet_ep2_init(&epp);
	inet_addr(&epp.local.addr, 127, 0, 0, 1);
	inet_addr(&epp.remote.addr, 127, 0, 0, 1);
	epp.remote.port = inet_port_user_lo;

	conn = tcp_conn_new(&epp);
	PCUT_ASSERT_NOT_NULL(conn);

	rc = tcp_conn_add(conn);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(st_listen, conn->cstate);

	tcp_conn_lock(conn);
	tcp_conn_sync(conn);
	PCUT_ASSERT_INT_EQUALS(st_syn_sent, conn->cstate);

	while (conn->cstate == st_syn_sent)
                fibril_condvar_wait(&conn->cstate_cv, &conn->lock);

	PCUT_ASSERT_INT_EQUALS(st_closed, conn->cstate);

	tcp_conn_unlock(conn);
	tcp_conn_delete(conn);
}

PCUT_EXPORT(conn);
