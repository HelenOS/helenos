/*
 * Copyright (c) 2011 Jiri Svoboda
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

/**
 * @file
 */

#include <byteorder.h>
#include <io/log.h>
#include "header.h"
#include "rqueue.h"
#include "segment.h"
#include "tqueue.h"
#include "tcp_type.h"

void tcp_tqueue_ctrl_seg(tcp_conn_t *conn, tcp_control_t ctrl)
{
	tcp_segment_t *seg;

	log_msg(LVL_DEBUG, "tcp_tqueue_ctrl_seg(%p, %u)", conn, ctrl);

	seg = tcp_segment_make_ctrl(ctrl);
	seg->seq = conn->snd_nxt;
	seg->ack = conn->rcv_nxt;
	tcp_tqueue_seg(conn, seg);
}

void tcp_tqueue_seg(tcp_conn_t *conn, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_tqueue_seg(%p, %p)", conn, seg);
	/* XXX queue */

	conn->snd_nxt += seg->len;
	tcp_transmit_segment(&conn->ident, seg);
}

/** Remove ACKed segments from retransmission queue.
 *
 * This should be called when SND.UNA is updated due to incoming ACK.
 */
void tcp_tqueue_remove_acked(tcp_conn_t *conn)
{
	(void) conn;
}

void tcp_transmit_segment(tcp_sockpair_t *sp, tcp_segment_t *seg)
{
	log_msg(LVL_DEBUG, "tcp_transmit_segment(%p, %p)", sp, seg);
/*
	tcp_pdu_prepare(conn, seg, &data, &len);
	tcp_pdu_transmit(data, len);
*/
	tcp_rqueue_bounce_seg(sp, seg);
}

/**
 * @}
 */
