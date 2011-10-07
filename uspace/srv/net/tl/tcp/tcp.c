/*
 * Copyright (c) 2008 Lukas Mejdrech
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

/** @file
 * TCP module implementation.
 * @see tcp.h
 */

#include <assert.h>
#include <async.h>
#include <fibril_synch.h>
#include <malloc.h>
/* TODO remove stdio */
#include <stdio.h>
#include <errno.h>

#include <ipc/services.h>
#include <ipc/net.h>
#include <ipc/tl.h>
#include <ipc/socket.h>

#include <net/socket_codes.h>
#include <net/ip_protocols.h>
#include <net/in.h>
#include <net/in6.h>
#include <net/inet.h>
#include <net/modules.h>

#include <adt/dynamic_fifo.h>
#include <packet_client.h>
#include <packet_remote.h>
#include <net_checksum.h>
#include <ip_client.h>
#include <ip_interface.h>
#include <icmp_client.h>
#include <icmp_remote.h>
#include <net_interface.h>
#include <socket_core.h>
#include <tl_common.h>
#include <tl_remote.h>
#include <tl_skel.h>

#include "tcp.h"
#include "tcp_header.h"

/** TCP module name. */
#define NAME  "tcp"

/** The TCP window default value. */
#define NET_DEFAULT_TCP_WINDOW		10240

/** Initial timeout for new connections. */
#define NET_DEFAULT_TCP_INITIAL_TIMEOUT	3000000L

/** Default timeout for closing. */
#define NET_DEFAULT_TCP_TIME_WAIT_TIMEOUT 2000L

/** The initial outgoing sequence number. */
#define TCP_INITIAL_SEQUENCE_NUMBER	2999

/** Maximum TCP fragment size. */
#define MAX_TCP_FRAGMENT_SIZE		65535

/** Free ports pool start. */
#define TCP_FREE_PORTS_START		1025

/** Free ports pool end. */
#define TCP_FREE_PORTS_END		65535

/** Timeout for connection initialization, SYN sent. */
#define TCP_SYN_SENT_TIMEOUT		1000000L

/** The maximum number of timeouts in a row before singaling connection lost. */
#define TCP_MAX_TIMEOUTS		8

/** The number of acknowledgements before retransmit. */
#define TCP_FAST_RETRANSMIT_COUNT	3

/** Returns a value indicating whether the value is in the interval respecting
 * the possible overflow.
 *
 * The high end and/or the value may overflow, be lower than the low value.
 *
 * @param[in] lower	The last value before the interval.
 * @param[in] value	The value to be checked.
 * @param[in] higher_equal The last value in the interval.
 */
#define IS_IN_INTERVAL_OVERFLOW(lower, value, higher_equal) \
	((((lower) < (value)) && (((value) <= (higher_equal)) || \
	((higher_equal) < (lower)))) || (((value) <= (higher_equal)) && \
	((higher_equal) < (lower))))

/** Type definition of the TCP timeout.
 *  @see tcp_timeout
 */
typedef struct tcp_timeout tcp_timeout_t;

/** TCP reply timeout data.
 *  Used as a timeouting fibril argument.
 *  @see tcp_timeout()
 */
struct tcp_timeout {
	/** TCP global data are going to be read only. */
	int globals_read_only;

	/** Socket port. */
	int port;

	/** Local sockets. */
	socket_cores_t *local_sockets;

	/** Socket identifier. */
	int socket_id;

	/** Socket state. */
	tcp_socket_state_t state;

	/** Sent packet sequence number. */
	int sequence_number;

	/** Timeout in microseconds. */
	suseconds_t timeout;

	/** Port map key. */
	uint8_t *key;

	/** Port map key length. */
	size_t key_length;
};

static int tcp_release_and_return(packet_t *, int);
static void tcp_prepare_operation_header(socket_core_t *, tcp_socket_data_t *,
    tcp_header_t *, int synchronize, int);
static int tcp_prepare_timeout(int (*)(void *), socket_core_t *,
    tcp_socket_data_t *, size_t, tcp_socket_state_t, suseconds_t, int);
static void tcp_free_socket_data(socket_core_t *);

static int tcp_timeout(void *);

static int tcp_release_after_timeout(void *);

static int tcp_process_packet(nic_device_id_t, packet_t *, services_t);
static int tcp_connect_core(socket_core_t *, socket_cores_t *,
    struct sockaddr *, socklen_t);
static int tcp_queue_prepare_packet(socket_core_t *, tcp_socket_data_t *,
    packet_t *, size_t);
static int tcp_queue_packet(socket_core_t *, tcp_socket_data_t *, packet_t *,
    size_t);
static packet_t *tcp_get_packets_to_send(socket_core_t *, tcp_socket_data_t *);
static void tcp_send_packets(nic_device_id_t, packet_t *);

static void tcp_process_acknowledgement(socket_core_t *, tcp_socket_data_t *,
    tcp_header_t *);
static packet_t *tcp_send_prepare_packet(socket_core_t *, tcp_socket_data_t *,
    packet_t *, size_t, size_t);
static packet_t *tcp_prepare_copy(socket_core_t *, tcp_socket_data_t *,
    packet_t *, size_t, size_t);
/* static */ void tcp_retransmit_packet(socket_core_t *, tcp_socket_data_t *,
    size_t);
static int tcp_create_notification_packet(packet_t **, socket_core_t *,
    tcp_socket_data_t *, int, int);
static void tcp_refresh_socket_data(tcp_socket_data_t *);

static void tcp_initialize_socket_data(tcp_socket_data_t *);

static int tcp_process_listen(socket_core_t *, tcp_socket_data_t *,
    tcp_header_t *, packet_t *, struct sockaddr *, struct sockaddr *, size_t);
static int tcp_process_syn_sent(socket_core_t *, tcp_socket_data_t *,
    tcp_header_t *, packet_t *);
static int tcp_process_syn_received(socket_core_t *, tcp_socket_data_t *,
    tcp_header_t *, packet_t *);
static int tcp_process_established(socket_core_t *, tcp_socket_data_t *,
    tcp_header_t *, packet_t *, int, size_t);
static int tcp_queue_received_packet(socket_core_t *, tcp_socket_data_t *,
    packet_t *, int, size_t);
static void tcp_queue_received_end_of_data(socket_core_t *socket);

static int tcp_received_msg(nic_device_id_t, packet_t *, services_t, services_t);
static int tcp_process_client_messages(async_sess_t *, ipc_callid_t,
    ipc_call_t);

static int tcp_listen_message(socket_cores_t *, int, int);
static int tcp_connect_message(socket_cores_t *, int, struct sockaddr *,
    socklen_t);
static int tcp_recvfrom_message(socket_cores_t *, int, int, size_t *);
static int tcp_send_message(socket_cores_t *, int, int, size_t *, int);
static int tcp_accept_message(socket_cores_t *, int, int, size_t *, size_t *);
static int tcp_close_message(socket_cores_t *, int);

/** TCP global data. */
tcp_globals_t tcp_globals;

int tcp_received_msg(nic_device_id_t device_id, packet_t *packet,
    services_t receiver, services_t error)
{
	int rc;

	if (receiver != SERVICE_TCP)
		return EREFUSED;

	fibril_rwlock_write_lock(&tcp_globals.lock);
	rc = tcp_process_packet(device_id, packet, error);
	if (rc != EOK)
		fibril_rwlock_write_unlock(&tcp_globals.lock);

	printf("receive %d \n", rc);

	return rc;
}

