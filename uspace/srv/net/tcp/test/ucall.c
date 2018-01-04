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

PCUT_TEST_SUITE(ucall);

static void test_cstate_change(tcp_conn_t *, void *, tcp_cstate_t);
static void test_conns_establish(tcp_conn_t **, tcp_conn_t **);
static void test_conns_tear_down(tcp_conn_t *, tcp_conn_t *);

static tcp_rqueue_cb_t test_rqueue_cb = {
	.seg_received = tcp_as_segment_arrived
};

static tcp_cb_t test_conn_cb = {
	.cstate_change = test_cstate_change
};

static tcp_conn_status_t cconn_status;
static tcp_conn_status_t sconn_status;

static FIBRIL_MUTEX_INITIALIZE(cst_lock);
static FIBRIL_CONDVAR_INITIALIZE(cst_cv);

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

/** Test creating a listening passive connection and then deleting it. */
PCUT_TEST(listen_delete)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	tcp_error_t trc;
	tcp_conn_status_t cstatus;

	inet_ep2_init(&epp);
	inet_addr(&epp.local.addr, 127, 0, 0, 1);
	epp.local.port = inet_port_user_lo;

	conn = NULL;
	trc = tcp_uc_open(&epp, ap_passive, tcp_open_nonblock, &conn);
	PCUT_ASSERT_INT_EQUALS(TCP_EOK, trc);
	PCUT_ASSERT_NOT_NULL(conn);

	tcp_uc_status(conn, &cstatus);
	PCUT_ASSERT_INT_EQUALS(st_listen, cstatus.cstate);

	trc = tcp_uc_close(conn);
	PCUT_ASSERT_INT_EQUALS(TCP_EOK, trc);
	tcp_uc_delete(conn);
}

/** Test trying to connect to endpoint that sends RST back */
PCUT_TEST(connect_rst)
{
	tcp_conn_t *conn;
	inet_ep2_t epp;
	tcp_error_t trc;

	inet_ep2_init(&epp);
	inet_addr(&epp.local.addr, 127, 0, 0, 1);
	inet_addr(&epp.remote.addr, 127, 0, 0, 1);
	epp.remote.port = inet_port_user_lo;

	conn = NULL;
	trc = tcp_uc_open(&epp, ap_active, 0, &conn);
	PCUT_ASSERT_INT_EQUALS(TCP_ERESET, trc);
	PCUT_ASSERT_NULL(conn);
}

/** Test establishing a connection */
PCUT_TEST(conn_establish)
{
	tcp_conn_t *cconn, *sconn;

	test_conns_establish(&cconn, &sconn);
	test_conns_tear_down(cconn, sconn);
}

/** Test establishing and then closing down a connection first on one side,
 * then on_the other. */
PCUT_TEST(conn_est_close_seq)
{
	tcp_conn_t *cconn, *sconn;
	tcp_error_t trc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "conn_est_close_seq: establish");
	/* Establish */
	test_conns_establish(&cconn, &sconn);

	log_msg(LOG_DEFAULT, LVL_NOTE, "conn_est_close_seq: close cconn");
	/* Close client side */
	trc = tcp_uc_close(cconn);
	PCUT_ASSERT_INT_EQUALS(TCP_EOK, trc);

	log_msg(LOG_DEFAULT, LVL_NOTE, "conn_est_close_seq: wait cconn = fin-wait-2");
	/* Wait for cconn to go to Fin-Wait-2 */
	fibril_mutex_lock(&cst_lock);
	tcp_uc_status(cconn, &cconn_status);
	while (cconn_status.cstate != st_fin_wait_2)
		fibril_condvar_wait(&cst_cv, &cst_lock);
	fibril_mutex_unlock(&cst_lock);

	log_msg(LOG_DEFAULT, LVL_NOTE, "conn_est_close_seq: wait sconn = close-wait");
	/* Wait for sconn to go to Close-Wait */
	fibril_mutex_lock(&cst_lock);
	tcp_uc_status(sconn, &sconn_status);
	while (sconn_status.cstate != st_close_wait)
		fibril_condvar_wait(&cst_cv, &cst_lock);
	fibril_mutex_unlock(&cst_lock);

	log_msg(LOG_DEFAULT, LVL_NOTE, "conn_est_close_seq: close sconn");
	/* Close server side */
	trc = tcp_uc_close(sconn);
	PCUT_ASSERT_INT_EQUALS(TCP_EOK, trc);

	log_msg(LOG_DEFAULT, LVL_NOTE, "conn_est_close_seq: wait cconn = time-wait");
	/* Wait for cconn to go to Time-Wait */
	fibril_mutex_lock(&cst_lock);
	tcp_uc_status(cconn, &cconn_status);
	while (cconn_status.cstate != st_time_wait)
		fibril_condvar_wait(&cst_cv, &cst_lock);
	fibril_mutex_unlock(&cst_lock);

	log_msg(LOG_DEFAULT, LVL_NOTE, "conn_est_close_seq: wait sconn = closed");
	/* Wait for sconn to go to Closed */
	fibril_mutex_lock(&cst_lock);
	tcp_uc_status(sconn, &sconn_status);
	while (sconn_status.cstate != st_closed) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "conn_est_close_seq: sconn.status == %d", sconn_status.cstate);
		fibril_condvar_wait(&cst_cv, &cst_lock);
	}
	fibril_mutex_unlock(&cst_lock);

	log_msg(LOG_DEFAULT, LVL_NOTE, "conn_est_close_seq: tear down");
	/* Tear down */
	test_conns_tear_down(cconn, sconn);
}

