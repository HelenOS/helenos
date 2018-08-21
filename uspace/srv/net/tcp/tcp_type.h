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

/** @addtogroup tcp
 * @{
 */
/** @file TCP type definitions
 */

#ifndef TCP_TYPE_H
#define TCP_TYPE_H

#include <adt/list.h>
#include <async.h>
#include <stdbool.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <refcount.h>
#include <stddef.h>
#include <stdint.h>
#include <inet/addr.h>
#include <inet/endpoint.h>

struct tcp_conn;

/** Connection state */
typedef enum {
	/** Listen */
	st_listen,
	/** Syn-sent */
	st_syn_sent,
	/** Syn-received */
	st_syn_received,
	/** Established */
	st_established,
	/** Fin-wait-1 */
	st_fin_wait_1,
	/** Fin-wait-2 */
	st_fin_wait_2,
	/** Close-wait */
	st_close_wait,
	/** Closing */
	st_closing,
	/** Last-ack */
	st_last_ack,
	/** Time-wait */
	st_time_wait,
	/** Closed */
	st_closed
} tcp_cstate_t;

/** Error codes returned by TCP user calls (per the spec). */
typedef enum {
	/* OK */
	TCP_EOK,
	/* Connection aborted due to user timeout */
	TCP_EABORTED,
	/* Connection already exists */
	TCP_EEXISTS,
	/* Connection closing */
	TCP_ECLOSING,
	/* Connection does not exist */
	TCP_ENOTEXIST,
	/* Connection illegal for this process */
	TCP_EILLEGAL,
	/* Connection not open */
	TCP_ENOTOPEN,
	/* Connection reset */
	TCP_ERESET,
	/* Remote endpoint unspecified */
	TCP_EUNSPEC,
	/* Insufficient resources */
	TCP_ENORES,
	/* Precedence not allowed */
	TCP_EINVPREC,
	/* Security/compartment not allowed */
	TCP_EINVCOMP,
	TCP_EAGAIN
} tcp_error_t;

/** Transfer flags */
typedef enum {
	XF_PUSH		= 0x1,
	XF_URGENT	= 0x2
} xflags_t;

/** Control message bits
 *
 * Note this is not the actual on-the-wire encoding
 */
typedef enum {
	CTL_SYN		= 0x1,
	CTL_FIN		= 0x2,
	CTL_RST		= 0x4,
	CTL_ACK		= 0x8
} tcp_control_t;

/** Connection incoming segments queue */
typedef struct {
	struct tcp_conn *conn;
	list_t list;
} tcp_iqueue_t;

/** Active or passive connection */
typedef enum {
	ap_active,
	ap_passive
} acpass_t;

/** Flags for TCP open operation */
typedef enum {
	tcp_open_nonblock = 1
} tcp_open_flags_t;

typedef struct tcp_conn tcp_conn_t;

/** Connection state change callback function */
typedef void (*tcp_cstate_cb_t)(tcp_conn_t *, void *);

/** Connection callbacks */
typedef struct {
	void (*cstate_change)(tcp_conn_t *, void *, tcp_cstate_t);
	void (*recv_data)(tcp_conn_t *, void *);
} tcp_cb_t;

/** Data returned by Status user call */
typedef struct {
	/** Connection state */
	tcp_cstate_t cstate;
} tcp_conn_status_t;

typedef struct {
	/** SYN, FIN */
	tcp_control_t ctrl;

	/** Segment sequence number */
	uint32_t seq;
	/** Segment acknowledgement number */
	uint32_t ack;
	/** Segment length in sequence space */
	uint32_t len;
	/** Segment window */
	uint32_t wnd;
	/** Segment urgent pointer */
	uint32_t up;

	/** Segment data, may be moved when trimming segment */
	void *data;
	/** Segment data, original pointer used to free data */
	void *dfptr;
} tcp_segment_t;

/** Receive queue entry */
typedef struct {
	link_t link;
	inet_ep2_t epp;
	tcp_segment_t *seg;
} tcp_rqueue_entry_t;

/** Receive queue callbacks */
typedef struct {
	/** Segment received */
	void (*seg_received)(inet_ep2_t *, tcp_segment_t *);
} tcp_rqueue_cb_t;

/** NCSim queue entry */
typedef struct {
	link_t link;
	usec_t delay;
	inet_ep2_t epp;
	tcp_segment_t *seg;
} tcp_squeue_entry_t;

/** Incoming queue entry */
typedef struct {
	link_t link;
	tcp_segment_t *seg;
} tcp_iqueue_entry_t;

/** Retransmission queue entry */
typedef struct {
	link_t link;
	tcp_conn_t *conn;
	tcp_segment_t *seg;
} tcp_tqueue_entry_t;

