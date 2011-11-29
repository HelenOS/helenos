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
 * @file TCP (Transmission Control Protocol) network module
 */

#include <async.h>
#include <byteorder.h>
#include <errno.h>
#include <io/log.h>
#include <stdio.h>
#include <task.h>

#include <icmp_remote.h>
#include <ip_client.h>
#include <ip_interface.h>
#include <ipc/services.h>
#include <ipc/tl.h>
#include <tl_common.h>
#include <tl_skel.h>
#include <packet_client.h>
#include <packet_remote.h>

#include "ncsim.h"
#include "pdu.h"
#include "rqueue.h"
#include "sock.h"
#include "std.h"
#include "tcp.h"
#include "test.h"

#define NAME       "tcp"

async_sess_t *net_sess;
static async_sess_t *icmp_sess;
static async_sess_t *ip_sess;
packet_dimensions_t pkt_dims;

static void tcp_received_pdu(tcp_pdu_t *pdu);

/* Pull up packets into a single memory block. */
static int pq_pullup(packet_t *packet, void **data, size_t *dsize)
{
	packet_t *npacket;
	size_t tot_len;
	int length;

	npacket = packet;
	tot_len = 0;
	do {
		length = packet_get_data_length(packet);
		if (length <= 0)
			return EINVAL;

		tot_len += length;
	} while ((npacket = pq_next(npacket)) != NULL);

	uint8_t *buf;
	uint8_t *dp;

	buf = calloc(tot_len, 1);
	if (buf == NULL) {
		free(buf);
		return ENOMEM;
	}

	npacket = packet;
	dp = buf;
	do {
		length = packet_get_data_length(packet);
		if (length <= 0) {
			free(buf);
			return EINVAL;
		}

		memcpy(dp, packet_get_data(packet), length);
		dp += length;
	} while ((npacket = pq_next(npacket)) != NULL);

	*data = buf;
	*dsize = tot_len;
	return EOK;
}

/** Process packet received from network layer. */
static int tcp_received_msg(device_id_t device_id, packet_t *packet,
    services_t error)
{
	int rc;
	size_t offset;
	int length;
	struct sockaddr_in *src_addr;
	struct sockaddr_in *dest_addr;
	size_t addr_len;

	log_msg(LVL_DEBUG, "tcp_received_msg()");

	switch (error) {
	case SERVICE_NONE:
		break;
	case SERVICE_ICMP:
	default:
		log_msg(LVL_WARN, "Unsupported service number %u",
		    (unsigned)error);
		pq_release_remote(net_sess, packet_get_id(packet));
		return ENOTSUP;
	}

	/* Process and trim off IP header */
	log_msg(LVL_DEBUG, "tcp_received_msg() - IP header");

	rc = ip_client_process_packet(packet, NULL, NULL, NULL, NULL, NULL);
	if (rc < 0) {
		log_msg(LVL_WARN, "ip_client_process_packet() failed");
		pq_release_remote(net_sess, packet_get_id(packet));
		return rc;
	}

	offset = (size_t)rc;
	length = packet_get_data_length(packet);

	if (length < 0 || (size_t)length < offset) {
		log_msg(LVL_WARN, "length=%d, dropping.", length);
		pq_release_remote(net_sess, packet_get_id(packet));
		return EINVAL;
	}

	addr_len = packet_get_addr(packet, (uint8_t **)&src_addr,
	    (uint8_t **)&dest_addr);
	if (addr_len <= 0) {
		log_msg(LVL_WARN, "Failed to get packet address.");
		pq_release_remote(net_sess, packet_get_id(packet));
		return EINVAL;
	}

	if (addr_len != sizeof(struct sockaddr_in)) {
		log_msg(LVL_WARN, "Unsupported address size %zu (!= %zu)",
		    addr_len, sizeof(struct sockaddr_in));
		pq_release_remote(net_sess, packet_get_id(packet));
		return EINVAL;
	}

	rc = packet_trim(packet, offset, 0);
	if (rc != EOK) {
		log_msg(LVL_WARN, "Failed to trim packet.");
		pq_release_remote(net_sess, packet_get_id(packet));
		return rc;
	}

	/* Pull up packets into a single memory block, pdu_raw. */
	log_msg(LVL_DEBUG, "tcp_received_msg() - pull up");
	uint8_t *pdu_raw;
	size_t pdu_raw_size = 0;

	pq_pullup(packet, (void **)&pdu_raw, &pdu_raw_size);

	/* Split into header and payload. */

	log_msg(LVL_DEBUG, "tcp_received_msg() - split header/payload");

	tcp_pdu_t *pdu;
	size_t hdr_size;

	/* XXX Header options */
	hdr_size = sizeof(tcp_header_t);

	if (pdu_raw_size < hdr_size) {
		log_msg(LVL_WARN, "pdu_raw_size = %zu < hdr_size = %zu",
		    pdu_raw_size, hdr_size);
		pq_release_remote(net_sess, packet_get_id(packet));
		return EINVAL;
	}

	log_msg(LVL_DEBUG, "pdu_raw_size=%zu, hdr_size=%zu",
	    pdu_raw_size, hdr_size);
	pdu = tcp_pdu_create(pdu_raw, hdr_size, pdu_raw + hdr_size,
	    pdu_raw_size - hdr_size);
	if (pdu == NULL) {
		log_msg(LVL_WARN, "Failed creating PDU. Dropped.");
		return ENOMEM;
	}

	free(pdu_raw);

	pdu->src_addr.ipv4 = uint32_t_be2host(src_addr->sin_addr.s_addr);
	pdu->dest_addr.ipv4 = uint32_t_be2host(dest_addr->sin_addr.s_addr);
	log_msg(LVL_DEBUG, "src: 0x%08x, dest: 0x%08x",
	    pdu->src_addr.ipv4, pdu->dest_addr.ipv4);

	tcp_received_pdu(pdu);
	tcp_pdu_delete(pdu);

	return EOK;
}