int tcp_process_packet(nic_device_id_t device_id, packet_t *packet, services_t error)
{
	size_t length;
	size_t offset;
	int result;
	tcp_header_t *header;
	socket_core_t *socket;
	tcp_socket_data_t *socket_data;
	packet_t *next_packet;
	size_t total_length;
	uint32_t checksum;
	int fragments;
	icmp_type_t type;
	icmp_code_t code;
	struct sockaddr *src;
	struct sockaddr *dest;
	size_t addrlen;
	int rc;

	switch (error) {
	case SERVICE_NONE:
		break;
	case SERVICE_ICMP:
		/* Process error */
		result = icmp_client_process_packet(packet, &type, &code, NULL,
		    NULL);
		if (result < 0)
			return tcp_release_and_return(packet, result);

		length = (size_t) result;
		rc = packet_trim(packet, length, 0);
		if (rc != EOK)
			return tcp_release_and_return(packet, rc);
		break;
	default:
		return tcp_release_and_return(packet, ENOTSUP);
	}

	/* TODO process received ipopts? */
	result = ip_client_process_packet(packet, NULL, NULL, NULL, NULL, NULL);
	if (result < 0)
		return tcp_release_and_return(packet, result);

	offset = (size_t) result;

	length = packet_get_data_length(packet);
	if (length <= 0)
		return tcp_release_and_return(packet, EINVAL);

	if (length < TCP_HEADER_SIZE + offset)
		return tcp_release_and_return(packet, NO_DATA);

	/* Trim all but TCP header */
	rc = packet_trim(packet, offset, 0);
	if (rc != EOK)
		return tcp_release_and_return(packet, rc);

	/* Get tcp header */
	header = (tcp_header_t *) packet_get_data(packet);
	if (!header)
		return tcp_release_and_return(packet, NO_DATA);

#if 0
	printf("header len %d, port %d \n", TCP_HEADER_LENGTH(header),
	    ntohs(header->destination_port));
#endif
	result = packet_get_addr(packet, (uint8_t **) &src, (uint8_t **) &dest);
	if (result <= 0)
		return tcp_release_and_return(packet, result);

	addrlen = (size_t) result;

	rc = tl_set_address_port(src, addrlen, ntohs(header->source_port));
	if (rc != EOK)
		return tcp_release_and_return(packet, rc);
	
	/* Find the destination socket */
	socket = socket_port_find(&tcp_globals.sockets,
	    ntohs(header->destination_port), (uint8_t *) src, addrlen);
	if (!socket) {
		/* Find the listening destination socket */
		socket = socket_port_find(&tcp_globals.sockets,
		    ntohs(header->destination_port),
		    (uint8_t *) SOCKET_MAP_KEY_LISTENING, 0);
	}

	if (!socket) {
		if (tl_prepare_icmp_packet(tcp_globals.net_sess,
		    tcp_globals.icmp_sess, packet, error) == EOK) {
			icmp_destination_unreachable_msg(tcp_globals.icmp_sess,
			    ICMP_PORT_UNREACH, 0, packet);
		}
		return EADDRNOTAVAIL;
	}

	printf("socket id %d\n", socket->socket_id);
	socket_data = (tcp_socket_data_t *) socket->specific_data;
	assert(socket_data);

	/* Some data received, clear the timeout counter */
	socket_data->timeout_count = 0;

	/* Count the received packet fragments */
	next_packet = packet;
	fragments = 0;
	checksum = 0;
	total_length = 0;
	do {
		fragments++;
		length = packet_get_data_length(next_packet);
		if (length <= 0)
			return tcp_release_and_return(packet, NO_DATA);

		total_length += length;

		/* Add partial checksum if set */
		if (!error) {
			checksum = compute_checksum(checksum,
			    packet_get_data(packet),
			    packet_get_data_length(packet));
		}

	} while ((next_packet = pq_next(next_packet)));

	fibril_rwlock_write_lock(socket_data->local_lock);

	if (error)
		goto has_error_service;
	
	if (socket_data->state == TCP_SOCKET_LISTEN) {
		if (socket_data->pseudo_header) {
			free(socket_data->pseudo_header);
			socket_data->pseudo_header = NULL;
			socket_data->headerlen = 0;
		}

		rc = ip_client_get_pseudo_header(IPPROTO_TCP, src, addrlen,
		    dest, addrlen, total_length, &socket_data->pseudo_header,
		    &socket_data->headerlen);
		if (rc != EOK) {
			fibril_rwlock_write_unlock(socket_data->local_lock);
			return tcp_release_and_return(packet, rc);
		}
	} else {
		rc = ip_client_set_pseudo_header_data_length(
		    socket_data->pseudo_header, socket_data->headerlen,
		    total_length);
		if (rc != EOK) {
			fibril_rwlock_write_unlock(socket_data->local_lock);
			return tcp_release_and_return(packet, rc);
		}
	}
	
	checksum = compute_checksum(checksum, socket_data->pseudo_header,
	    socket_data->headerlen);
	if (flip_checksum(compact_checksum(checksum)) != IP_CHECKSUM_ZERO) {
		printf("checksum err %x -> %x\n", header->checksum,
		    flip_checksum(compact_checksum(checksum)));
		fibril_rwlock_write_unlock(socket_data->local_lock);

		rc = tl_prepare_icmp_packet(tcp_globals.net_sess,
		    tcp_globals.icmp_sess, packet, error);
		if (rc == EOK) {
			/* Checksum error ICMP */
			icmp_parameter_problem_msg(tcp_globals.icmp_sess,
			    ICMP_PARAM_POINTER,
			    ((size_t) ((void *) &header->checksum)) -
			    ((size_t) ((void *) header)), packet);
		}

		return EINVAL;
	}

has_error_service:
	fibril_rwlock_write_unlock(&tcp_globals.lock);

	/* TODO error reporting/handling */
	switch (socket_data->state) {
	case TCP_SOCKET_LISTEN:
		rc = tcp_process_listen(socket, socket_data, header, packet,
		    src, dest, addrlen);
		break;
	case TCP_SOCKET_SYN_RECEIVED:
		rc = tcp_process_syn_received(socket, socket_data, header,
		    packet);
		break;
	case TCP_SOCKET_SYN_SENT:
		rc = tcp_process_syn_sent(socket, socket_data, header, packet);
		break;
	case TCP_SOCKET_FIN_WAIT_1:
		/* ack changing the state to FIN_WAIT_2 gets processed later */
	case TCP_SOCKET_FIN_WAIT_2:
		/* fin changing state to LAST_ACK gets processed later */
	case TCP_SOCKET_LAST_ACK:
		/* ack releasing the socket get processed later */
	case TCP_SOCKET_CLOSING:
		/* ack releasing the socket gets processed later */
	case TCP_SOCKET_ESTABLISHED:
		rc = tcp_process_established(socket, socket_data, header,
		    packet, fragments, total_length);
		break;
	default:
		pq_release_remote(tcp_globals.net_sess, packet_get_id(packet));
	}

	if (rc != EOK) {
		fibril_rwlock_write_unlock(socket_data->local_lock);
		printf("process %d\n", rc);
	}

	return EOK;
}

int tcp_process_established(socket_core_t *socket, tcp_socket_data_t *
    socket_data, tcp_header_t *header, packet_t *packet, int fragments,
    size_t total_length)
{
	packet_t *next_packet;
	packet_t *tmp_packet;
	uint32_t old_incoming;
	size_t order;
	uint32_t sequence_number;
	size_t length;
	size_t offset;
	uint32_t new_sequence_number;
	bool forced_ack;
	int rc;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(header);
	assert(packet);

	forced_ack = false;

	new_sequence_number = ntohl(header->sequence_number);
	old_incoming = socket_data->next_incoming;

	if (GET_TCP_HEADER_FINALIZE(header)) {
		socket_data->fin_incoming = new_sequence_number +
		    total_length - TCP_HEADER_LENGTH(header);
	}

	/* Trim begining if containing expected data */
	if (IS_IN_INTERVAL_OVERFLOW(new_sequence_number,
	    socket_data->next_incoming, new_sequence_number + total_length)) {

		/* Get the acknowledged offset */
		if (socket_data->next_incoming < new_sequence_number) {
			offset = new_sequence_number -
			    socket_data->next_incoming;
		} else {
			offset = socket_data->next_incoming -
			    new_sequence_number;
		}

		new_sequence_number += offset;
		total_length -= offset;
		length = packet_get_data_length(packet);

		/* Trim the acknowledged data */
		while (length <= offset) {
			/* Release the acknowledged packets */
			next_packet = pq_next(packet);
			pq_release_remote(tcp_globals.net_sess,
			    packet_get_id(packet));
			packet = next_packet;
			offset -= length;
			length = packet_get_data_length(packet);
		}

		if (offset > 0) {
			rc = packet_trim(packet, offset, 0);
			if (rc != EOK)
				return tcp_release_and_return(packet, rc);
		}

		assert(new_sequence_number == socket_data->next_incoming);
	}

	/* Release if overflowing the window */
/*
	if (IS_IN_INTERVAL_OVERFLOW(socket_data->next_incoming +
	    socket_data->window, new_sequence_number, new_sequence_number +
	    total_length)) {
		return tcp_release_and_return(packet, EOVERFLOW);
	}

	// trim end if overflowing the window
	if (IS_IN_INTERVAL_OVERFLOW(new_sequence_number,
	    socket_data->next_incoming + socket_data->window,
	    new_sequence_number + total_length)) {
		// get the allowed data length
		if (socket_data->next_incoming + socket_data->window <
		    new_sequence_number) {
			offset = new_sequence_number -
			    socket_data->next_incoming + socket_data->window;
		} else {
			offset = socket_data->next_incoming +
			    socket_data->window - new_sequence_number;
		}
		next_packet = packet;
		// trim the overflowing data
		while (next_packet && (offset > 0)) {
			length = packet_get_data_length(packet);
			if (length <= offset)
				next_packet = pq_next(next_packet);
			else {
				rc = packet_trim(next_packet, 0,
				    length - offset));
				if (rc != EOK)
					return tcp_release_and_return(packet,
					    rc);
			}
			offset -= length;
			total_length -= length - offset;
		}
		// release the overflowing packets
		next_packet = pq_next(next_packet);
		if (next_packet) {
			tmp_packet = next_packet;
			next_packet = pq_next(next_packet);
			pq_insert_after(tmp_packet, next_packet);
			pq_release_remote(tcp_globals.net_sess,
			    packet_get_id(tmp_packet));
		}
		assert(new_sequence_number + total_length ==
		    socket_data->next_incoming + socket_data->window);
	}
*/
	/* The expected one arrived? */
	if (new_sequence_number == socket_data->next_incoming) {
		printf("expected\n");
		/* Process acknowledgement */
		tcp_process_acknowledgement(socket, socket_data, header);

		/* Remove the header */
		total_length -= TCP_HEADER_LENGTH(header);
		rc = packet_trim(packet, TCP_HEADER_LENGTH(header), 0);
		if (rc != EOK)
			return tcp_release_and_return(packet, rc);

		if (total_length) {
			rc = tcp_queue_received_packet(socket, socket_data,
			    packet, fragments, total_length);
			if (rc != EOK)
				return rc;
		} else {
			total_length = 1;
		}

		socket_data->next_incoming = old_incoming + total_length;
		packet = socket_data->incoming;
		while (packet) {
			rc = pq_get_order(socket_data->incoming, &order, NULL);
			if (rc != EOK) {
				/* Remove the corrupted packet */
				next_packet = pq_detach(packet);
				if (packet == socket_data->incoming)
					socket_data->incoming = next_packet;
				pq_release_remote(tcp_globals.net_sess,
				    packet_get_id(packet));
				packet = next_packet;
				continue;
			}

			sequence_number = (uint32_t) order;
			if (IS_IN_INTERVAL_OVERFLOW(sequence_number,
			    old_incoming, socket_data->next_incoming)) {
				/* Move to the next */
				packet = pq_next(packet);
				/* Coninual data? */
			} else if (IS_IN_INTERVAL_OVERFLOW(old_incoming,
			    sequence_number, socket_data->next_incoming)) {
				/* Detach the packet */
				next_packet = pq_detach(packet);
				if (packet == socket_data->incoming)
					socket_data->incoming = next_packet;
				/* Get data length */
				length = packet_get_data_length(packet);
				new_sequence_number = sequence_number + length;
				if (length <= 0) {
					/* Remove the empty packet */
					pq_release_remote(tcp_globals.net_sess,
					    packet_get_id(packet));
					packet = next_packet;
					continue;
				}
				/* Exactly following */
				if (sequence_number ==
				    socket_data->next_incoming) {
					/* Queue received data */
					rc = tcp_queue_received_packet(socket,
					    socket_data, packet, 1,
					    packet_get_data_length(packet));
					if (rc != EOK)
						return rc;
					socket_data->next_incoming =
					    new_sequence_number;
					packet = next_packet;
					continue;
					/* At least partly following data? */
				}
				if (IS_IN_INTERVAL_OVERFLOW(sequence_number,
				    socket_data->next_incoming, new_sequence_number)) {
					if (socket_data->next_incoming <
					    new_sequence_number) {
						length = new_sequence_number -
						    socket_data->next_incoming;
					} else {
						length =
						    socket_data->next_incoming -
						    new_sequence_number;
					}
					rc = packet_trim(packet,length, 0);
					if (rc == EOK) {
						/* Queue received data */
						rc = tcp_queue_received_packet(
						    socket, socket_data, packet,
						    1, packet_get_data_length(
						    packet));
						if (rc != EOK)
							return rc;
						socket_data->next_incoming =
						    new_sequence_number;
						packet = next_packet;
						continue;
					}
				}
				/* Remove the duplicit or corrupted packet */
				pq_release_remote(tcp_globals.net_sess,
				    packet_get_id(packet));
				packet = next_packet;
				continue;
			} else {
				break;
			}
		}
	} else if (IS_IN_INTERVAL(socket_data->next_incoming,
	    new_sequence_number,
	    socket_data->next_incoming + socket_data->window)) {
		printf("in window\n");
		/* Process acknowledgement */
		tcp_process_acknowledgement(socket, socket_data, header);

		/* Remove the header */
		total_length -= TCP_HEADER_LENGTH(header);
		rc = packet_trim(packet, TCP_HEADER_LENGTH(header), 0);
		if (rc != EOK)
			return tcp_release_and_return(packet, rc);

		next_packet = pq_detach(packet);
		length = packet_get_data_length(packet);
		rc = pq_add(&socket_data->incoming, packet, new_sequence_number,
		    length);
		if (rc != EOK) {
			/* Remove the corrupted packets */
			pq_release_remote(tcp_globals.net_sess,
			    packet_get_id(packet));
			pq_release_remote(tcp_globals.net_sess,
			    packet_get_id(next_packet));
		} else {
			while (next_packet) {
				new_sequence_number += length;
				tmp_packet = pq_detach(next_packet);
				length = packet_get_data_length(next_packet);

				rc = pq_set_order(next_packet,
				    new_sequence_number, length);
				if (rc != EOK) {
					pq_release_remote(tcp_globals.net_sess,
					    packet_get_id(next_packet));
				}
				rc = pq_insert_after(packet, next_packet);
				if (rc != EOK) {
					pq_release_remote(tcp_globals.net_sess,
					    packet_get_id(next_packet));
				}
				next_packet = tmp_packet;
			}
		}
	} else {
		printf("unexpected\n");
		/* Release duplicite or restricted */
		pq_release_remote(tcp_globals.net_sess, packet_get_id(packet));
		forced_ack = true;
	}

	/* If next in sequence is an incoming FIN */
	if (socket_data->next_incoming == socket_data->fin_incoming) {
		/* Advance sequence number */
		socket_data->next_incoming += 1;

		/* Handle FIN */
		switch (socket_data->state) {
		case TCP_SOCKET_FIN_WAIT_1:
		case TCP_SOCKET_FIN_WAIT_2:
		case TCP_SOCKET_CLOSING:
			socket_data->state = TCP_SOCKET_CLOSING;
			break;
		case TCP_SOCKET_ESTABLISHED:
			/* Queue end-of-data marker on the socket. */
			tcp_queue_received_end_of_data(socket);
			socket_data->state = TCP_SOCKET_CLOSE_WAIT;
			break;
		default:
			socket_data->state = TCP_SOCKET_CLOSE_WAIT;
			break;
		}
	}

	packet = tcp_get_packets_to_send(socket, socket_data);
	if (!packet && (socket_data->next_incoming != old_incoming || forced_ack)) {
		/* Create the notification packet */
		rc = tcp_create_notification_packet(&packet, socket,
		    socket_data, 0, 0);
		if (rc != EOK)
			return rc;
		rc = tcp_queue_prepare_packet(socket, socket_data, packet, 1);
		if (rc != EOK)
			return rc;
		packet = tcp_send_prepare_packet(socket, socket_data, packet, 1,
		    socket_data->last_outgoing + 1);
	}

	fibril_rwlock_write_unlock(socket_data->local_lock);

	/* Send the packet */
	tcp_send_packets(socket_data->device_id, packet);

	return EOK;
}