/** Retransmission queue callbacks */
typedef struct {
	/** Segment received */
	void (*transmit_seg)(inet_ep2_t *, tcp_segment_t *);
} tcp_tqueue_cb_t;

/** Retransmission queue */
typedef struct {
	struct tcp_conn *conn;
	list_t list;

	/** Retransmission timer */
	fibril_timer_t *timer;

	/** Callbacks */
	tcp_tqueue_cb_t *cb;
} tcp_tqueue_t;

/** Connection */
struct tcp_conn {
	char *name;
	link_t link;

	/** Connection callbacks function */
	tcp_cb_t *cb;
	/** Argument to @c cstate_cb */
	void *cb_arg;

	/** Connection identification (local and remote endpoint) */
	inet_ep2_t ident;
	/** Connection is in association map */
	bool mapped;

	/** Active or passive connection */
	acpass_t ap;

	/** Protects access to connection structure */
	fibril_mutex_t lock;
	/** Reference count */
	atomic_refcount_t refcnt;

	/** Connection state */
	tcp_cstate_t cstate;
	/** True if connection was reset */
	bool reset;
	/** True if connection was deleted by user */
	bool deleted;
	/** Signalled when @c cstate changes */
	fibril_condvar_t cstate_cv;

	/** Set when FIN is removed from the retransmission queue */
	bool fin_is_acked;

	/** Queue of incoming segments */
	tcp_iqueue_t incoming;

	/** Retransmission queue */
	tcp_tqueue_t retransmit;

	/** Time-Wait timeout timer */
	fibril_timer_t *tw_timer;

	/** Receive buffer */
	uint8_t *rcv_buf;
	/** Receive buffer size */
	size_t rcv_buf_size;
	/** Receive buffer number of bytes used */
	size_t rcv_buf_used;
	/** Receive buffer contains FIN */
	bool rcv_buf_fin;
	/** Receive buffer CV. Broadcast when new data is inserted */
	fibril_condvar_t rcv_buf_cv;

	/** Send buffer */
	uint8_t *snd_buf;
	/** Send buffer size */
	size_t snd_buf_size;
	/** Send buffer number of bytes used */
	size_t snd_buf_used;
	/** Send buffer contains FIN */
	bool snd_buf_fin;
	/** Send buffer CV. Broadcast when space is made available in buffer */
	fibril_condvar_t snd_buf_cv;

	/** Send unacknowledged */
	uint32_t snd_una;
	/** Send next */
	uint32_t snd_nxt;
	/** Send window */
	uint32_t snd_wnd;
	/** Send urgent pointer */
	uint32_t snd_up;
	/** Segment sequence number used for last window update */
	uint32_t snd_wl1;
	/** Segment acknowledgement number used for last window update */
	uint32_t snd_wl2;
	/** Initial send sequence number */
	uint32_t iss;

	/** Receive next */
	uint32_t rcv_nxt;
	/** Receive window */
	uint32_t rcv_wnd;
	/** Receive urgent pointer */
	uint32_t rcv_up;
	/** Initial receive sequence number */
	uint32_t irs;
};

/** Continuation of processing.
 *
 * When processing incoming segment, are we done or should we continue
 * processing it?
 */
typedef enum {
	cp_continue,
	cp_done
} cproc_t;

/** Encoded PDU */
typedef struct {
	/** Source address */
	inet_addr_t src;
	/** Destination address */
	inet_addr_t dest;
	/** Encoded header */
	void *header;
	/** Encoded header size */
	size_t header_size;
	/** Text */
	void *text;
	/** Text size */
	size_t text_size;
} tcp_pdu_t;

/** TCP client connection */
typedef struct tcp_cconn {
	/** Connection */
	tcp_conn_t *conn;
	/** Connection ID for the client */
	sysarg_t id;
	/** Client */
	struct tcp_client *client;
	link_t lclient;
} tcp_cconn_t;

/** TCP client listener */
typedef struct tcp_clst {
	/** Local endpoint */
	inet_ep_t elocal;
	/** Connection */
	tcp_conn_t *conn;
	/** Listener ID for the client */
	sysarg_t id;
	/** Client */
	struct tcp_client *client;
	/** Link to tcp_client_t.clst */
	link_t lclient;
} tcp_clst_t;

/** TCP client */
typedef struct tcp_client {
	/** Client callback session */
	async_sess_t *sess;
	/** Client's connections */
	list_t cconn; /* of tcp_cconn_t */
	/** Client's listeners */
	list_t clst;
} tcp_client_t;

/** Internal loopback type */
typedef enum {
	/** No loopback */
	tcp_lb_none,
	/** Segment loopback */
	tcp_lb_segment,
	/** PDU loopback */
	tcp_lb_pdu
} tcp_lb_t;

#endif

/** @}
 */