/** Receive packets from network layer. */
static void tcp_receiver(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	packet_t *packet;
	int rc;

	log_msg(LVL_DEBUG, "tcp_receiver()");

	while (true) {
		switch (IPC_GET_IMETHOD(*icall)) {
		case NET_TL_RECEIVED:
			log_msg(LVL_DEBUG, "method = NET_TL_RECEIVED");
			rc = packet_translate_remote(net_sess, &packet,
			    IPC_GET_PACKET(*icall));
			if (rc != EOK) {
				log_msg(LVL_DEBUG, "Error %d translating packet.", rc);
				async_answer_0(iid, (sysarg_t)rc);
				break;
			}
			rc = tcp_received_msg(IPC_GET_DEVICE(*icall), packet,
			    IPC_GET_ERROR(*icall));
			async_answer_0(iid, (sysarg_t)rc);
			break;
		default:
			log_msg(LVL_DEBUG, "method = %u",
			    (unsigned)IPC_GET_IMETHOD(*icall));
			async_answer_0(iid, ENOTSUP);
			break;
		}

		iid = async_get_call(icall);
	}
}

/** Transmit PDU over network layer. */
void tcp_transmit_pdu(tcp_pdu_t *pdu)
{
	struct sockaddr_in dest;
	device_id_t dev_id;
	void *phdr;
	size_t phdr_len;
	packet_dimension_t *pkt_dim;
	int rc;
	packet_t *packet;
	void *pkt_data;
	size_t pdu_size;

	dest.sin_family = AF_INET;
	dest.sin_port = 0; /* not needed */
	dest.sin_addr.s_addr = host2uint32_t_be(pdu->dest_addr.ipv4);

	/* Find route. Obtained pseudo-header is not used. */
	rc = ip_get_route_req(ip_sess, IPPROTO_TCP, (struct sockaddr *)&dest,
	    sizeof(dest), &dev_id, &phdr, &phdr_len);
	if (rc != EOK) {
		log_msg(LVL_DEBUG, "tcp_transmit_pdu: Failed to find route.");
		return;
	}

	rc = tl_get_ip_packet_dimension(ip_sess, &pkt_dims, dev_id, &pkt_dim);
	if (rc != EOK) {
		log_msg(LVL_DEBUG, "tcp_transmit_pdu: Failed to get dimension.");
		return;
	}

	pdu_size = pdu->header_size + pdu->text_size;

	packet = packet_get_4_remote(net_sess, pdu_size, pkt_dim->addr_len,
	    pkt_dim->prefix, pkt_dim->suffix);
	if (!packet) {
		log_msg(LVL_DEBUG, "tcp_transmit_pdu: Failed to get packet.");
		return;
	}

	pkt_data = packet_suffix(packet, pdu_size);
	if (!pkt_data) {
		log_msg(LVL_DEBUG, "tcp_transmit_pdu: Failed to get pkt_data ptr.");
		pq_release_remote(net_sess, packet_get_id(packet));
		return;
	}

	rc = ip_client_prepare_packet(packet, IPPROTO_TCP, 0, 0, 0, 0);
	if (rc != EOK) {
		log_msg(LVL_DEBUG, "tcp_transmit_pdu: Failed to prepare IP packet part.");
		pq_release_remote(net_sess, packet_get_id(packet));
		return;
	}

	rc = packet_set_addr(packet, NULL, (uint8_t *)&dest, sizeof(dest));
	if (rc != EOK) {
		log_msg(LVL_DEBUG, "tcp_transmit_pdu: Failed to set packet address.");
		pq_release_remote(net_sess, packet_get_id(packet));
		return;
	}

	/* Copy PDU data to packet */
	memcpy(pkt_data, pdu->header, pdu->header_size);
	memcpy((uint8_t *)pkt_data + pdu->header_size, pdu->text,
	    pdu->text_size);

	/* Transmit packet. XXX Transfers packet ownership to IP? */
	ip_send_msg(ip_sess, dev_id, packet, SERVICE_TCP, 0);
}