int tcp_queue_received_packet(socket_core_t *socket,
    tcp_socket_data_t *socket_data, packet_t *packet, int fragments,
    size_t total_length)
{
	packet_dimension_t *packet_dimension;
	int rc;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(packet);
	assert(fragments >= 1);
	assert(socket_data->window > total_length);

	/* Queue the received packet */
	rc = dyn_fifo_push(&socket->received, packet_get_id(packet),
	    SOCKET_MAX_RECEIVED_SIZE);
	if (rc != EOK)
		return tcp_release_and_return(packet, rc);
	rc = tl_get_ip_packet_dimension(tcp_globals.ip_sess,
	    &tcp_globals.dimensions, socket_data->device_id, &packet_dimension);
	if (rc != EOK)
		return tcp_release_and_return(packet, rc);

	/* Decrease the window size */
	socket_data->window -= total_length;

	/* Notify the destination socket */
	async_exch_t *exch = async_exchange_begin(socket->sess);
	async_msg_5(exch, NET_SOCKET_RECEIVED, (sysarg_t) socket->socket_id,
	    ((packet_dimension->content < socket_data->data_fragment_size) ?
	    packet_dimension->content : socket_data->data_fragment_size), 0, 0,
	    (sysarg_t) fragments);
	async_exchange_end(exch);

	return EOK;
}

/** Queue end-of-data marker on the socket.
 *
 * Next element in the sequence space is FIN. Queue end-of-data marker
 * on the socket.
 *
 * @param socket	Socket
 */
static void tcp_queue_received_end_of_data(socket_core_t *socket)
{
	assert(socket != NULL);

	/* Notify the destination socket */
	async_exch_t *exch = async_exchange_begin(socket->sess);
	async_msg_5(exch, NET_SOCKET_RECEIVED, (sysarg_t) socket->socket_id,
	    0, 0, 0, (sysarg_t) 0 /* 0 fragments == no more data */);
	async_exchange_end(exch);
}

int tcp_process_syn_sent(socket_core_t *socket, tcp_socket_data_t *
    socket_data, tcp_header_t *header, packet_t *packet)
{
	packet_t *next_packet;
	int rc;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(header);
	assert(packet);

	if (!GET_TCP_HEADER_SYNCHRONIZE(header))
		return tcp_release_and_return(packet, EINVAL);
	
	/* Process acknowledgement */
	tcp_process_acknowledgement(socket, socket_data, header);

	socket_data->next_incoming = ntohl(header->sequence_number) + 1;

	/* Release additional packets */
	next_packet = pq_detach(packet);
	if (next_packet) {
		pq_release_remote(tcp_globals.net_sess,
		    packet_get_id(next_packet));
	}

	/* Trim if longer than the header */
	if (packet_get_data_length(packet) > sizeof(*header)) {
		rc = packet_trim(packet, 0,
		    packet_get_data_length(packet) - sizeof(*header));
		if (rc != EOK)
			return tcp_release_and_return(packet, rc);
	}

	tcp_prepare_operation_header(socket, socket_data, header, 0, 0);
	fibril_mutex_lock(&socket_data->operation.mutex);
	socket_data->operation.result = tcp_queue_packet(socket, socket_data,
	    packet, 1);

	if (socket_data->operation.result == EOK) {
		socket_data->state = TCP_SOCKET_ESTABLISHED;
		packet = tcp_get_packets_to_send(socket, socket_data);
		if (packet) {
			fibril_rwlock_write_unlock( socket_data->local_lock);
			/* Send the packet */
			tcp_send_packets(socket_data->device_id, packet);
			/* Signal the result */
			fibril_condvar_signal( &socket_data->operation.condvar);
			fibril_mutex_unlock( &socket_data->operation.mutex);
			return EOK;
		}
	}

	fibril_mutex_unlock(&socket_data->operation.mutex);
	return tcp_release_and_return(packet, EINVAL);
}

