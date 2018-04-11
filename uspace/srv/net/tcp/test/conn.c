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

PCUT_INIT;

PCUT_TEST_SUITE(conn);

static tcp_rqueue_cb_t test_rqueue_cb = {
	.seg_received = tcp_as_segment_arrived
};

PCUT_TEST_BEFORE
{
	errno_t rc;

	/* We will be calling functions that perform logging */
	rc = log_init("test-tcp");
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = tcp_conns_init();
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	tcp_rqueue_init(&test_rqueue_cb);
	tcp_rqueue_fibril_start();

	/* Enable internal loopback */
	tcp_conn_lb = tcp_lb_segment;
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
	errno_t rc;

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
	errno_t rc;

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

/** Test establishing a connection */
PCUT_TEST(conn_establish)
{
	tcp_conn_t *cconn, *sconn;
	inet_ep2_t cepp, sepp;
	errno_t rc;

	/* Client EPP */
	inet_ep2_init(&cepp);
	inet_addr(&cepp.local.addr, 127, 0, 0, 1);
	inet_addr(&cepp.remote.addr, 127, 0, 0, 1);
	cepp.remote.port = inet_port_user_lo;

	/* Server EPP */
	inet_ep2_init(&sepp);
	inet_addr(&sepp.local.addr, 127, 0, 0, 1);
	sepp.local.port = inet_port_user_lo;

	/* Client side of the connection */
	cconn = tcp_conn_new(&cepp);
	PCUT_ASSERT_NOT_NULL(cconn);

	rc = tcp_conn_add(cconn);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(st_listen, cconn->cstate);
	PCUT_ASSERT_FALSE(tcp_conn_got_syn(cconn));

	/* Server side of the connection */
	sconn = tcp_conn_new(&sepp);
	PCUT_ASSERT_NOT_NULL(sconn);

	rc = tcp_conn_add(sconn);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	PCUT_ASSERT_INT_EQUALS(st_listen, sconn->cstate);
	PCUT_ASSERT_FALSE(tcp_conn_got_syn(sconn));

	/* Start establishing the connection */

	tcp_conn_lock(cconn);
	tcp_conn_sync(cconn);
	PCUT_ASSERT_INT_EQUALS(st_syn_sent, cconn->cstate);
	PCUT_ASSERT_FALSE(tcp_conn_got_syn(cconn));

	/* Wait for client-side state to transition */
	while (cconn->cstate == st_syn_sent)
		fibril_condvar_wait(&cconn->cstate_cv, &cconn->lock);

	PCUT_ASSERT_INT_EQUALS(st_established, cconn->cstate);
	PCUT_ASSERT_TRUE(tcp_conn_got_syn(cconn));
	tcp_conn_unlock(cconn);

	/* Wait for server-side state to transition */
	tcp_conn_lock(sconn);
	while (sconn->cstate == st_listen || sconn->cstate == st_syn_received)
		fibril_condvar_wait(&sconn->cstate_cv, &sconn->lock);

	PCUT_ASSERT_INT_EQUALS(st_established, sconn->cstate);
	PCUT_ASSERT_TRUE(tcp_conn_got_syn(sconn));

	/* Verify counters */
	PCUT_ASSERT_EQUALS(cconn->iss + 1, cconn->snd_nxt);
	PCUT_ASSERT_EQUALS(cconn->iss + 1, cconn->snd_una);
	PCUT_ASSERT_EQUALS(sconn->iss + 1, sconn->snd_nxt);
	PCUT_ASSERT_EQUALS(sconn->iss + 1, sconn->snd_una);

	tcp_conn_unlock(sconn);

	tcp_conn_lock(cconn);
	tcp_conn_reset(cconn);
	tcp_conn_unlock(cconn);
	tcp_conn_delete(cconn);

	tcp_conn_lock(sconn);
	tcp_conn_reset(sconn);
	tcp_conn_unlock(sconn);
	tcp_conn_delete(sconn);
}

PCUT_TEST(ep2_flipped)
{
	inet_ep2_t a, fa;

	inet_addr(&a.local.addr, 1, 2, 3, 4);
	a.local.port = 1234;
	inet_addr(&a.remote.addr, 5, 6, 7, 8);
	a.remote.port = 5678;

	tcp_ep2_flipped(&a, &fa);

	PCUT_ASSERT_INT_EQUALS(a.local.port, fa.remote.port);
	PCUT_ASSERT_INT_EQUALS(a.remote.port, fa.local.port);

	PCUT_ASSERT_TRUE(inet_addr_compare(&a.local.addr, &fa.remote.addr));
	PCUT_ASSERT_TRUE(inet_addr_compare(&a.remote.addr, &fa.local.addr));
}

PCUT_EXPORT(conn);
