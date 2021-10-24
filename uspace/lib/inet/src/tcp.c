/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup libinet
 * @{
 */
/** @file TCP API
 */

#include <errno.h>
#include <fibril.h>
#include <inet/endpoint.h>
#include <inet/tcp.h>
#include <ipc/services.h>
#include <ipc/tcp.h>
#include <stdlib.h>

static void tcp_cb_conn(ipc_call_t *, void *);
static errno_t tcp_conn_fibril(void *);

/** Incoming TCP connection info
 *
 * Used to pass information about incoming TCP connection to the connection
 * fibril
 */
typedef struct {
	/** Listener who received the connection */
	tcp_listener_t *lst;
	/** Incoming connection */
	tcp_conn_t *conn;
} tcp_in_conn_t;

/** Create callback connection from TCP service.
 *
 * @param tcp TCP service
 * @return EOK on success or an error code
 */
static errno_t tcp_callback_create(tcp_t *tcp)
{
	async_exch_t *exch = async_exchange_begin(tcp->sess);

	aid_t req = async_send_0(exch, TCP_CALLBACK_CREATE, NULL);

	port_id_t port;
	errno_t rc = async_create_callback_port(exch, INTERFACE_TCP_CB, 0, 0,
	    tcp_cb_conn, tcp, &port);

	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

/** Create TCP client instance.
 *
 * @param  rtcp Place to store pointer to new TCP client
 * @return EOK on success, ENOMEM if out of memory, EIO if service
 *         cannot be contacted
 */
errno_t tcp_create(tcp_t **rtcp)
{
	tcp_t *tcp;
	service_id_t tcp_svcid;
	errno_t rc;

	tcp = calloc(1, sizeof(tcp_t));
	if (tcp == NULL) {
		rc = ENOMEM;
		goto error;
	}

	list_initialize(&tcp->conn);
	list_initialize(&tcp->listener);
	fibril_mutex_initialize(&tcp->lock);
	fibril_condvar_initialize(&tcp->cv);

	rc = loc_service_get_id(SERVICE_NAME_TCP, &tcp_svcid,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	tcp->sess = loc_service_connect(tcp_svcid, INTERFACE_TCP,
	    IPC_FLAG_BLOCKING);
	if (tcp->sess == NULL) {
		rc = EIO;
		goto error;
	}

	rc = tcp_callback_create(tcp);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	*rtcp = tcp;
	return EOK;
error:
	free(tcp);
	return rc;
}

/** Destroy TCP client instance.
 *
 * @param tcp TCP client
 */
void tcp_destroy(tcp_t *tcp)
{
	if (tcp == NULL)
		return;

	async_hangup(tcp->sess);

	fibril_mutex_lock(&tcp->lock);
	while (!tcp->cb_done)
		fibril_condvar_wait(&tcp->cv, &tcp->lock);
	fibril_mutex_unlock(&tcp->lock);

	free(tcp);
}

/** Create new TCP connection
 *
 * @param tcp   TCP client instance
 * @param id    Connection ID
 * @param cb    Callbacks
 * @param arg   Callback argument
 * @param rconn Place to store pointer to new connection
 *
 * @return EOK on success, ENOMEM if out of memory
 */
static errno_t tcp_conn_new(tcp_t *tcp, sysarg_t id, tcp_cb_t *cb, void *arg,
    tcp_conn_t **rconn)
{
	tcp_conn_t *conn;

	conn = calloc(1, sizeof(tcp_conn_t));
	if (conn == NULL)
		return ENOMEM;

	conn->data_avail = false;
	fibril_mutex_initialize(&conn->lock);
	fibril_condvar_initialize(&conn->cv);

	conn->tcp = tcp;
	conn->id = id;
	conn->cb = cb;
	conn->cb_arg = arg;

	list_append(&conn->ltcp, &tcp->conn);
	*rconn = conn;

	return EOK;
}

/** Create new TCP connection.
 *
 * Open a connection to the specified destination. This function returns
 * even before the connection is established (or not). When the connection
 * is established, @a cb->connected is called. If the connection fails,
 * @a cb->conn_failed is called. Alternatively, the caller can call
 * @c tcp_conn_wait_connected() to wait for connection to complete or fail.
 * Other callbacks are available to monitor the changes in connection state.
 *
 * @a epp must specify the remote address and port. Both local address and
 * port are optional. If local address is not specified, address selection
 * will take place. If local port number is not specified, a suitable
 * free dynamic port number will be allocated.
 *
 * @param tcp   TCP client
 * @param epp   Internet endpoint pair
 * @param cb    Callbacks
 * @param arg   Argument to callbacks
 * @param rconn Place to store pointer to new connection
 *
 * @return EOK on success or an error code.
 */
errno_t tcp_conn_create(tcp_t *tcp, inet_ep2_t *epp, tcp_cb_t *cb, void *arg,
    tcp_conn_t **rconn)
{
	async_exch_t *exch;
	ipc_call_t answer;
	sysarg_t conn_id;

	exch = async_exchange_begin(tcp->sess);
	aid_t req = async_send_0(exch, TCP_CONN_CREATE, &answer);
	errno_t rc = async_data_write_start(exch, (void *)epp,
	    sizeof(inet_ep2_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		errno_t rc_orig;
		async_wait_for(req, &rc_orig);
		if (rc_orig != EOK)
			rc = rc_orig;
		goto error;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		goto error;

	conn_id = ipc_get_arg1(&answer);

	rc = tcp_conn_new(tcp, conn_id, cb, arg, rconn);
	if (rc != EOK)
		return rc;

	return EOK;
error:
	return (errno_t) rc;
}

/** Destroy TCP connection.
 *
 * Destroy TCP connection. The caller should destroy all connections
 * he created before destroying the TCP client and before terminating.
 *
 * @param conn TCP connection
 */
void tcp_conn_destroy(tcp_conn_t *conn)
{
	async_exch_t *exch;

	if (conn == NULL)
		return;

	list_remove(&conn->ltcp);

	exch = async_exchange_begin(conn->tcp->sess);
	errno_t rc = async_req_1_0(exch, TCP_CONN_DESTROY, conn->id);
	async_exchange_end(exch);

	free(conn);
	(void) rc;
}

/** Get connection based on its ID.
 *
 * @param tcp   TCP client
 * @param id    Connection ID
 * @param rconn Place to store pointer to connection
 *
 * @return EOK on success, EINVAL if no connection with the given ID exists
 */
static errno_t tcp_conn_get(tcp_t *tcp, sysarg_t id, tcp_conn_t **rconn)
{
	list_foreach(tcp->conn, ltcp, tcp_conn_t, conn) {
		if (conn->id == id) {
			*rconn = conn;
			return EOK;
		}
	}

	return EINVAL;
}

/** Get the user/callback argument for a connection.
 *
 * @param conn TCP connection
 * @return User argument associated with connection
 */
void *tcp_conn_userptr(tcp_conn_t *conn)
{
	return conn->cb_arg;
}

/** Create a TCP connection listener.
 *
 * A listener listens for connections on the set of endpoints specified
 * by @a ep. Each time a new incoming connection is established,
 * @a lcb->new_conn is called (and passed @a larg). Also, the new connection
 * will have callbacks set to @a cb and argument to @a arg.
 *
 * @a ep must specify a valid port number. @a ep may specify an address
 * or link to listen on. If it does not, the listener will listen on
 * all links/addresses.
 *
 * @param tcp  TCP client
 * @param ep   Internet endpoint
 * @param lcb  Listener callbacks
 * @param larg Listener callback argument
 * @param cb   Connection callbacks for every new connection
 * @param arg  Connection argument for every new connection
 * @param rlst Place to store pointer to new listener
 *
 * @return EOK on success or an error code
 */
errno_t tcp_listener_create(tcp_t *tcp, inet_ep_t *ep, tcp_listen_cb_t *lcb,
    void *larg, tcp_cb_t *cb, void *arg, tcp_listener_t **rlst)
{
	async_exch_t *exch;
	tcp_listener_t *lst;
	ipc_call_t answer;

	lst = calloc(1, sizeof(tcp_listener_t));
	if (lst == NULL)
		return ENOMEM;

	exch = async_exchange_begin(tcp->sess);
	aid_t req = async_send_0(exch, TCP_LISTENER_CREATE, &answer);
	errno_t rc = async_data_write_start(exch, (void *)ep,
	    sizeof(inet_ep_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		errno_t rc_orig;
		async_wait_for(req, &rc_orig);
		if (rc_orig != EOK)
			rc = rc_orig;
		goto error;
	}

	async_wait_for(req, &rc);
	if (rc != EOK)
		goto error;

	lst->tcp = tcp;
	lst->id = ipc_get_arg1(&answer);
	lst->lcb = lcb;
	lst->lcb_arg = larg;
	lst->cb = cb;
	lst->cb_arg = arg;

	list_append(&lst->ltcp, &tcp->listener);
	*rlst = lst;

	return EOK;
error:
	free(lst);
	return (errno_t) rc;
}

/** Destroy TCP connection listener.
 *
 * @param lst Listener
 */
void tcp_listener_destroy(tcp_listener_t *lst)
{
	async_exch_t *exch;

	if (lst == NULL)
		return;

	list_remove(&lst->ltcp);

	exch = async_exchange_begin(lst->tcp->sess);
	errno_t rc = async_req_1_0(exch, TCP_LISTENER_DESTROY, lst->id);
	async_exchange_end(exch);

	free(lst);
	(void) rc;
}

/** Get TCP connection listener based on its ID.
 *
 * @param tcp TCP client
 * @param id  Listener ID
 * @param rlst Place to store pointer to listener
 *
 * @return EOK on success, EINVAL if no listener with the given ID is found
 */
static errno_t tcp_listener_get(tcp_t *tcp, sysarg_t id, tcp_listener_t **rlst)
{
	list_foreach(tcp->listener, ltcp, tcp_listener_t, lst) {
		if (lst->id == id) {
			*rlst = lst;
			return EOK;
		}
	}

	return EINVAL;
}

/** Get callback/user argument associated with listener.
 *
 * @param lst Listener
 * @return Callback/user argument
 */
void *tcp_listener_userptr(tcp_listener_t *lst)
{
	return lst->lcb_arg;
}

/** Wait until connection is either established or connection fails.
 *
 * Can be called after calling tcp_conn_create() to block until connection
 * either completes or fails. If the connection fails, EIO is returned.
 * In this case the connection still exists, but is in a failed
 * state.
 *
 * @param conn Connection
 * @return EOK if connection is established, EIO otherwise
 */
errno_t tcp_conn_wait_connected(tcp_conn_t *conn)
{
	fibril_mutex_lock(&conn->lock);
	while (!conn->connected && !conn->conn_failed && !conn->conn_reset)
		fibril_condvar_wait(&conn->cv, &conn->lock);

	if (conn->connected) {
		fibril_mutex_unlock(&conn->lock);
		return EOK;
	} else {
		assert(conn->conn_failed || conn->conn_reset);
		fibril_mutex_unlock(&conn->lock);
		return EIO;
	}
}

/** Send data over TCP connection.
 *
 * @param conn  Connection
 * @param data  Data
 * @param bytes Data size in bytes
 *
 * @return EOK on success or an error code
 */
errno_t tcp_conn_send(tcp_conn_t *conn, const void *data, size_t bytes)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(conn->tcp->sess);
	aid_t req = async_send_1(exch, TCP_CONN_SEND, conn->id, NULL);
	rc = async_data_write_start(exch, data, bytes);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &rc);
	return rc;
}

/** Send FIN.
 *
 * Send FIN, indicating no more data will be send over the connection.
 *
 * @param conn Connection
 * @return EOK on success or an error code
 */
errno_t tcp_conn_send_fin(tcp_conn_t *conn)
{
	async_exch_t *exch;

	exch = async_exchange_begin(conn->tcp->sess);
	errno_t rc = async_req_1_0(exch, TCP_CONN_SEND_FIN, conn->id);
	async_exchange_end(exch);

	return rc;
}

/** Push connection.
 *
 * @param conn Connection
 * @return EOK on success or an error code
 */
errno_t tcp_conn_push(tcp_conn_t *conn)
{
	async_exch_t *exch;

	exch = async_exchange_begin(conn->tcp->sess);
	errno_t rc = async_req_1_0(exch, TCP_CONN_PUSH, conn->id);
	async_exchange_end(exch);

	return rc;
}

/** Reset connection.
 *
 * @param conn Connection
 * @return EOK on success or an error code
 */
errno_t tcp_conn_reset(tcp_conn_t *conn)
{
	async_exch_t *exch;

	exch = async_exchange_begin(conn->tcp->sess);
	errno_t rc = async_req_1_0(exch, TCP_CONN_RESET, conn->id);
	async_exchange_end(exch);

	return rc;
}

/** Read received data from connection without blocking.
 *
 * If any received data is pending on the connection, up to @a bsize bytes
 * are copied to @a buf and the acutal number is stored in @a *nrecv.
 * The entire buffer of @a bsize bytes is filled except when less data
 * is currently available or FIN is received. EOK is returned.
 *
 * If no received data is pending, returns EAGAIN.
 *
 * @param conn Connection
 * @param buf  Buffer
 * @param bsize Buffer size
 * @param nrecv Place to store actual number of received bytes
 *
 * @return EOK on success, EAGAIN if no received data is pending, or other
 *         error code in case of other error
 */
errno_t tcp_conn_recv(tcp_conn_t *conn, void *buf, size_t bsize, size_t *nrecv)
{
	async_exch_t *exch;
	ipc_call_t answer;

	fibril_mutex_lock(&conn->lock);
	if (!conn->data_avail) {
		fibril_mutex_unlock(&conn->lock);
		return EAGAIN;
	}

	exch = async_exchange_begin(conn->tcp->sess);
	aid_t req = async_send_1(exch, TCP_CONN_RECV, conn->id, &answer);
	errno_t rc = async_data_read_start(exch, buf, bsize);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		fibril_mutex_unlock(&conn->lock);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	if (retval != EOK) {
		fibril_mutex_unlock(&conn->lock);
		return retval;
	}

	*nrecv = ipc_get_arg1(&answer);
	fibril_mutex_unlock(&conn->lock);
	return EOK;
}

/** Read received data from connection with blocking.
 *
 * Wait for @a bsize bytes of data to be received and copy them to
 * @a buf. Less data may be returned if FIN is received on the connection.
 * The actual If any received data is written to @a *nrecv and EOK
 * is returned on success.
 *
 * @param conn Connection
 * @param buf  Buffer
 * @param bsize Buffer size
 * @param nrecv Place to store actual number of received bytes
 *
 * @return EOK on success or an error code
 */
errno_t tcp_conn_recv_wait(tcp_conn_t *conn, void *buf, size_t bsize,
    size_t *nrecv)
{
	async_exch_t *exch;
	ipc_call_t answer;

again:
	fibril_mutex_lock(&conn->lock);
	while (!conn->data_avail) {
		fibril_condvar_wait(&conn->cv, &conn->lock);
	}

	exch = async_exchange_begin(conn->tcp->sess);
	aid_t req = async_send_1(exch, TCP_CONN_RECV_WAIT, conn->id, &answer);
	errno_t rc = async_data_read_start(exch, buf, bsize);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		if (rc == EAGAIN) {
			conn->data_avail = false;
			fibril_mutex_unlock(&conn->lock);
			goto again;
		}
		fibril_mutex_unlock(&conn->lock);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	if (retval != EOK) {
		if (rc == EAGAIN) {
			conn->data_avail = false;
		}
		fibril_mutex_unlock(&conn->lock);
		return retval;
	}

	*nrecv = ipc_get_arg1(&answer);
	fibril_mutex_unlock(&conn->lock);
	return EOK;
}

/** Connection established event.
 *
 * @param tcp   TCP client
 * @param icall Call data
 *
 */
static void tcp_ev_connected(tcp_t *tcp, ipc_call_t *icall)
{
	tcp_conn_t *conn;
	sysarg_t conn_id;
	errno_t rc;

	conn_id = ipc_get_arg1(icall);

	rc = tcp_conn_get(tcp, conn_id, &conn);
	if (rc != EOK) {
		async_answer_0(icall, ENOENT);
		return;
	}

	fibril_mutex_lock(&conn->lock);
	conn->connected = true;
	fibril_condvar_broadcast(&conn->cv);
	fibril_mutex_unlock(&conn->lock);

	async_answer_0(icall, EOK);
}

/** Connection failed event.
 *
 * @param tcp   TCP client
 * @param icall Call data
 *
 */
static void tcp_ev_conn_failed(tcp_t *tcp, ipc_call_t *icall)
{
	tcp_conn_t *conn;
	sysarg_t conn_id;
	errno_t rc;

	conn_id = ipc_get_arg1(icall);

	rc = tcp_conn_get(tcp, conn_id, &conn);
	if (rc != EOK) {
		async_answer_0(icall, ENOENT);
		return;
	}

	fibril_mutex_lock(&conn->lock);
	conn->conn_failed = true;
	fibril_condvar_broadcast(&conn->cv);
	fibril_mutex_unlock(&conn->lock);

	async_answer_0(icall, EOK);
}

/** Connection reset event.
 *
 * @param tcp   TCP client
 * @param icall Call data
 *
 */
static void tcp_ev_conn_reset(tcp_t *tcp, ipc_call_t *icall)
{
	tcp_conn_t *conn;
	sysarg_t conn_id;
	errno_t rc;

	conn_id = ipc_get_arg1(icall);

	rc = tcp_conn_get(tcp, conn_id, &conn);
	if (rc != EOK) {
		async_answer_0(icall, ENOENT);
		return;
	}

	fibril_mutex_lock(&conn->lock);
	conn->conn_reset = true;
	fibril_condvar_broadcast(&conn->cv);
	fibril_mutex_unlock(&conn->lock);

	async_answer_0(icall, EOK);
}

/** Data available event.
 *
 * @param tcp   TCP client
 * @param icall Call data
 *
 */
static void tcp_ev_data(tcp_t *tcp, ipc_call_t *icall)
{
	tcp_conn_t *conn;
	sysarg_t conn_id;
	errno_t rc;

	conn_id = ipc_get_arg1(icall);

	rc = tcp_conn_get(tcp, conn_id, &conn);
	if (rc != EOK) {
		async_answer_0(icall, ENOENT);
		return;
	}

	conn->data_avail = true;
	fibril_condvar_broadcast(&conn->cv);

	if (conn->cb != NULL && conn->cb->data_avail != NULL)
		conn->cb->data_avail(conn);

	async_answer_0(icall, EOK);
}

/** Urgent data event.
 *
 * @param tcp   TCP client
 * @param icall Call data
 *
 */
static void tcp_ev_urg_data(tcp_t *tcp, ipc_call_t *icall)
{
	async_answer_0(icall, ENOTSUP);
}

/** New connection event.
 *
 * @param tcp   TCP client
 * @param icall Call data
 *
 */
static void tcp_ev_new_conn(tcp_t *tcp, ipc_call_t *icall)
{
	tcp_listener_t *lst;
	tcp_conn_t *conn;
	sysarg_t lst_id;
	sysarg_t conn_id;
	fid_t fid;
	tcp_in_conn_t *cinfo;
	errno_t rc;

	lst_id = ipc_get_arg1(icall);
	conn_id = ipc_get_arg2(icall);

	rc = tcp_listener_get(tcp, lst_id, &lst);
	if (rc != EOK) {
		async_answer_0(icall, ENOENT);
		return;
	}

	rc = tcp_conn_new(tcp, conn_id, lst->cb, lst->cb_arg, &conn);
	if (rc != EOK) {
		async_answer_0(icall, ENOMEM);
		return;
	}

	if (lst->lcb != NULL && lst->lcb->new_conn != NULL) {
		cinfo = calloc(1, sizeof(tcp_in_conn_t));
		if (cinfo == NULL) {
			async_answer_0(icall, ENOMEM);
			return;
		}

		cinfo->lst = lst;
		cinfo->conn = conn;

		fid = fibril_create(tcp_conn_fibril, cinfo);
		if (fid == 0) {
			async_answer_0(icall, ENOMEM);
		}

		fibril_add_ready(fid);
	}

	async_answer_0(icall, EOK);
}

/** Callback connection handler.
 *
 * @param icall Connect call data
 * @param arg   Argument, TCP client
 *
 */
static void tcp_cb_conn(ipc_call_t *icall, void *arg)
{
	tcp_t *tcp = (tcp_t *)arg;

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			/* Hangup */
			async_answer_0(&call, EOK);
			goto out;
		}

		switch (ipc_get_imethod(&call)) {
		case TCP_EV_CONNECTED:
			tcp_ev_connected(tcp, &call);
			break;
		case TCP_EV_CONN_FAILED:
			tcp_ev_conn_failed(tcp, &call);
			break;
		case TCP_EV_CONN_RESET:
			tcp_ev_conn_reset(tcp, &call);
			break;
		case TCP_EV_DATA:
			tcp_ev_data(tcp, &call);
			break;
		case TCP_EV_URG_DATA:
			tcp_ev_urg_data(tcp, &call);
			break;
		case TCP_EV_NEW_CONN:
			tcp_ev_new_conn(tcp, &call);
			break;
		default:
			async_answer_0(&call, ENOTSUP);
			break;
		}
	}

out:
	fibril_mutex_lock(&tcp->lock);
	tcp->cb_done = true;
	fibril_mutex_unlock(&tcp->lock);
	fibril_condvar_broadcast(&tcp->cv);
}

/** Fibril for handling incoming TCP connection in background.
 *
 * @param arg Argument, incoming connection information (@c tcp_in_conn_t)
 */
static errno_t tcp_conn_fibril(void *arg)
{
	tcp_in_conn_t *cinfo = (tcp_in_conn_t *)arg;

	cinfo->lst->lcb->new_conn(cinfo->lst, cinfo->conn);
	tcp_conn_destroy(cinfo->conn);

	return EOK;
}

/** @}
 */