int tcp_process_listen(socket_core_t *listening_socket,
    tcp_socket_data_t *listening_socket_data, tcp_header_t *header,
    packet_t *packet, struct sockaddr *src, struct sockaddr *dest,
    size_t addrlen)
{
	packet_t *next_packet;
	socket_core_t *socket;
	tcp_socket_data_t *socket_data;
	int socket_id;
	int listening_socket_id = listening_socket->socket_id;
	int listening_port = listening_socket->port;
	int rc;

	assert(listening_socket);
	assert(listening_socket_data);
	assert(listening_socket->specific_data == listening_socket_data);
	assert(header);
	assert(packet);

	if (!GET_TCP_HEADER_SYNCHRONIZE(header))
		return tcp_release_and_return(packet, EINVAL);

	socket_data = (tcp_socket_data_t *) malloc(sizeof(*socket_data));
	if (!socket_data)
		return tcp_release_and_return(packet, ENOMEM);

	tcp_initialize_socket_data(socket_data);
	socket_data->local_lock = listening_socket_data->local_lock;
	socket_data->local_sockets = listening_socket_data->local_sockets;
	socket_data->listening_socket_id = listening_socket->socket_id;
	socket_data->next_incoming = ntohl(header->sequence_number);
	socket_data->treshold = socket_data->next_incoming +
	    ntohs(header->window);
	socket_data->addrlen = addrlen;
	socket_data->addr = malloc(socket_data->addrlen);
	if (!socket_data->addr) {
		free(socket_data);
		return tcp_release_and_return(packet, ENOMEM);
	}

	memcpy(socket_data->addr, src, socket_data->addrlen);
	socket_data->dest_port = ntohs(header->source_port);
	rc = tl_set_address_port(socket_data->addr, socket_data->addrlen,
	    socket_data->dest_port);
	if (rc != EOK) {
		free(socket_data->addr);
		free(socket_data);
		return tcp_release_and_return(packet, rc);
	}

	/* Create a socket */
	socket_id = -1;
	rc = socket_create(socket_data->local_sockets, listening_socket->sess,
	    socket_data, &socket_id);
	if (rc != EOK) {
		free(socket_data->addr);
		free(socket_data);
		return tcp_release_and_return(packet, rc);
	}

	printf("new_sock %d\n", socket_id);
	socket_data->pseudo_header = listening_socket_data->pseudo_header;
	socket_data->headerlen = listening_socket_data->headerlen;
	listening_socket_data->pseudo_header = NULL;
	listening_socket_data->headerlen = 0;

	fibril_rwlock_write_unlock(socket_data->local_lock);
	fibril_rwlock_write_lock(&tcp_globals.lock);

	/* Find the destination socket */
	listening_socket = socket_port_find(&tcp_globals.sockets,
	    listening_port, (uint8_t *) SOCKET_MAP_KEY_LISTENING, 0);
	if (!listening_socket ||
	    (listening_socket->socket_id != listening_socket_id)) {
		fibril_rwlock_write_unlock(&tcp_globals.lock);
		/* A shadow may remain until app hangs up */
		return tcp_release_and_return(packet, EOK /*ENOTSOCK*/);
	}
	listening_socket_data =
	    (tcp_socket_data_t *) listening_socket->specific_data;
	assert(listening_socket_data);

	fibril_rwlock_write_lock(listening_socket_data->local_lock);

	socket = socket_cores_find(listening_socket_data->local_sockets,
	    socket_id);
	if (!socket) {
		/* Where is the socket?!? */
		fibril_rwlock_write_unlock(&tcp_globals.lock);
		return ENOTSOCK;
	}
	socket_data = (tcp_socket_data_t *) socket->specific_data;
	assert(socket_data);

	rc = socket_port_add(&tcp_globals.sockets, listening_port, socket,
	    (uint8_t *) socket_data->addr, socket_data->addrlen);
	assert(socket == socket_port_find(&tcp_globals.sockets, listening_port,
	    (uint8_t *) socket_data->addr, socket_data->addrlen));

//	rc = socket_bind_free_port(&tcp_globals.sockets, socket,
//	    TCP_FREE_PORTS_START, TCP_FREE_PORTS_END,
//	    tcp_globals.last_used_port);
//	tcp_globals.last_used_port = socket->port;
	fibril_rwlock_write_unlock(&tcp_globals.lock);
	if (rc != EOK) {
		socket_destroy(tcp_globals.net_sess, socket->socket_id,
		    socket_data->local_sockets, &tcp_globals.sockets,
		    tcp_free_socket_data);
		return tcp_release_and_return(packet, rc);
	}

	socket_data->state = TCP_SOCKET_LISTEN;
	socket_data->next_incoming = ntohl(header->sequence_number) + 1;

	/* Release additional packets */
	next_packet = pq_detach(packet);
	if (next_packet) {
		pq_release_remote(tcp_globals.net_sess,
		    packet_get_id(next_packet));
	}

	/* Trim if longer than the header */
	if (packet_get_data_length(packet) > sizeof(*header)) {
		rc = packet_trim(packet, 0,
		    packet_get_data_length(packet) - sizeof(*header));
		if (rc != EOK) {
			socket_destroy(tcp_globals.net_sess, socket->socket_id,
			    socket_data->local_sockets, &tcp_globals.sockets,
			    tcp_free_socket_data);
			return tcp_release_and_return(packet, rc);
		}
	}

	tcp_prepare_operation_header(socket, socket_data, header, 1, 0);

	rc = tcp_queue_packet(socket, socket_data, packet, 1);
	if (rc != EOK) {
		socket_destroy(tcp_globals.net_sess, socket->socket_id,
		    socket_data->local_sockets, &tcp_globals.sockets,
		    tcp_free_socket_data);
		return rc;
	}

	packet = tcp_get_packets_to_send(socket, socket_data);
	if (!packet) {
		socket_destroy(tcp_globals.net_sess, socket->socket_id,
		    socket_data->local_sockets, &tcp_globals.sockets,
		    tcp_free_socket_data);
		return EINVAL;
	}

	socket_data->state = TCP_SOCKET_SYN_RECEIVED;
	fibril_rwlock_write_unlock(socket_data->local_lock);

	/* Send the packet */
	tcp_send_packets(socket_data->device_id, packet);

	return EOK;
}

int tcp_process_syn_received(socket_core_t *socket,
    tcp_socket_data_t *socket_data, tcp_header_t *header, packet_t *packet)
{
	socket_core_t *listening_socket;
	tcp_socket_data_t *listening_socket_data;
	int rc;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(header);
	assert(packet);

	if (!GET_TCP_HEADER_ACKNOWLEDGE(header))
		return tcp_release_and_return(packet, EINVAL);

	/* Process acknowledgement */
	tcp_process_acknowledgement(socket, socket_data, header);

	socket_data->next_incoming = ntohl(header->sequence_number); /* + 1; */
	pq_release_remote(tcp_globals.net_sess, packet_get_id(packet));
	socket_data->state = TCP_SOCKET_ESTABLISHED;
	listening_socket = socket_cores_find(socket_data->local_sockets,
	    socket_data->listening_socket_id);
	if (listening_socket) {
		listening_socket_data =
		    (tcp_socket_data_t *) listening_socket->specific_data;
		assert(listening_socket_data);

		/* Queue the received packet */
		rc = dyn_fifo_push(&listening_socket->accepted,
		    (-1 * socket->socket_id), listening_socket_data->backlog);
		if (rc == EOK) {
			/* Notify the destination socket */
			async_exch_t *exch = async_exchange_begin(socket->sess);
			async_msg_5(exch, NET_SOCKET_ACCEPTED,
			    (sysarg_t) listening_socket->socket_id,
			    socket_data->data_fragment_size, TCP_HEADER_SIZE,
			    0, (sysarg_t) socket->socket_id);
			async_exchange_end(exch);

			fibril_rwlock_write_unlock(socket_data->local_lock);
			return EOK;
		}
	}
	/* Send FIN */
	socket_data->state = TCP_SOCKET_FIN_WAIT_1;

	/* Create the notification packet */
	rc = tcp_create_notification_packet(&packet, socket, socket_data, 0, 1);
	if (rc != EOK)
		return rc;

	/* Send the packet */
	rc = tcp_queue_packet(socket, socket_data, packet, 1);
	if (rc != EOK)
		return rc;

	/* Flush packets */
	packet = tcp_get_packets_to_send(socket, socket_data);
	fibril_rwlock_write_unlock(socket_data->local_lock);
	if (packet) {
		/* Send the packet */
		tcp_send_packets(socket_data->device_id, packet);
	}

	return EOK;
}

void tcp_process_acknowledgement(socket_core_t *socket,
    tcp_socket_data_t *socket_data, tcp_header_t *header)
{
	size_t number;
	size_t length;
	packet_t *packet;
	packet_t *next;
	packet_t *acknowledged = NULL;
	uint32_t old;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(header);

	if (!GET_TCP_HEADER_ACKNOWLEDGE(header))
		return;

	number = ntohl(header->acknowledgement_number);

	/* If more data acknowledged */
	if (number != socket_data->expected) {
		old = socket_data->expected;
		if (IS_IN_INTERVAL_OVERFLOW(old, socket_data->fin_outgoing,
		    number)) {
			switch (socket_data->state) {
			case TCP_SOCKET_FIN_WAIT_1:
				socket_data->state = TCP_SOCKET_FIN_WAIT_2;
				break;
			case TCP_SOCKET_LAST_ACK:
			case TCP_SOCKET_CLOSING:
				/*
				 * FIN acknowledged - release the socket in
				 * another fibril.
				 */
				tcp_prepare_timeout(tcp_release_after_timeout,
				    socket, socket_data, 0,
				    TCP_SOCKET_TIME_WAIT,
				    NET_DEFAULT_TCP_TIME_WAIT_TIMEOUT, true);
				break;
			default:
				break;
			}
		}

		/* Update the treshold if higher than set */
		if (number + ntohs(header->window) >
		    socket_data->expected + socket_data->treshold) {
			socket_data->treshold = number + ntohs(header->window) -
			    socket_data->expected;
		}

		/* Set new expected sequence number */
		socket_data->expected = number;
		socket_data->expected_count = 1;
		packet = socket_data->outgoing;
		while (pq_get_order(packet, &number, &length) == EOK) {
			if (IS_IN_INTERVAL_OVERFLOW((uint32_t) old,
			    (uint32_t) (number + length),
			    (uint32_t) socket_data->expected)) {
				next = pq_detach(packet);
				if (packet == socket_data->outgoing)
					socket_data->outgoing = next;

				/* Add to acknowledged or release */
				if (pq_add(&acknowledged, packet, 0, 0) != EOK)
					pq_release_remote(tcp_globals.net_sess,
					    packet_get_id(packet));
				packet = next;
			} else if (old < socket_data->expected)
				break;
		}

		/* Release acknowledged */
		if (acknowledged) {
			pq_release_remote(tcp_globals.net_sess,
			    packet_get_id(acknowledged));
		}
		return;
		/* If the same as the previous time */
	}

	if (number == socket_data->expected) {
		/* Increase the counter */
		socket_data->expected_count++;
		if (socket_data->expected_count == TCP_FAST_RETRANSMIT_COUNT) {
			socket_data->expected_count = 1;
			/* TODO retransmit lock */
			//tcp_retransmit_packet(socket, socket_data, number);
		}
	}
}

/** Per-connection initialization
 *
 */
void tl_connection(void)
{
}

/** Processes the TCP message.
 *
 * @param[in] callid	The message identifier.
 * @param[in] call	The message parameters.
 * @param[out] answer	The message answer parameters.
 * @param[out] answer_count The last parameter for the actual answer in the
 *			answer parameter.
 * @return		EOK on success.
 * @return		ENOTSUP if the message is not known.
 *
 * @see tcp_interface.h
 * @see IS_NET_TCP_MESSAGE()
 */
int tl_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, size_t *answer_count)
{
	assert(call);
	assert(answer);
	assert(answer_count);
	
	*answer_count = 0;
	
	async_sess_t *callback =
	    async_callback_receive_start(EXCHANGE_SERIALIZE, call);
	if (callback)
		return tcp_process_client_messages(callback, callid, *call);
	
	return ENOTSUP;
}