/** Test establishing and then simultaneously closing down a connection. */
PCUT_TEST(conn_est_close_simult)
{
	tcp_conn_t *cconn, *sconn;
	tcp_error_t trc;

	/* Establish */
	test_conns_establish(&cconn, &sconn);

	/* Close both sides simultaneously */
	trc = tcp_uc_close(cconn);
	PCUT_ASSERT_INT_EQUALS(TCP_EOK, trc);
	trc = tcp_uc_close(sconn);
	PCUT_ASSERT_INT_EQUALS(TCP_EOK, trc);

	/* Wait for cconn to go to Time-Wait */
	fibril_mutex_lock(&cst_lock);
	tcp_uc_status(cconn, &cconn_status);
	while (cconn_status.cstate != st_time_wait)
		fibril_condvar_wait(&cst_cv, &cst_lock);
	fibril_mutex_unlock(&cst_lock);

	/*
	 * Wait for sconn to go to Closed or Time-Wait. The connection
	 * goes to Closed if we managed to call tcp_uc_close() before
	 * sconn received FIN. Otherwise it goes to Time-Wait.
	 *
	 * XXX We may want to add delay to the loopback here to be
	 * absolutely sure that we go to Closing -> Time-Wait.
	 */
	fibril_mutex_lock(&cst_lock);
	tcp_uc_status(sconn, &sconn_status);
	while (sconn_status.cstate != st_time_wait &&
	    sconn_status.cstate != st_closed)
		fibril_condvar_wait(&cst_cv, &cst_lock);
	fibril_mutex_unlock(&cst_lock);

	/* Tear down */
	test_conns_tear_down(cconn, sconn);
}

static void test_cstate_change(tcp_conn_t *conn, void *arg,
    tcp_cstate_t old_state)
{
	tcp_conn_status_t *status = (tcp_conn_status_t *)arg;

	fibril_mutex_lock(&cst_lock);
	tcp_uc_status(conn, status);
	fibril_mutex_unlock(&cst_lock);
	fibril_condvar_broadcast(&cst_cv);
}

/** Establish client-server connection */
static void test_conns_establish(tcp_conn_t **rcconn, tcp_conn_t **rsconn)
{
	tcp_conn_t *cconn, *sconn;
	inet_ep2_t cepp, sepp;
	tcp_conn_status_t cstatus;
	tcp_error_t trc;

	/* Client EPP */
	inet_ep2_init(&cepp);
	inet_addr(&cepp.local.addr, 127, 0, 0, 1);
	inet_addr(&cepp.remote.addr, 127, 0, 0, 1);
	cepp.remote.port = inet_port_user_lo;

	/* Server EPP */
	inet_ep2_init(&sepp);
	inet_addr(&sepp.local.addr, 127, 0, 0, 1);
	sepp.local.port = inet_port_user_lo;

	/* Server side of the connection */
	sconn = NULL;
	trc = tcp_uc_open(&sepp, ap_passive, tcp_open_nonblock, &sconn);
	PCUT_ASSERT_INT_EQUALS(TCP_EOK, trc);
	PCUT_ASSERT_NOT_NULL(sconn);

	tcp_uc_set_cb(sconn, &test_conn_cb, &sconn_status);
	PCUT_ASSERT_EQUALS(&sconn_status, tcp_uc_get_userptr(sconn));

	tcp_uc_status(sconn, &cstatus);
	PCUT_ASSERT_INT_EQUALS(st_listen, cstatus.cstate);

	/* Client side of the connection */

	cconn = NULL;
	trc = tcp_uc_open(&cepp, ap_active, 0, &cconn);
	PCUT_ASSERT_INT_EQUALS(TCP_EOK, trc);
	PCUT_ASSERT_NOT_NULL(cconn);

	tcp_uc_set_cb(cconn, &test_conn_cb, &cconn_status);
	PCUT_ASSERT_EQUALS(&cconn_status, tcp_uc_get_userptr(cconn));

	/* The client side of the connection should be established by now */

	tcp_uc_status(cconn, &cstatus);
	PCUT_ASSERT_INT_EQUALS(st_established, cstatus.cstate);

	/* Need to wait for server side */

	fibril_mutex_lock(&cst_lock);
	tcp_uc_status(sconn, &sconn_status);
	while (sconn_status.cstate != st_established)
		fibril_condvar_wait(&cst_cv, &cst_lock);
	fibril_mutex_unlock(&cst_lock);

	*rcconn = cconn;
	*rsconn = sconn;
}

/* Tear down client-server connection. */
static void test_conns_tear_down(tcp_conn_t *cconn, tcp_conn_t *sconn)
{
	tcp_uc_abort(cconn);
	tcp_uc_delete(cconn);

	tcp_uc_abort(sconn);
	tcp_uc_delete(sconn);
}

PCUT_EXPORT(ucall);