/** Process received PDU. */
static void tcp_received_pdu(tcp_pdu_t *pdu)
{
	tcp_segment_t *dseg;
	tcp_sockpair_t rident;

	log_msg(LVL_DEBUG, "tcp_received_pdu()");

	if (tcp_pdu_decode(pdu, &rident, &dseg) != EOK) {
		log_msg(LVL_WARN, "Not enough memory. PDU dropped.");
		return;
	}

	/* Insert decoded segment into rqueue */
	tcp_rqueue_insert_seg(&rident, dseg);
}

/* Called from libnet */
void tl_connection(void)
{
	log_msg(LVL_DEBUG, "tl_connection()");
}

/* Called from libnet */
int tl_message(ipc_callid_t callid, ipc_call_t *call, ipc_call_t *answer,
    size_t *answer_count)
{
	async_sess_t *callback;

	log_msg(LVL_DEBUG, "tl_message()");

	*answer_count = 0;
	callback = async_callback_receive_start(EXCHANGE_SERIALIZE, call);
	if (callback)
		return tcp_sock_connection(callback, callid, *call);

	return ENOTSUP;
}

/* Called from libnet */
int tl_initialize(async_sess_t *sess)
{
	int rc;

	net_sess = sess;
	icmp_sess = icmp_connect_module();

	log_msg(LVL_DEBUG, "tl_initialize()");

	tcp_sock_init();

	ip_sess = ip_bind_service(SERVICE_IP, IPPROTO_TCP, SERVICE_TCP,
	    tcp_receiver);
	if (ip_sess == NULL)
		return ENOENT;

	rc = packet_dimensions_initialize(&pkt_dims);
	if (rc != EOK)
		return rc;

	return EOK;
}

int main(int argc, char **argv)
{
	int rc;

	printf(NAME ": TCP (Transmission Control Protocol) network module\n");

	rc = log_init(NAME, LVL_DEBUG);
	if (rc != EOK) {
		printf(NAME ": Failed to initialize log.\n");
		return 1;
	}

//	printf(NAME ": Accepting connections\n");
//	task_retval(0);

	tcp_rqueue_init();
	tcp_rqueue_thread_start();

	tcp_ncsim_init();
	tcp_ncsim_thread_start();

	tcp_test();
/*
	async_manager();
*/
	tl_module_start(SERVICE_TCP);

	/* Not reached */
	return 0;
}

/**
 * @}
 */