void tcp_refresh_socket_data(tcp_socket_data_t *socket_data)
{
	assert(socket_data);

	bzero(socket_data, sizeof(*socket_data));
	socket_data->state = TCP_SOCKET_INITIAL;
	socket_data->device_id = NIC_DEVICE_INVALID_ID;
	socket_data->window = NET_DEFAULT_TCP_WINDOW;
	socket_data->treshold = socket_data->window;
	socket_data->last_outgoing = TCP_INITIAL_SEQUENCE_NUMBER;
	socket_data->timeout = NET_DEFAULT_TCP_INITIAL_TIMEOUT;
	socket_data->acknowledged = socket_data->last_outgoing;
	socket_data->next_outgoing = socket_data->last_outgoing + 1;
	socket_data->expected = socket_data->next_outgoing;
}

void tcp_initialize_socket_data(tcp_socket_data_t *socket_data)
{
	assert(socket_data);

	tcp_refresh_socket_data(socket_data);
	fibril_mutex_initialize(&socket_data->operation.mutex);
	fibril_condvar_initialize(&socket_data->operation.condvar);
	socket_data->data_fragment_size = MAX_TCP_FRAGMENT_SIZE;
}

int tcp_process_client_messages(async_sess_t *sess, ipc_callid_t callid,
    ipc_call_t call)
{
	int res;
	socket_cores_t local_sockets;
	struct sockaddr *addr;
	int socket_id;
	size_t addrlen;
	size_t size;
	fibril_rwlock_t lock;
	ipc_call_t answer;
	size_t answer_count;
	tcp_socket_data_t *socket_data;
	socket_core_t *socket;
	packet_dimension_t *packet_dimension;

	/*
	 * Accept the connection
	 *  - Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	res = EOK;
	answer_count = 0;

	socket_cores_initialize(&local_sockets);
	fibril_rwlock_initialize(&lock);

	while (true) {

		/* Answer the call */
		answer_call(callid, res, &answer, answer_count);
		/* Refresh data */
		refresh_answer(&answer, &answer_count);
		/* Get the next call */
		callid = async_get_call(&call);
		
		if (!IPC_GET_IMETHOD(call)) {
			res = EHANGUP;
			break;
		}

		/* Process the call */
		switch (IPC_GET_IMETHOD(call)) {
		case NET_SOCKET:
			socket_data =
			    (tcp_socket_data_t *) malloc(sizeof(*socket_data));
			if (!socket_data) {
				res = ENOMEM;
				break;
			}
			
			tcp_initialize_socket_data(socket_data);
			socket_data->local_lock = &lock;
			socket_data->local_sockets = &local_sockets;
			fibril_rwlock_write_lock(&lock);
			socket_id = SOCKET_GET_SOCKET_ID(call);
			res = socket_create(&local_sockets, sess,
			    socket_data, &socket_id);
			SOCKET_SET_SOCKET_ID(answer, socket_id);
			fibril_rwlock_write_unlock(&lock);
			if (res != EOK) {
				free(socket_data);
				break;
			}
			if (tl_get_ip_packet_dimension(tcp_globals.ip_sess,
			    &tcp_globals.dimensions, NIC_DEVICE_INVALID_ID,
			    &packet_dimension) == EOK) {
				SOCKET_SET_DATA_FRAGMENT_SIZE(answer,
				    ((packet_dimension->content <
				    socket_data->data_fragment_size) ?
				    packet_dimension->content :
				    socket_data->data_fragment_size));
			}
//                      SOCKET_SET_DATA_FRAGMENT_SIZE(answer, MAX_TCP_FRAGMENT_SIZE);
			SOCKET_SET_HEADER_SIZE(answer, TCP_HEADER_SIZE);
			answer_count = 3;
			break;

		case NET_SOCKET_BIND:
			res = async_data_write_accept((void **) &addr, false,
			    0, 0, 0, &addrlen);
			if (res != EOK)
				break;
			fibril_rwlock_write_lock(&tcp_globals.lock);
			fibril_rwlock_write_lock(&lock);
			res = socket_bind(&local_sockets, &tcp_globals.sockets,
			    SOCKET_GET_SOCKET_ID(call), addr, addrlen,
			    TCP_FREE_PORTS_START, TCP_FREE_PORTS_END,
			    tcp_globals.last_used_port);
			if (res == EOK) {
				socket = socket_cores_find(&local_sockets,
				    SOCKET_GET_SOCKET_ID(call));
				if (socket) {
					socket_data = (tcp_socket_data_t *)
					    socket->specific_data;
					assert(socket_data);
					socket_data->state = TCP_SOCKET_LISTEN;
				}
			}
			fibril_rwlock_write_unlock(&lock);
			fibril_rwlock_write_unlock(&tcp_globals.lock);
			free(addr);
			break;

		case NET_SOCKET_LISTEN:
			fibril_rwlock_read_lock(&tcp_globals.lock);
//                      fibril_rwlock_write_lock(&tcp_globals.lock);
			fibril_rwlock_write_lock(&lock);
			res = tcp_listen_message(&local_sockets,
			    SOCKET_GET_SOCKET_ID(call),
			    SOCKET_GET_BACKLOG(call));
			fibril_rwlock_write_unlock(&lock);
//                      fibril_rwlock_write_unlock(&tcp_globals.lock);
			fibril_rwlock_read_unlock(&tcp_globals.lock);
			break;

		case NET_SOCKET_CONNECT:
			res = async_data_write_accept((void **) &addr, false,
			    0, 0, 0, &addrlen);
			if (res != EOK)
				break;
			/*
			 * The global lock may be released in the
			 * tcp_connect_message() function.
			 */
			fibril_rwlock_write_lock(&tcp_globals.lock);
			fibril_rwlock_write_lock(&lock);
			res = tcp_connect_message(&local_sockets,
			    SOCKET_GET_SOCKET_ID(call), addr, addrlen);
			if (res != EOK) {
				fibril_rwlock_write_unlock(&lock);
				fibril_rwlock_write_unlock(&tcp_globals.lock);
				free(addr);
			}
			break;

		case NET_SOCKET_ACCEPT:
			fibril_rwlock_read_lock(&tcp_globals.lock);
			fibril_rwlock_write_lock(&lock);
			res = tcp_accept_message(&local_sockets,
			    SOCKET_GET_SOCKET_ID(call),
			    SOCKET_GET_NEW_SOCKET_ID(call), &size, &addrlen);
			SOCKET_SET_DATA_FRAGMENT_SIZE(answer, size);
			fibril_rwlock_write_unlock(&lock);
			fibril_rwlock_read_unlock(&tcp_globals.lock);
			if (res > 0) {
				SOCKET_SET_SOCKET_ID(answer, res);
				SOCKET_SET_ADDRESS_LENGTH(answer, addrlen);
				answer_count = 3;
			}
			break;

		case NET_SOCKET_SEND:
			fibril_rwlock_read_lock(&tcp_globals.lock);
			fibril_rwlock_write_lock(&lock);
			res = tcp_send_message(&local_sockets,
			    SOCKET_GET_SOCKET_ID(call),
			    SOCKET_GET_DATA_FRAGMENTS(call), &size,
			    SOCKET_GET_FLAGS(call));
			SOCKET_SET_DATA_FRAGMENT_SIZE(answer, size);
			if (res != EOK) {
				fibril_rwlock_write_unlock(&lock);
				fibril_rwlock_read_unlock(&tcp_globals.lock);
			} else {
				answer_count = 2;
			}
			break;

		case NET_SOCKET_SENDTO:
			res = async_data_write_accept((void **) &addr, false,
			    0, 0, 0, &addrlen);
			if (res != EOK)
				break;
			fibril_rwlock_read_lock(&tcp_globals.lock);
			fibril_rwlock_write_lock(&lock);
			res = tcp_send_message(&local_sockets,
			    SOCKET_GET_SOCKET_ID(call),
			    SOCKET_GET_DATA_FRAGMENTS(call), &size,
			    SOCKET_GET_FLAGS(call));
			SOCKET_SET_DATA_FRAGMENT_SIZE(answer, size);
			if (res != EOK) {
				fibril_rwlock_write_unlock(&lock);
				fibril_rwlock_read_unlock(&tcp_globals.lock);
			} else {
				answer_count = 2;
			}
			free(addr);
			break;

		case NET_SOCKET_RECV:
			fibril_rwlock_read_lock(&tcp_globals.lock);
			fibril_rwlock_write_lock(&lock);
			res = tcp_recvfrom_message(&local_sockets,
			    SOCKET_GET_SOCKET_ID(call), SOCKET_GET_FLAGS(call),
			    NULL);
			fibril_rwlock_write_unlock(&lock);
			fibril_rwlock_read_unlock(&tcp_globals.lock);
			if (res > 0) {
				SOCKET_SET_READ_DATA_LENGTH(answer, res);
				answer_count = 1;
				res = EOK;
			}
			break;

		case NET_SOCKET_RECVFROM:
			fibril_rwlock_read_lock(&tcp_globals.lock);
			fibril_rwlock_write_lock(&lock);
			res = tcp_recvfrom_message(&local_sockets,
			    SOCKET_GET_SOCKET_ID(call), SOCKET_GET_FLAGS(call),
			    &addrlen);
			fibril_rwlock_write_unlock(&lock);
			fibril_rwlock_read_unlock(&tcp_globals.lock);
			if (res > 0) {
				SOCKET_SET_READ_DATA_LENGTH(answer, res);
				SOCKET_SET_ADDRESS_LENGTH(answer, addrlen);
				answer_count = 3;
				res = EOK;
			}
			break;

		case NET_SOCKET_CLOSE:
			fibril_rwlock_write_lock(&tcp_globals.lock);
			fibril_rwlock_write_lock(&lock);
			res = tcp_close_message(&local_sockets,
			    SOCKET_GET_SOCKET_ID(call));
			if (res != EOK) {
				fibril_rwlock_write_unlock(&lock);
				fibril_rwlock_write_unlock(&tcp_globals.lock);
			}
			break;

		case NET_SOCKET_GETSOCKOPT:
		case NET_SOCKET_SETSOCKOPT:
		default:
			res = ENOTSUP;
			break;
		}
	}

	/* Release the application session */
	async_hangup(sess);

	printf("release\n");
	/* Release all local sockets */
	socket_cores_release(tcp_globals.net_sess, &local_sockets,
	    &tcp_globals.sockets, tcp_free_socket_data);

	return EOK;
}

int tcp_timeout(void *data)
{
	tcp_timeout_t *timeout = data;
	int keep_write_lock = false;
	socket_core_t *socket;
	tcp_socket_data_t *socket_data;

	assert(timeout);

	/* Sleep the given timeout */
	async_usleep(timeout->timeout);
	/* Lock the globals */
	if (timeout->globals_read_only) 
		fibril_rwlock_read_lock(&tcp_globals.lock);
	else 
		fibril_rwlock_write_lock(&tcp_globals.lock);

	/* Find the pending operation socket */
	socket = socket_port_find(&tcp_globals.sockets, timeout->port,
	    timeout->key, timeout->key_length);
	if (!socket || (socket->socket_id != timeout->socket_id))
		goto out;
	
	socket_data = (tcp_socket_data_t *) socket->specific_data;
	assert(socket_data);
	if (socket_data->local_sockets != timeout->local_sockets)
		goto out;
	
	fibril_rwlock_write_lock(socket_data->local_lock);
	if (timeout->sequence_number) {
		/* Increase the timeout counter */
		socket_data->timeout_count++;
		if (socket_data->timeout_count == TCP_MAX_TIMEOUTS) {
			/* TODO release as connection lost */
			//tcp_refresh_socket_data(socket_data);
			fibril_rwlock_write_unlock(socket_data->local_lock);
		} else {
			/* Retransmit */
//                      tcp_retransmit_packet(socket,
//			    socket_data, timeout->sequence_number);
			fibril_rwlock_write_unlock(socket_data->local_lock);
		}
	} else {
		fibril_mutex_lock(&socket_data->operation.mutex);
		/* Set the timeout operation result if state not changed */
		if (socket_data->state == timeout->state) {
			socket_data->operation.result = ETIMEOUT;

			/* Notify the main fibril */
			fibril_condvar_signal(&socket_data->operation.condvar);

			/* Keep the global write lock */
			keep_write_lock = true;
		} else {
			/*
			 * Operation is ok, do nothing.
			 * Unlocking from now on, so the unlocking
			 * order does not matter.
			 */
			fibril_rwlock_write_unlock(socket_data->local_lock);
		}
		fibril_mutex_unlock(&socket_data->operation.mutex);
	}

out:
	/* Unlock only if no socket */
	if (timeout->globals_read_only)
		fibril_rwlock_read_unlock(&tcp_globals.lock);
	else if (!keep_write_lock)
		/* Release if not desired */
		fibril_rwlock_write_unlock(&tcp_globals.lock);
	
	/* Release the timeout structure */
	free(timeout);
	return EOK;
}

int tcp_release_after_timeout(void *data)
{
	tcp_timeout_t *timeout = data;
	socket_core_t *socket;
	tcp_socket_data_t *socket_data;
	fibril_rwlock_t *local_lock;

	assert(timeout);

	/* Sleep the given timeout */
	async_usleep(timeout->timeout);

	/* Lock the globals */
	fibril_rwlock_write_lock(&tcp_globals.lock);

	/* Find the pending operation socket */
	socket = socket_port_find(&tcp_globals.sockets, timeout->port,
	    timeout->key, timeout->key_length);

	if (socket && (socket->socket_id == timeout->socket_id)) {
		socket_data = (tcp_socket_data_t *) socket->specific_data;
		assert(socket_data);
		if (socket_data->local_sockets == timeout->local_sockets) {
			local_lock = socket_data->local_lock;
			fibril_rwlock_write_lock(local_lock);
			socket_destroy(tcp_globals.net_sess,
			    timeout->socket_id, timeout->local_sockets,
			    &tcp_globals.sockets, tcp_free_socket_data);
			fibril_rwlock_write_unlock(local_lock);
		}
	}

	/* Unlock the globals */
	fibril_rwlock_write_unlock(&tcp_globals.lock);

	/* Release the timeout structure */
	free(timeout);

	return EOK;
}

void tcp_retransmit_packet(socket_core_t *socket, tcp_socket_data_t *
    socket_data, size_t sequence_number)
{
	packet_t *packet;
	packet_t *copy;
	size_t data_length;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	/* Sent packet? */
	packet = pq_find(socket_data->outgoing, sequence_number);
	printf("retransmit %lu\n", packet_get_id(packet));
	if (packet) {
		pq_get_order(packet, NULL, &data_length);
		copy = tcp_prepare_copy(socket, socket_data, packet,
		    data_length, sequence_number);
		fibril_rwlock_write_unlock(socket_data->local_lock);
//              printf("r send %d\n", packet_get_id(packet));
		if (copy) 
			tcp_send_packets(socket_data->device_id, copy);
	} else {
		fibril_rwlock_write_unlock(socket_data->local_lock);
	}
}

int tcp_listen_message(socket_cores_t *local_sockets, int socket_id,
    int backlog)
{
	socket_core_t *socket;
	tcp_socket_data_t *socket_data;

	assert(local_sockets);

	if (backlog < 0) 
		return EINVAL;

	/* Find the socket */
	socket = socket_cores_find(local_sockets, socket_id);
	if (!socket) 
		return ENOTSOCK;
	
	/* Get the socket specific data */
	socket_data = (tcp_socket_data_t *) socket->specific_data;
	assert(socket_data);

	/* Set the backlog */
	socket_data->backlog = backlog;

	return EOK;
}

int tcp_connect_message(socket_cores_t *local_sockets, int socket_id,
    struct sockaddr *addr, socklen_t addrlen)
{
	socket_core_t *socket;
	int rc;

	assert(local_sockets);
	assert(addr);
	assert(addrlen > 0);

	/* Find the socket */
	socket = socket_cores_find(local_sockets, socket_id);
	if (!socket)
		return ENOTSOCK;
	
	rc = tcp_connect_core(socket, local_sockets, addr, addrlen);
	if (rc != EOK) {
		tcp_free_socket_data(socket);
		/* Unbind if bound */
		if (socket->port > 0) {
			socket_ports_exclude(&tcp_globals.sockets,
			    socket->port, free);
			socket->port = 0;
		}
	}
	return rc;
}

int tcp_connect_core(socket_core_t *socket, socket_cores_t *local_sockets,
    struct sockaddr *addr, socklen_t addrlen)
{
	tcp_socket_data_t *socket_data;
	packet_t *packet;
	int rc;

	assert(socket);
	assert(addr);
	assert(addrlen > 0);

	/* Get the socket specific data */
	socket_data = (tcp_socket_data_t *) socket->specific_data;
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	if ((socket_data->state != TCP_SOCKET_INITIAL) &&
	    ((socket_data->state != TCP_SOCKET_LISTEN) ||
	    (socket->port <= 0)))
		return EINVAL;

	/* Get the destination port */
	rc = tl_get_address_port(addr, addrlen, &socket_data->dest_port);
	if (rc != EOK)
		return rc;
	
	if (socket->port <= 0) {
		/* Try to find a free port */
		rc = socket_bind_free_port(&tcp_globals.sockets, socket,
		    TCP_FREE_PORTS_START, TCP_FREE_PORTS_END,
		    tcp_globals.last_used_port);
		if (rc != EOK)
			return rc;
		/* Set the next port as the search starting port number */
		tcp_globals.last_used_port = socket->port;
	}

	rc = ip_get_route_req(tcp_globals.ip_sess, IPPROTO_TCP,
	    addr, addrlen, &socket_data->device_id,
	    &socket_data->pseudo_header, &socket_data->headerlen);
	if (rc != EOK)
		return rc;

	/* Create the notification packet */
	rc = tcp_create_notification_packet(&packet, socket, socket_data, 1, 0);
	if (rc != EOK)
		return rc;

	/* Unlock the globals and wait for an operation */
	fibril_rwlock_write_unlock(&tcp_globals.lock);

	socket_data->addr = addr;
	socket_data->addrlen = addrlen;

	/* Send the packet */

	if (((rc = tcp_queue_packet(socket, socket_data, packet, 1)) != EOK) ||
	    ((rc = tcp_prepare_timeout(tcp_timeout, socket, socket_data, 0,
	    TCP_SOCKET_INITIAL, NET_DEFAULT_TCP_INITIAL_TIMEOUT, false)) !=
	    EOK)) {
		socket_data->addr = NULL;
		socket_data->addrlen = 0;
		fibril_rwlock_write_lock(&tcp_globals.lock);
	} else {
		packet = tcp_get_packets_to_send(socket, socket_data);
		if (packet) {
			fibril_mutex_lock(&socket_data->operation.mutex);
			fibril_rwlock_write_unlock(socket_data->local_lock);

			socket_data->state = TCP_SOCKET_SYN_SENT;

			/* Send the packet */
			printf("connecting %lu\n", packet_get_id(packet));
			tcp_send_packets(socket_data->device_id, packet);

			/* Wait for a reply */
			fibril_condvar_wait(&socket_data->operation.condvar,
			    &socket_data->operation.mutex);
			rc = socket_data->operation.result;
			if (rc != EOK) {
				socket_data->addr = NULL;
				socket_data->addrlen = 0;
			}
		} else {
			socket_data->addr = NULL;
			socket_data->addrlen = 0;
			rc = EINTR;
		}
	}

	fibril_mutex_unlock(&socket_data->operation.mutex);
	return rc;
}

int tcp_queue_prepare_packet(socket_core_t *socket,
    tcp_socket_data_t *socket_data, packet_t *packet, size_t data_length)
{
	tcp_header_t *header;
	int rc;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	/* Get TCP header */
	header = (tcp_header_t *) packet_get_data(packet);
	if (!header)
		return NO_DATA;
	
	header->destination_port = htons(socket_data->dest_port);
	header->source_port = htons(socket->port);
	header->sequence_number = htonl(socket_data->next_outgoing);

	rc = packet_set_addr(packet, NULL, (uint8_t *) socket_data->addr,
	    socket_data->addrlen);
	if (rc != EOK)
		return tcp_release_and_return(packet, EINVAL);

	/* Remember the outgoing FIN */
	if (GET_TCP_HEADER_FINALIZE(header))
		socket_data->fin_outgoing = socket_data->next_outgoing;
	
	return EOK;
}

int tcp_queue_packet(socket_core_t *socket, tcp_socket_data_t *socket_data,
    packet_t *packet, size_t data_length)
{
	int rc;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	rc = tcp_queue_prepare_packet(socket, socket_data, packet, data_length);
	if (rc != EOK)
		return rc;

	rc = pq_add(&socket_data->outgoing, packet, socket_data->next_outgoing,
	    data_length);
	if (rc != EOK)
		return tcp_release_and_return(packet, rc);

	socket_data->next_outgoing += data_length;
	return EOK;
}

packet_t *tcp_get_packets_to_send(socket_core_t *socket, tcp_socket_data_t *
    socket_data)
{
	packet_t *packet;
	packet_t *copy;
	packet_t *sending = NULL;
	packet_t *previous = NULL;
	size_t data_length;
	int rc;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	packet = pq_find(socket_data->outgoing, socket_data->last_outgoing + 1);
	while (packet) {
		pq_get_order(packet, NULL, &data_length);

		/*
		 * Send only if fits into the window, respecting the possible
		 * overflow.
		 */
		if (!IS_IN_INTERVAL_OVERFLOW(
		    (uint32_t) socket_data->last_outgoing,
		    (uint32_t) (socket_data->last_outgoing + data_length),
		    (uint32_t) (socket_data->expected + socket_data->treshold)))
			break;

		copy = tcp_prepare_copy(socket, socket_data, packet,
		    data_length, socket_data->last_outgoing + 1);
		if (!copy) 
			return sending;
			
		if (!sending) {
			sending = copy;
		} else {
			rc = pq_insert_after(previous, copy);
			if (rc != EOK) {
				pq_release_remote(tcp_globals.net_sess,
				    packet_get_id(copy));
				return sending;
			}
		}

		previous = copy;
		packet = pq_next(packet);

		/* Overflow occurred? */
		if (!packet &&
		    (socket_data->last_outgoing > socket_data->next_outgoing)) {
			printf("gpts overflow\n");
			/* Continue from the beginning */
			packet = socket_data->outgoing;
		}
		socket_data->last_outgoing += data_length;
	}

	return sending;
}

packet_t *tcp_send_prepare_packet(socket_core_t *socket, tcp_socket_data_t *
    socket_data, packet_t *packet, size_t data_length, size_t sequence_number)
{
	tcp_header_t *header;
	uint32_t checksum;
	int rc;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	/* Adjust the pseudo header */
	rc = ip_client_set_pseudo_header_data_length(socket_data->pseudo_header,
	    socket_data->headerlen, packet_get_data_length(packet));
	if (rc != EOK) {
		pq_release_remote(tcp_globals.net_sess, packet_get_id(packet));
		return NULL;
	}

	/* Get the header */
	header = (tcp_header_t *) packet_get_data(packet);
	if (!header) {
		pq_release_remote(tcp_globals.net_sess, packet_get_id(packet));
		return NULL;
	}
	assert(ntohl(header->sequence_number) == sequence_number);

	/* Adjust the header */
	if (socket_data->next_incoming) {
		header->acknowledgement_number =
		    htonl(socket_data->next_incoming);
		SET_TCP_HEADER_ACKNOWLEDGE(header, 1);
	}
	header->window = htons(socket_data->window);

	/* Checksum */
	header->checksum = 0;
	checksum = compute_checksum(0, socket_data->pseudo_header,
	    socket_data->headerlen);
	checksum = compute_checksum(checksum,
	    (uint8_t *) packet_get_data(packet),
	    packet_get_data_length(packet));
	header->checksum = htons(flip_checksum(compact_checksum(checksum)));

	/* Prepare the packet */
	rc = ip_client_prepare_packet(packet, IPPROTO_TCP, 0, 0, 0, 0);
	if (rc != EOK) {
		pq_release_remote(tcp_globals.net_sess, packet_get_id(packet));
		return NULL;
	}

	rc = tcp_prepare_timeout(tcp_timeout, socket, socket_data,
	    sequence_number, socket_data->state, socket_data->timeout, true);
	if (rc != EOK) {
		pq_release_remote(tcp_globals.net_sess, packet_get_id(packet));
		return NULL;
	}

	return packet;
}

packet_t *tcp_prepare_copy(socket_core_t *socket, tcp_socket_data_t *
    socket_data, packet_t *packet, size_t data_length, size_t sequence_number)
{
	packet_t *copy;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	/* Make a copy of the packet */
	copy = packet_get_copy(tcp_globals.net_sess, packet);
	if (!copy)
		return NULL;

	return tcp_send_prepare_packet(socket, socket_data, copy, data_length,
	    sequence_number);
}

void tcp_send_packets(nic_device_id_t device_id, packet_t *packet)
{
	packet_t *next;

	while (packet) {
		next = pq_detach(packet);
		ip_send_msg(tcp_globals.ip_sess, device_id, packet,
		    SERVICE_TCP, 0);
		packet = next;
	}
}

void tcp_prepare_operation_header(socket_core_t *socket,
    tcp_socket_data_t *socket_data, tcp_header_t *header, int synchronize,
    int finalize)
{
	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(header);

	bzero(header, sizeof(*header));
	header->source_port = htons(socket->port);
	header->source_port = htons(socket_data->dest_port);
	SET_TCP_HEADER_LENGTH(header,
	    TCP_COMPUTE_HEADER_LENGTH(sizeof(*header)));
	SET_TCP_HEADER_SYNCHRONIZE(header, synchronize);
	SET_TCP_HEADER_FINALIZE(header, finalize);
}

int tcp_prepare_timeout(int (*timeout_function)(void *tcp_timeout_t),
    socket_core_t *socket, tcp_socket_data_t *socket_data,
    size_t sequence_number, tcp_socket_state_t state, suseconds_t timeout,
    int globals_read_only)
{
	tcp_timeout_t *operation_timeout;
	fid_t fibril;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	/* Prepare the timeout with key bundle structure */
	operation_timeout = malloc(sizeof(*operation_timeout) +
	    socket->key_length + 1);
	if (!operation_timeout)
		return ENOMEM;

	bzero(operation_timeout, sizeof(*operation_timeout));
	operation_timeout->globals_read_only = globals_read_only;
	operation_timeout->port = socket->port;
	operation_timeout->local_sockets = socket_data->local_sockets;
	operation_timeout->socket_id = socket->socket_id;
	operation_timeout->timeout = timeout;
	operation_timeout->sequence_number = sequence_number;
	operation_timeout->state = state;

	/* Copy the key */
	operation_timeout->key = ((uint8_t *) operation_timeout) +
	    sizeof(*operation_timeout);
	operation_timeout->key_length = socket->key_length;
	memcpy(operation_timeout->key, socket->key, socket->key_length);
	operation_timeout->key[operation_timeout->key_length] = '\0';

	/* Prepare the timeouting thread */
	fibril = fibril_create(timeout_function, operation_timeout);
	if (!fibril) {
		free(operation_timeout);
		return ENOMEM;
	}

//      fibril_mutex_lock(&socket_data->operation.mutex);
	/* Start the timeout fibril */
	fibril_add_ready(fibril);
	//socket_data->state = state;
	return EOK;
}

int tcp_recvfrom_message(socket_cores_t *local_sockets, int socket_id,
    int flags, size_t *addrlen)
{
	socket_core_t *socket;
	tcp_socket_data_t *socket_data;
	int packet_id;
	packet_t *packet;
	size_t length;
	int rc;

	assert(local_sockets);

	/* Find the socket */
	socket = socket_cores_find(local_sockets, socket_id);
	if (!socket)
		return ENOTSOCK;

	/* Get the socket specific data */
	if (!socket->specific_data)
		return NO_DATA;

	socket_data = (tcp_socket_data_t *) socket->specific_data;

	/* Check state */
	if ((socket_data->state != TCP_SOCKET_ESTABLISHED) &&
	    (socket_data->state != TCP_SOCKET_CLOSE_WAIT))
		return ENOTCONN;

	/* Send the source address if desired */
	if (addrlen) {
		rc = data_reply(socket_data->addr, socket_data->addrlen);
		if (rc != EOK)
			return rc;
		*addrlen = socket_data->addrlen;
	}

	/* Get the next received packet */
	packet_id = dyn_fifo_value(&socket->received);
	if (packet_id < 0)
		return NO_DATA;

	rc = packet_translate_remote(tcp_globals.net_sess, &packet, packet_id);
	if (rc != EOK)
		return rc;

	/* Reply the packets */
	rc = socket_reply_packets(packet, &length);
	if (rc != EOK)
		return rc;

	/* Release the packet */
	dyn_fifo_pop(&socket->received);
	pq_release_remote(tcp_globals.net_sess, packet_get_id(packet));

	/* Return the total length */
	return (int) length;
}

int tcp_send_message(socket_cores_t *local_sockets, int socket_id,
    int fragments, size_t *data_fragment_size, int flags)
{
	socket_core_t *socket;
	tcp_socket_data_t *socket_data;
	packet_dimension_t *packet_dimension;
	packet_t *packet;
	size_t total_length;
	tcp_header_t *header;
	int index;
	int result;
	int rc;

	assert(local_sockets);
	assert(data_fragment_size);

	/* Find the socket */
	socket = socket_cores_find(local_sockets, socket_id);
	if (!socket)
		return ENOTSOCK;

	/* Get the socket specific data */
	if (!socket->specific_data)
		return NO_DATA;

	socket_data = (tcp_socket_data_t *) socket->specific_data;

	/* Check state */
	if ((socket_data->state != TCP_SOCKET_ESTABLISHED) &&
	    (socket_data->state != TCP_SOCKET_CLOSE_WAIT))
		return ENOTCONN;

	rc = tl_get_ip_packet_dimension(tcp_globals.ip_sess,
	    &tcp_globals.dimensions, socket_data->device_id, &packet_dimension);
	if (rc != EOK)
		return rc;

	*data_fragment_size =
	    ((packet_dimension->content < socket_data->data_fragment_size) ?
	    packet_dimension->content : socket_data->data_fragment_size);

	for (index = 0; index < fragments; index++) {
		/* Read the data fragment */
		result = tl_socket_read_packet_data(tcp_globals.net_sess,
		    &packet, TCP_HEADER_SIZE, packet_dimension,
		    socket_data->addr, socket_data->addrlen);
		if (result < 0)
			return result;

		total_length = (size_t) result;

		/* Prefix the TCP header */
		header = PACKET_PREFIX(packet, tcp_header_t);
		if (!header)
			return tcp_release_and_return(packet, ENOMEM);

		tcp_prepare_operation_header(socket, socket_data, header, 0, 0);
		rc = tcp_queue_packet(socket, socket_data, packet, total_length);
		if (rc != EOK)
			return rc;
	}

	/* Flush packets */
	packet = tcp_get_packets_to_send(socket, socket_data);
	fibril_rwlock_write_unlock(socket_data->local_lock);
	fibril_rwlock_read_unlock(&tcp_globals.lock);

	if (packet) {
		/* Send the packet */
		tcp_send_packets(socket_data->device_id, packet);
	}

	return EOK;
}

int
tcp_close_message(socket_cores_t *local_sockets, int socket_id)
{
	socket_core_t *socket;
	tcp_socket_data_t *socket_data;
	packet_t *packet;
	int rc;

	/* Find the socket */
	socket = socket_cores_find(local_sockets, socket_id);
	if (!socket)
		return ENOTSOCK;

	/* Get the socket specific data */
	socket_data = (tcp_socket_data_t *) socket->specific_data;
	assert(socket_data);

	/* Check state */
	switch (socket_data->state) {
	case TCP_SOCKET_ESTABLISHED:
		socket_data->state = TCP_SOCKET_FIN_WAIT_1;
		break;

	case TCP_SOCKET_CLOSE_WAIT:
		socket_data->state = TCP_SOCKET_LAST_ACK;
		break;

//      case TCP_SOCKET_LISTEN:

	default:
		/* Just destroy */
		rc = socket_destroy(tcp_globals.net_sess, socket_id,
		    local_sockets, &tcp_globals.sockets,
		    tcp_free_socket_data);
		if (rc == EOK) {
			fibril_rwlock_write_unlock(socket_data->local_lock);
			fibril_rwlock_write_unlock(&tcp_globals.lock);
		}
		return rc;
	}

	/*
	 * Send FIN.
	 * TODO should I wait to complete?
	 */

	/* Create the notification packet */
	rc = tcp_create_notification_packet(&packet, socket, socket_data, 0, 1);
	if (rc != EOK)
		return rc;

	/* Send the packet */
	rc = tcp_queue_packet(socket, socket_data, packet, 1);
	if (rc != EOK)
		return rc;

	/* Flush packets */
	packet = tcp_get_packets_to_send(socket, socket_data);
	fibril_rwlock_write_unlock(socket_data->local_lock);
	fibril_rwlock_write_unlock(&tcp_globals.lock);

	if (packet) {
		/* Send the packet */
		tcp_send_packets(socket_data->device_id, packet);
	}

	return EOK;
}

int tcp_create_notification_packet(packet_t **packet, socket_core_t *socket,
    tcp_socket_data_t *socket_data, int synchronize, int finalize)
{
	packet_dimension_t *packet_dimension;
	tcp_header_t *header;
	int rc;

	assert(packet);

	/* Get the device packet dimension */
	rc = tl_get_ip_packet_dimension(tcp_globals.ip_sess,
	    &tcp_globals.dimensions, socket_data->device_id, &packet_dimension);
	if (rc != EOK)
		return rc;

	/* Get a new packet */
	*packet = packet_get_4_remote(tcp_globals.net_sess, TCP_HEADER_SIZE,
	    packet_dimension->addr_len, packet_dimension->prefix,
	    packet_dimension->suffix);
	
	if (!*packet) 
		return ENOMEM;

	/* Allocate space in the packet */
	header = PACKET_SUFFIX(*packet, tcp_header_t);
	if (!header)
		tcp_release_and_return(*packet, ENOMEM);

	tcp_prepare_operation_header(socket, socket_data, header, synchronize,
	    finalize);

	return EOK;
}

int tcp_accept_message(socket_cores_t *local_sockets, int socket_id,
    int new_socket_id, size_t *data_fragment_size, size_t *addrlen)
{
	socket_core_t *accepted;
	socket_core_t *socket;
	tcp_socket_data_t *socket_data;
	packet_dimension_t *packet_dimension;
	int rc;

	assert(local_sockets);
	assert(data_fragment_size);
	assert(addrlen);

	/* Find the socket */
	socket = socket_cores_find(local_sockets, socket_id);
	if (!socket)
		return ENOTSOCK;

	/* Get the socket specific data */
	socket_data = (tcp_socket_data_t *) socket->specific_data;
	assert(socket_data);

	/* Check state */
	if (socket_data->state != TCP_SOCKET_LISTEN)
		return EINVAL;

	do {
		socket_id = dyn_fifo_value(&socket->accepted);
		if (socket_id < 0)
			return ENOTSOCK;
		socket_id *= -1;

		accepted = socket_cores_find(local_sockets, socket_id);
		if (!accepted)
			return ENOTSOCK;

		/* Get the socket specific data */
		socket_data = (tcp_socket_data_t *) accepted->specific_data;
		assert(socket_data);
		/* TODO can it be in another state? */
		if (socket_data->state == TCP_SOCKET_ESTABLISHED) {
			rc = data_reply(socket_data->addr,
			    socket_data->addrlen);
			if (rc != EOK)
				return rc;
			rc = tl_get_ip_packet_dimension(tcp_globals.ip_sess,
			    &tcp_globals.dimensions, socket_data->device_id,
			    &packet_dimension);
			if (rc != EOK)
				return rc;
			*addrlen = socket_data->addrlen;

			*data_fragment_size =
			    ((packet_dimension->content <
			    socket_data->data_fragment_size) ?
			    packet_dimension->content :
			    socket_data->data_fragment_size);
	
			if (new_socket_id > 0) {
				rc = socket_cores_update(local_sockets,
				    accepted->socket_id, new_socket_id);
				if (rc != EOK)
					return rc;
				accepted->socket_id = new_socket_id;
			}
		}
		dyn_fifo_pop(&socket->accepted);
	} while (socket_data->state != TCP_SOCKET_ESTABLISHED);

	printf("ret accept %d\n", accepted->socket_id);
	return accepted->socket_id;
}

void tcp_free_socket_data(socket_core_t *socket)
{
	tcp_socket_data_t *socket_data;

	assert(socket);

	printf("destroy_socket %d\n", socket->socket_id);

	/* Get the socket specific data */
	socket_data = (tcp_socket_data_t *) socket->specific_data;
	assert(socket_data);

	/* Free the pseudo header */
	if (socket_data->pseudo_header) {
		if (socket_data->headerlen) {
			printf("d pseudo\n");
			free(socket_data->pseudo_header);
			socket_data->headerlen = 0;
		}
		socket_data->pseudo_header = NULL;
	}

	socket_data->headerlen = 0;

	/* Free the address */
	if (socket_data->addr) {
		if (socket_data->addrlen) {
			printf("d addr\n");
			free(socket_data->addr);
			socket_data->addrlen = 0;
		}
		socket_data->addr = NULL;
	}
	socket_data->addrlen = 0;
}

/** Releases the packet and returns the result.
 *
 * @param[in] packet	The packet queue to be released.
 * @param[in] result	The result to be returned.
 * @return		The result parameter.
 */
int tcp_release_and_return(packet_t *packet, int result)
{
	pq_release_remote(tcp_globals.net_sess, packet_get_id(packet));
	return result;
}

/** Process IPC messages from the IP module
 *
 * @param[in]     iid   Message identifier.
 * @param[in,out] icall Message parameters.
 * @param[in]     arg   Local argument.
 *
 */
static void tcp_receiver(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	packet_t *packet;
	int rc;
	
	while (true) {
		switch (IPC_GET_IMETHOD(*icall)) {
		case NET_TL_RECEIVED:
			rc = packet_translate_remote(tcp_globals.net_sess, &packet,
			    IPC_GET_PACKET(*icall));
			if (rc == EOK)
				rc = tcp_received_msg(IPC_GET_DEVICE(*icall), packet,
				    SERVICE_TCP, IPC_GET_ERROR(*icall));
			
			async_answer_0(iid, (sysarg_t) rc);
			break;
		default:
			async_answer_0(iid, (sysarg_t) ENOTSUP);
		}
		
		iid = async_get_call(icall);
	}
}

/** Initialize the TCP module.
 *
 * @param[in] sess Network module session.
 *
 * @return EOK on success.
 * @return ENOMEM if there is not enough memory left.
 *
 */
int tl_initialize(async_sess_t *sess)
{
	fibril_rwlock_initialize(&tcp_globals.lock);
	fibril_rwlock_write_lock(&tcp_globals.lock);
	
	tcp_globals.net_sess = sess;
	
	tcp_globals.icmp_sess = icmp_connect_module();
	tcp_globals.ip_sess = ip_bind_service(SERVICE_IP, IPPROTO_TCP,
	    SERVICE_TCP, tcp_receiver);
	if (tcp_globals.ip_sess == NULL) {
		fibril_rwlock_write_unlock(&tcp_globals.lock);
		return ENOENT;
	}
	
	int rc = socket_ports_initialize(&tcp_globals.sockets);
	if (rc != EOK)
		goto out;

	rc = packet_dimensions_initialize(&tcp_globals.dimensions);
	if (rc != EOK) {
		socket_ports_destroy(&tcp_globals.sockets, free);
		goto out;
	}

	tcp_globals.last_used_port = TCP_FREE_PORTS_START - 1;

out:
	fibril_rwlock_write_unlock(&tcp_globals.lock);
	return rc;
}

int main(int argc, char *argv[])
{
	return tl_module_start(SERVICE_TCP);
}

/** @}
 */
