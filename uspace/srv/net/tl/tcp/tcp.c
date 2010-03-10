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
 *  @{
 */

/** @file
 *  TCP module implementation.
 *  @see tcp.h
 */

#include <assert.h>
#include <async.h>
#include <fibril_synch.h>
#include <malloc.h>
//TODO remove stdio
#include <stdio.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include "../../err.h"
#include "../../messages.h"
#include "../../modules.h"

#include "../../structures/dynamic_fifo.h"
#include "../../structures/packet/packet_client.h"

#include "../../include/checksum.h"
#include "../../include/in.h"
#include "../../include/in6.h"
#include "../../include/inet.h"
#include "../../include/ip_client.h"
#include "../../include/ip_interface.h"
#include "../../include/ip_protocols.h"
#include "../../include/icmp_client.h"
#include "../../include/icmp_interface.h"
#include "../../include/net_interface.h"
#include "../../include/socket_codes.h"
#include "../../include/socket_errno.h"
#include "../../include/tcp_codes.h"

#include "../../socket/socket_core.h"
#include "../../socket/socket_messages.h"

#include "../tl_common.h"
#include "../tl_messages.h"

#include "tcp.h"
#include "tcp_header.h"
#include "tcp_module.h"

/** The TCP window default value.
 */
#define NET_DEFAULT_TCP_WINDOW	10240

/** Initial timeout for new connections.
 */
#define NET_DEFAULT_TCP_INITIAL_TIMEOUT	3000000L

/** Default timeout for closing.
 */
#define NET_DEFAULT_TCP_TIME_WAIT_TIMEOUT	2000L

/** The initial outgoing sequence number.
 */
#define TCP_INITIAL_SEQUENCE_NUMBER		2999

/** Maximum TCP fragment size.
 */
#define MAX_TCP_FRAGMENT_SIZE	65535

/** Free ports pool start.
 */
#define TCP_FREE_PORTS_START	1025

/** Free ports pool end.
 */
#define TCP_FREE_PORTS_END		65535

/** Timeout for connection initialization, SYN sent.
 */
#define TCP_SYN_SENT_TIMEOUT	1000000L

/** The maximum number of timeouts in a row before singaling connection lost.
 */
#define TCP_MAX_TIMEOUTS		8

/** The number of acknowledgements before retransmit.
 */
#define TCP_FAST_RETRANSMIT_COUNT	3

/** Returns a value indicating whether the value is in the interval respecting the possible overflow.
 *  The high end and/or the value may overflow, be lower than the low value.
 *  @param[in] lower The last value before the interval.
 *  @param[in] value The value to be checked.
 *  @param[in] higher_equal The last value in the interval.
 */
#define IS_IN_INTERVAL_OVERFLOW(lower, value, higher_equal)	((((lower) < (value)) && (((value) <= (higher_equal)) || ((higher_equal) < (lower)))) || (((value) <= (higher_equal)) && ((higher_equal) < (lower))))

/** Type definition of the TCP timeout.
 *  @see tcp_timeout
 */
typedef struct tcp_timeout	tcp_timeout_t;

/** Type definition of the TCP timeout pointer.
 *  @see tcp_timeout
 */
typedef tcp_timeout_t *	tcp_timeout_ref;

/** TCP reply timeout data.
 *  Used as a timeouting fibril argument.
 *  @see tcp_timeout()
 */
struct tcp_timeout{
	/** TCP global data are going to be read only.
	 */
	int globals_read_only;
	/** Socket port.
	 */
	int port;
	/** Local sockets.
	 */
	socket_cores_ref local_sockets;
	/** Socket identifier.
	 */
	int socket_id;
	/** Socket state.
	 */
	tcp_socket_state_t state;
	/** Sent packet sequence number.
	 */
	int sequence_number;
	/** Timeout in microseconds.
	 */
	suseconds_t timeout;
	/** Port map key.
	 */
	char * key;
	/** Port map key length.
	 */
	size_t key_length;
};

/** Releases the packet and returns the result.
 *  @param[in] packet The packet queue to be released.
 *  @param[in] result The result to be returned.
 *  @return The result parameter.
 */
int tcp_release_and_return(packet_t packet, int result);

void tcp_prepare_operation_header(socket_core_ref socket, tcp_socket_data_ref socket_data, tcp_header_ref header, int synchronize, int finalize);
int tcp_prepare_timeout(int (*timeout_function)(void * tcp_timeout_t), socket_core_ref socket, tcp_socket_data_ref socket_data, size_t sequence_number, tcp_socket_state_t state, suseconds_t timeout, int globals_read_only);
void tcp_free_socket_data(socket_core_ref socket);
int tcp_timeout(void * data);
int tcp_release_after_timeout(void * data);
int tcp_process_packet(device_id_t device_id, packet_t packet, services_t error);
int tcp_connect_core(socket_core_ref socket, socket_cores_ref local_sockets, struct sockaddr * addr, socklen_t addrlen);
int tcp_queue_prepare_packet(socket_core_ref socket, tcp_socket_data_ref socket_data, packet_t packet, size_t data_length);
int tcp_queue_packet(socket_core_ref socket, tcp_socket_data_ref socket_data, packet_t packet, size_t data_length);
packet_t tcp_get_packets_to_send(socket_core_ref socket, tcp_socket_data_ref socket_data);
void tcp_send_packets(device_id_t device_id, packet_t packet);
void tcp_process_acknowledgement(socket_core_ref socket, tcp_socket_data_ref socket_data, tcp_header_ref header);
packet_t tcp_send_prepare_packet(socket_core_ref socket, tcp_socket_data_ref socket_data, packet_t packet, size_t data_length, size_t sequence_number);
packet_t tcp_prepare_copy(socket_core_ref socket, tcp_socket_data_ref socket_data, packet_t packet, size_t data_length, size_t sequence_number);
void tcp_retransmit_packet(socket_core_ref socket, tcp_socket_data_ref socket_data, size_t sequence_number);
int tcp_create_notification_packet(packet_t * packet, socket_core_ref socket, tcp_socket_data_ref socket_data, int synchronize, int finalize);
void tcp_refresh_socket_data(tcp_socket_data_ref socket_data);
void tcp_initialize_socket_data(tcp_socket_data_ref socket_data);
int tcp_process_listen(socket_core_ref listening_socket, tcp_socket_data_ref listening_socket_data, tcp_header_ref header, packet_t packet, struct sockaddr * src, struct sockaddr * dest, size_t addrlen);
int tcp_process_syn_sent(socket_core_ref socket, tcp_socket_data_ref socket_data, tcp_header_ref header, packet_t packet);
int tcp_process_syn_received(socket_core_ref socket, tcp_socket_data_ref socket_data, tcp_header_ref header, packet_t packet);
int tcp_process_established(socket_core_ref socket, tcp_socket_data_ref socket_data, tcp_header_ref header, packet_t packet, int fragments, size_t total_length);
int tcp_queue_received_packet(socket_core_ref socket, tcp_socket_data_ref socket_data, packet_t packet, int fragments, size_t total_length);

int tcp_received_msg(device_id_t device_id, packet_t packet, services_t receiver, services_t error);
int tcp_process_client_messages(ipc_callid_t callid, ipc_call_t call);
int tcp_listen_message(socket_cores_ref local_sockets, int socket_id, int backlog);
int tcp_connect_message(socket_cores_ref local_sockets, int socket_id, struct sockaddr * addr, socklen_t addrlen);
int tcp_recvfrom_message(socket_cores_ref local_sockets, int socket_id, int flags, size_t * addrlen);
int tcp_send_message(socket_cores_ref local_sockets, int socket_id, int fragments, size_t * data_fragment_size, int flags);
int tcp_accept_message(socket_cores_ref local_sockets, int socket_id, int new_socket_id, size_t * data_fragment_size, size_t * addrlen);
int tcp_close_message(socket_cores_ref local_sockets, int socket_id);

/** TCP global data.
 */
tcp_globals_t	tcp_globals;

int tcp_initialize(async_client_conn_t client_connection){
	ERROR_DECLARE;

	assert(client_connection);
	fibril_rwlock_initialize(&tcp_globals.lock);
	fibril_rwlock_write_lock(&tcp_globals.lock);
	tcp_globals.icmp_phone = icmp_connect_module(SERVICE_ICMP, ICMP_CONNECT_TIMEOUT);
	tcp_globals.ip_phone = ip_bind_service(SERVICE_IP, IPPROTO_TCP, SERVICE_TCP, client_connection, tcp_received_msg);
	if(tcp_globals.ip_phone < 0){
		return tcp_globals.ip_phone;
	}
	ERROR_PROPAGATE(socket_ports_initialize(&tcp_globals.sockets));
	if(ERROR_OCCURRED(packet_dimensions_initialize(&tcp_globals.dimensions))){
		socket_ports_destroy(&tcp_globals.sockets);
		return ERROR_CODE;
	}
	tcp_globals.last_used_port = TCP_FREE_PORTS_START - 1;
	fibril_rwlock_write_unlock(&tcp_globals.lock);
	return EOK;
}

int tcp_received_msg(device_id_t device_id, packet_t packet, services_t receiver, services_t error){
	ERROR_DECLARE;

	if(receiver != SERVICE_TCP){
		return EREFUSED;
	}
	fibril_rwlock_write_lock(&tcp_globals.lock);
	if(ERROR_OCCURRED(tcp_process_packet(device_id, packet, error))){
		fibril_rwlock_write_unlock(&tcp_globals.lock);
	}
	printf("receive %d \n", ERROR_CODE);

	return ERROR_CODE;
}

int tcp_process_packet(device_id_t device_id, packet_t packet, services_t error){
	ERROR_DECLARE;

	size_t length;
	size_t offset;
	int result;
	tcp_header_ref header;
	socket_core_ref  socket;
	tcp_socket_data_ref socket_data;
	packet_t next_packet;
	size_t total_length;
	uint32_t checksum;
	int fragments;
	icmp_type_t type;
	icmp_code_t code;
	struct sockaddr * src;
	struct sockaddr * dest;
	size_t addrlen;

	printf("p1 \n");
	if(error){
		switch(error){
			case SERVICE_ICMP:
				// process error
				result = icmp_client_process_packet(packet, &type, &code, NULL, NULL);
				if(result < 0){
					return tcp_release_and_return(packet, result);
				}
				length = (size_t) result;
				if(ERROR_OCCURRED(packet_trim(packet, length, 0))){
					return tcp_release_and_return(packet, ERROR_CODE);
				}
				break;
			default:
				return tcp_release_and_return(packet, ENOTSUP);
		}
	}

	// TODO process received ipopts?
	result = ip_client_process_packet(packet, NULL, NULL, NULL, NULL, NULL);
//	printf("ip len %d\n", result);
	if(result < 0){
		return tcp_release_and_return(packet, result);
	}
	offset = (size_t) result;

	length = packet_get_data_length(packet);
//	printf("packet len %d\n", length);
	if(length <= 0){
		return tcp_release_and_return(packet, EINVAL);
	}
	if(length < TCP_HEADER_SIZE + offset){
		return tcp_release_and_return(packet, NO_DATA);
	}

	// trim all but TCP header
	if(ERROR_OCCURRED(packet_trim(packet, offset, 0))){
		return tcp_release_and_return(packet, ERROR_CODE);
	}

	// get tcp header
	header = (tcp_header_ref) packet_get_data(packet);
	if(! header){
		return tcp_release_and_return(packet, NO_DATA);
	}
//	printf("header len %d, port %d \n", TCP_HEADER_LENGTH(header), ntohs(header->destination_port));

	result = packet_get_addr(packet, (uint8_t **) &src, (uint8_t **) &dest);
	if(result <= 0){
		return tcp_release_and_return(packet, result);
	}
	addrlen = (size_t) result;

	if(ERROR_OCCURRED(tl_set_address_port(src, addrlen, ntohs(header->source_port)))){
		return tcp_release_and_return(packet, ERROR_CODE);
	}

	// find the destination socket
	socket = socket_port_find(&tcp_globals.sockets, ntohs(header->destination_port), (const char *) src, addrlen);
	if(! socket){
//		printf("listening?\n");
		// find the listening destination socket
		socket = socket_port_find(&tcp_globals.sockets, ntohs(header->destination_port), SOCKET_MAP_KEY_LISTENING, 0);
		if(! socket){
			if(tl_prepare_icmp_packet(tcp_globals.net_phone, tcp_globals.icmp_phone, packet, error) == EOK){
				icmp_destination_unreachable_msg(tcp_globals.icmp_phone, ICMP_PORT_UNREACH, 0, packet);
			}
			return EADDRNOTAVAIL;
		}
	}
	printf("socket id %d\n", socket->socket_id);
	socket_data = (tcp_socket_data_ref) socket->specific_data;
	assert(socket_data);

	// some data received, clear the timeout counter
	socket_data->timeout_count = 0;

	// count the received packet fragments
	next_packet = packet;
	fragments = 0;
	checksum = 0;
	total_length = 0;
	do{
		++ fragments;
		length = packet_get_data_length(next_packet);
		if(length <= 0){
			return tcp_release_and_return(packet, NO_DATA);
		}
		total_length += length;
		// add partial checksum if set
		if(! error){
			checksum = compute_checksum(checksum, packet_get_data(packet), packet_get_data_length(packet));
		}
	}while((next_packet = pq_next(next_packet)));
//	printf("fragments %d of %d bytes\n", fragments, total_length);

//	printf("lock?\n");
	fibril_rwlock_write_lock(socket_data->local_lock);
//	printf("locked\n");
	if(! error){
		if(socket_data->state == TCP_SOCKET_LISTEN){
			if(socket_data->pseudo_header){
				free(socket_data->pseudo_header);
				socket_data->pseudo_header = NULL;
				socket_data->headerlen = 0;
			}
			if(ERROR_OCCURRED(ip_client_get_pseudo_header(IPPROTO_TCP, src, addrlen, dest, addrlen, total_length, &socket_data->pseudo_header, &socket_data->headerlen))){
				fibril_rwlock_write_unlock(socket_data->local_lock);
				return tcp_release_and_return(packet, ERROR_CODE);
			}
		}else if(ERROR_OCCURRED(ip_client_set_pseudo_header_data_length(socket_data->pseudo_header, socket_data->headerlen, total_length))){
			fibril_rwlock_write_unlock(socket_data->local_lock);
			return tcp_release_and_return(packet, ERROR_CODE);
		}
		checksum = compute_checksum(checksum, socket_data->pseudo_header, socket_data->headerlen);
		if(flip_checksum(compact_checksum(checksum)) != IP_CHECKSUM_ZERO){
			printf("checksum err %x -> %x\n", header->checksum, flip_checksum(compact_checksum(checksum)));
			fibril_rwlock_write_unlock(socket_data->local_lock);
			if(! ERROR_OCCURRED(tl_prepare_icmp_packet(tcp_globals.net_phone, tcp_globals.icmp_phone, packet, error))){
				// checksum error ICMP
				icmp_parameter_problem_msg(tcp_globals.icmp_phone, ICMP_PARAM_POINTER, ((size_t) ((void *) &header->checksum)) - ((size_t) ((void *) header)), packet);
			}
			return EINVAL;
		}
	}

	fibril_rwlock_read_unlock(&tcp_globals.lock);

	// TODO error reporting/handling
//	printf("st %d\n", socket_data->state);
	switch(socket_data->state){
		case TCP_SOCKET_LISTEN:
			ERROR_CODE = tcp_process_listen(socket, socket_data, header, packet, src, dest, addrlen);
			break;
		case TCP_SOCKET_SYN_RECEIVED:
			ERROR_CODE = tcp_process_syn_received(socket, socket_data, header, packet);
			break;
		case TCP_SOCKET_SYN_SENT:
			ERROR_CODE = tcp_process_syn_sent(socket, socket_data, header, packet);
			break;
		case TCP_SOCKET_FIN_WAIT_1:
			// ack changing the state to FIN_WAIT_2 gets processed later
		case TCP_SOCKET_FIN_WAIT_2:
			// fin changing state to LAST_ACK gets processed later
		case TCP_SOCKET_LAST_ACK:
			// ack releasing the socket get processed later
		case TCP_SOCKET_CLOSING:
			// ack releasing the socket gets processed later
		case TCP_SOCKET_ESTABLISHED:
			ERROR_CODE = tcp_process_established(socket, socket_data, header, packet, fragments, total_length);
			break;
		default:
			pq_release(tcp_globals.net_phone, packet_get_id(packet));
	}

	if(ERROR_CODE != EOK){
		printf("process %d\n", ERROR_CODE);
		fibril_rwlock_write_unlock(socket_data->local_lock);
	}
	return EOK;
}

int tcp_process_established(socket_core_ref socket, tcp_socket_data_ref socket_data, tcp_header_ref header, packet_t packet, int fragments, size_t total_length){
	ERROR_DECLARE;

	packet_t next_packet;
	packet_t tmp_packet;
	uint32_t old_incoming;
	size_t order;
	uint32_t sequence_number;
	size_t length;
	size_t offset;
	uint32_t new_sequence_number;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(header);
	assert(packet);

	new_sequence_number = ntohl(header->sequence_number);
	old_incoming = socket_data->next_incoming;

	if(header->finalize){
		socket_data->fin_incoming = new_sequence_number;
	}

//	printf("pe %d < %d <= %d\n", new_sequence_number, socket_data->next_incoming, new_sequence_number + total_length);
	// trim begining if containing expected data
	if(IS_IN_INTERVAL_OVERFLOW(new_sequence_number, socket_data->next_incoming, new_sequence_number + total_length)){
		// get the acknowledged offset
		if(socket_data->next_incoming < new_sequence_number){
			offset = new_sequence_number - socket_data->next_incoming;
		}else{
			offset = socket_data->next_incoming - new_sequence_number;
		}
//		printf("offset %d\n", offset);
		new_sequence_number += offset;
		total_length -= offset;
		length = packet_get_data_length(packet);
		// trim the acknowledged data
		while(length <= offset){
			// release the acknowledged packets
			next_packet = pq_next(packet);
			pq_release(tcp_globals.net_phone, packet_get_id(packet));
			packet = next_packet;
			offset -= length;
			length = packet_get_data_length(packet);
		}
		if((offset > 0)
			&& (ERROR_OCCURRED(packet_trim(packet, offset, 0)))){
			return tcp_release_and_return(packet, ERROR_CODE);
		}
		assert(new_sequence_number == socket_data->next_incoming);
	}

	// release if overflowing the window
//	if(IS_IN_INTERVAL_OVERFLOW(socket_data->next_incoming + socket_data->window, new_sequence_number, new_sequence_number + total_length)){
//		return tcp_release_and_return(packet, EOVERFLOW);
//	}

/*
	// trim end if overflowing the window
	if(IS_IN_INTERVAL_OVERFLOW(new_sequence_number, socket_data->next_incoming + socket_data->window, new_sequence_number + total_length)){
		// get the allowed data length
		if(socket_data->next_incoming + socket_data->window < new_sequence_number){
			offset = new_sequence_number - socket_data->next_incoming + socket_data->window;
		}else{
			offset = socket_data->next_incoming + socket_data->window - new_sequence_number;
		}
		next_packet = packet;
		// trim the overflowing data
		while(next_packet && (offset > 0)){
			length = packet_get_data_length(packet);
			if(length <= offset){
				next_packet = pq_next(next_packet);
			}else if(ERROR_OCCURRED(packet_trim(next_packet, 0, length - offset))){
				return tcp_release_and_return(packet, ERROR_CODE);
			}
			offset -= length;
			total_length -= length - offset;
		}
		// release the overflowing packets
		next_packet = pq_next(next_packet);
		if(next_packet){
			tmp_packet = next_packet;
			next_packet = pq_next(next_packet);
			pq_insert_after(tmp_packet, next_packet);
			pq_release(tcp_globals.net_phone, packet_get_id(tmp_packet));
		}
		assert(new_sequence_number + total_length == socket_data->next_incoming + socket_data->window);
	}
*/
	// the expected one arrived?
	if(new_sequence_number == socket_data->next_incoming){
		printf("expected\n");
		// process acknowledgement
		tcp_process_acknowledgement(socket, socket_data, header);

		// remove the header
		total_length -= TCP_HEADER_LENGTH(header);
		if(ERROR_OCCURRED(packet_trim(packet, TCP_HEADER_LENGTH(header), 0))){
			return tcp_release_and_return(packet, ERROR_CODE);
		}

		if(total_length){
			ERROR_PROPAGATE(tcp_queue_received_packet(socket, socket_data, packet, fragments, total_length));
		}else{
			total_length = 1;
		}
		socket_data->next_incoming = old_incoming + total_length;
		packet = socket_data->incoming;
		while(packet){
			if(ERROR_OCCURRED(pq_get_order(socket_data->incoming, &order, NULL))){
				// remove the corrupted packet
				next_packet = pq_detach(packet);
				if(packet == socket_data->incoming){
					socket_data->incoming = next_packet;
				}
				pq_release(tcp_globals.net_phone, packet_get_id(packet));
				packet = next_packet;
				continue;
			}
			sequence_number = (uint32_t) order;
			if(IS_IN_INTERVAL_OVERFLOW(sequence_number, old_incoming, socket_data->next_incoming)){
				// move to the next
				packet = pq_next(packet);
			// coninual data?
			}else if(IS_IN_INTERVAL_OVERFLOW(old_incoming, sequence_number, socket_data->next_incoming)){
				// detach the packet
				next_packet = pq_detach(packet);
				if(packet == socket_data->incoming){
					socket_data->incoming = next_packet;
				}
				// get data length
				length = packet_get_data_length(packet);
				new_sequence_number = sequence_number + length;
				if(length <= 0){
					// remove the empty packet
					pq_release(tcp_globals.net_phone, packet_get_id(packet));
					packet = next_packet;
					continue;
				}
				// exactly following
				if(sequence_number == socket_data->next_incoming){
					// queue received data
					ERROR_PROPAGATE(tcp_queue_received_packet(socket, socket_data, packet, 1, packet_get_data_length(packet)));
					socket_data->next_incoming = new_sequence_number;
					packet = next_packet;
					continue;
				// at least partly following data?
				}else if(IS_IN_INTERVAL_OVERFLOW(sequence_number, socket_data->next_incoming, new_sequence_number)){
					if(socket_data->next_incoming < new_sequence_number){
						length = new_sequence_number - socket_data->next_incoming;
					}else{
						length = socket_data->next_incoming - new_sequence_number;
					}
					if(! ERROR_OCCURRED(packet_trim(packet, length, 0))){
						// queue received data
						ERROR_PROPAGATE(tcp_queue_received_packet(socket, socket_data, packet, 1, packet_get_data_length(packet)));
						socket_data->next_incoming = new_sequence_number;
						packet = next_packet;
						continue;
					}
				}
				// remove the duplicit or corrupted packet
				pq_release(tcp_globals.net_phone, packet_get_id(packet));
				packet = next_packet;
				continue;
			}else{
				break;
			}
		}
	}else if(IS_IN_INTERVAL(socket_data->next_incoming, new_sequence_number, socket_data->next_incoming + socket_data->window)){
		printf("in window\n");
		// process acknowledgement
		tcp_process_acknowledgement(socket, socket_data, header);

		// remove the header
		total_length -= TCP_HEADER_LENGTH(header);
		if(ERROR_OCCURRED(packet_trim(packet, TCP_HEADER_LENGTH(header), 0))){
			return tcp_release_and_return(packet, ERROR_CODE);
		}

		next_packet = pq_detach(packet);
		length = packet_get_data_length(packet);
		if(ERROR_OCCURRED(pq_add(&socket_data->incoming, packet, new_sequence_number, length))){
			// remove the corrupted packets
			pq_release(tcp_globals.net_phone, packet_get_id(packet));
			pq_release(tcp_globals.net_phone, packet_get_id(next_packet));
		}else{
			while(next_packet){
				new_sequence_number += length;
				tmp_packet = pq_detach(next_packet);
				length = packet_get_data_length(next_packet);
				if(ERROR_OCCURRED(pq_set_order(next_packet, new_sequence_number, length))
					|| ERROR_OCCURRED(pq_insert_after(packet, next_packet))){
					pq_release(tcp_globals.net_phone, packet_get_id(next_packet));
				}
				next_packet = tmp_packet;
			}
		}
	}else{
		printf("unexpected\n");
		// release duplicite or restricted
		pq_release(tcp_globals.net_phone, packet_get_id(packet));
	}

	// change state according to the acknowledging incoming fin
	if(IS_IN_INTERVAL_OVERFLOW(old_incoming, socket_data->fin_incoming, socket_data->next_incoming)){
		switch(socket_data->state){
			case TCP_SOCKET_FIN_WAIT_1:
			case TCP_SOCKET_FIN_WAIT_2:
			case TCP_SOCKET_CLOSING:
				socket_data->state = TCP_SOCKET_CLOSING;
				break;
			//case TCP_ESTABLISHED:
			default:
				socket_data->state = TCP_SOCKET_CLOSE_WAIT;
				break;
		}
	}

	packet = tcp_get_packets_to_send(socket, socket_data);
	if(! packet){
		// create the notification packet
		ERROR_PROPAGATE(tcp_create_notification_packet(&packet, socket, socket_data, 0, 0));
		ERROR_PROPAGATE(tcp_queue_prepare_packet(socket, socket_data, packet, 1));
		packet = tcp_send_prepare_packet(socket, socket_data, packet, 1, socket_data->last_outgoing + 1);
	}
	fibril_rwlock_write_unlock(socket_data->local_lock);
	// send the packet
	tcp_send_packets(socket_data->device_id, packet);
	return EOK;
}

int tcp_queue_received_packet(socket_core_ref socket, tcp_socket_data_ref socket_data, packet_t packet, int fragments, size_t total_length){
	ERROR_DECLARE;

	packet_dimension_ref packet_dimension;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(packet);
	assert(fragments >= 1);
	assert(socket_data->window > total_length);

	// queue the received packet
	if(ERROR_OCCURRED(dyn_fifo_push(&socket->received, packet_get_id(packet), SOCKET_MAX_RECEIVED_SIZE))
		|| ERROR_OCCURRED(tl_get_ip_packet_dimension(tcp_globals.ip_phone, &tcp_globals.dimensions, socket_data->device_id, &packet_dimension))){
		return tcp_release_and_return(packet, ERROR_CODE);
	}

	// decrease the window size
	socket_data->window -= total_length;

	// notify the destination socket
	async_msg_5(socket->phone, NET_SOCKET_RECEIVED, (ipcarg_t) socket->socket_id, ((packet_dimension->content < socket_data->data_fragment_size) ? packet_dimension->content : socket_data->data_fragment_size), 0, 0, (ipcarg_t) fragments);
	return EOK;
}

int tcp_process_syn_sent(socket_core_ref socket, tcp_socket_data_ref socket_data, tcp_header_ref header, packet_t packet){
	ERROR_DECLARE;

	packet_t next_packet;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(header);
	assert(packet);

	if(header->synchronize){
		// process acknowledgement
		tcp_process_acknowledgement(socket, socket_data, header);

		socket_data->next_incoming = ntohl(header->sequence_number) + 1;
		// release additional packets
		next_packet = pq_detach(packet);
		if(next_packet){
			pq_release(tcp_globals.net_phone, packet_get_id(next_packet));
		}
		// trim if longer than the header
		if((packet_get_data_length(packet) > sizeof(*header))
			&& ERROR_OCCURRED(packet_trim(packet, 0, packet_get_data_length(packet) - sizeof(*header)))){
			return tcp_release_and_return(packet, ERROR_CODE);
		}
		tcp_prepare_operation_header(socket, socket_data, header, 0, 0);
		fibril_mutex_lock(&socket_data->operation.mutex);
		socket_data->operation.result = tcp_queue_packet(socket, socket_data, packet, 1);
		if(socket_data->operation.result == EOK){
			socket_data->state = TCP_SOCKET_ESTABLISHED;
			packet = tcp_get_packets_to_send(socket, socket_data);
			if(packet){
				fibril_rwlock_write_unlock(socket_data->local_lock);
				// send the packet
				tcp_send_packets(socket_data->device_id, packet);
				// signal the result
				fibril_condvar_signal(&socket_data->operation.condvar);
				fibril_mutex_unlock(&socket_data->operation.mutex);
				return EOK;
			}
		}
		fibril_mutex_unlock(&socket_data->operation.mutex);
	}
	return tcp_release_and_return(packet, EINVAL);
}

int tcp_process_listen(socket_core_ref listening_socket, tcp_socket_data_ref listening_socket_data, tcp_header_ref header, packet_t packet, struct sockaddr * src, struct sockaddr * dest, size_t addrlen){
	ERROR_DECLARE;

	packet_t next_packet;
	socket_core_ref socket;
	tcp_socket_data_ref socket_data;
	int socket_id;
	int listening_socket_id = listening_socket->socket_id;
	int listening_port = listening_socket->port;

	assert(listening_socket);
	assert(listening_socket_data);
	assert(listening_socket->specific_data == listening_socket_data);
	assert(header);
	assert(packet);

//	printf("syn %d\n", header->synchronize);
	if(header->synchronize){
		socket_data = (tcp_socket_data_ref) malloc(sizeof(*socket_data));
		if(! socket_data){
			return tcp_release_and_return(packet, ENOMEM);
		}else{
			tcp_initialize_socket_data(socket_data);
			socket_data->local_lock = listening_socket_data->local_lock;
			socket_data->local_sockets = listening_socket_data->local_sockets;
			socket_data->listening_socket_id = listening_socket->socket_id;

			socket_data->next_incoming = ntohl(header->sequence_number);
			socket_data->treshold = socket_data->next_incoming + ntohs(header->window);

			socket_data->addrlen = addrlen;
			socket_data->addr = malloc(socket_data->addrlen);
			if(! socket_data->addr){
				free(socket_data);
				return tcp_release_and_return(packet, ENOMEM);
			}
			memcpy(socket_data->addr, src, socket_data->addrlen);

			socket_data->dest_port = ntohs(header->source_port);
			if(ERROR_OCCURRED(tl_set_address_port(socket_data->addr, socket_data->addrlen, socket_data->dest_port))){
				free(socket_data->addr);
				free(socket_data);
				pq_release(tcp_globals.net_phone, packet_get_id(packet));
				return ERROR_CODE;
			}

//			printf("addr %p\n", socket_data->addr, socket_data->addrlen);
			// create a socket
			socket_id = -1;
			if(ERROR_OCCURRED(socket_create(socket_data->local_sockets, listening_socket->phone, socket_data, &socket_id))){
				free(socket_data->addr);
				free(socket_data);
				return tcp_release_and_return(packet, ERROR_CODE);
			}

			printf("new_sock %d\n", socket_id);
			socket_data->pseudo_header = listening_socket_data->pseudo_header;
			socket_data->headerlen = listening_socket_data->headerlen;
			listening_socket_data->pseudo_header = NULL;
			listening_socket_data->headerlen = 0;

			fibril_rwlock_write_unlock(socket_data->local_lock);
//			printf("list lg\n");
			fibril_rwlock_write_lock(&tcp_globals.lock);
//			printf("list locked\n");
			// find the destination socket
			listening_socket = socket_port_find(&tcp_globals.sockets, listening_port, SOCKET_MAP_KEY_LISTENING, 0);
			if((! listening_socket) || (listening_socket->socket_id != listening_socket_id)){
				fibril_rwlock_write_unlock(&tcp_globals.lock);
				// a shadow may remain until app hangs up
				return tcp_release_and_return(packet, EOK/*ENOTSOCK*/);
			}
//			printf("port %d\n", listening_socket->port);
			listening_socket_data = (tcp_socket_data_ref) listening_socket->specific_data;
			assert(listening_socket_data);

//			printf("list ll\n");
			fibril_rwlock_write_lock(listening_socket_data->local_lock);
//			printf("list locked\n");

			socket = socket_cores_find(listening_socket_data->local_sockets, socket_id);
			if(! socket){
				// where is the socket?!?
				fibril_rwlock_write_unlock(&tcp_globals.lock);
				return ENOTSOCK;
			}
			socket_data = (tcp_socket_data_ref) socket->specific_data;
			assert(socket_data);

//			uint8_t * data = socket_data->addr;
//			printf("addr %d of %x %x %x %x-%x %x %x %x-%x %x %x %x-%x %x %x %x\n", socket_data->addrlen, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

			ERROR_CODE = socket_port_add(&tcp_globals.sockets, listening_port, socket, (const char *) socket_data->addr, socket_data->addrlen);
			assert(socket == socket_port_find(&tcp_globals.sockets, listening_port, (const char *) socket_data->addr, socket_data->addrlen));
			//ERROR_CODE = socket_bind_free_port(&tcp_globals.sockets, socket, TCP_FREE_PORTS_START, TCP_FREE_PORTS_END, tcp_globals.last_used_port);
			//tcp_globals.last_used_port = socket->port;
//			printf("bound %d\n", socket->port);
			fibril_rwlock_write_unlock(&tcp_globals.lock);
			if(ERROR_CODE != EOK){
				socket_destroy(tcp_globals.net_phone, socket->socket_id, socket_data->local_sockets, &tcp_globals.sockets, tcp_free_socket_data);
				return tcp_release_and_return(packet, ERROR_CODE);
			}

			socket_data->state = TCP_SOCKET_LISTEN;
			socket_data->next_incoming = ntohl(header->sequence_number) + 1;
			// release additional packets
			next_packet = pq_detach(packet);
			if(next_packet){
				pq_release(tcp_globals.net_phone, packet_get_id(next_packet));
			}
			// trim if longer than the header
			if((packet_get_data_length(packet) > sizeof(*header))
				&& ERROR_OCCURRED(packet_trim(packet, 0, packet_get_data_length(packet) - sizeof(*header)))){
				socket_destroy(tcp_globals.net_phone, socket->socket_id, socket_data->local_sockets, &tcp_globals.sockets, tcp_free_socket_data);
				return tcp_release_and_return(packet, ERROR_CODE);
			}
			tcp_prepare_operation_header(socket, socket_data, header, 1, 0);
			if(ERROR_OCCURRED(tcp_queue_packet(socket, socket_data, packet, 1))){
				socket_destroy(tcp_globals.net_phone, socket->socket_id, socket_data->local_sockets, &tcp_globals.sockets, tcp_free_socket_data);
				return ERROR_CODE;
			}
			packet = tcp_get_packets_to_send(socket, socket_data);
//			printf("send %d\n", packet_get_id(packet));
			if(! packet){
				socket_destroy(tcp_globals.net_phone, socket->socket_id, socket_data->local_sockets, &tcp_globals.sockets, tcp_free_socket_data);
				return EINVAL;
			}else{
				socket_data->state = TCP_SOCKET_SYN_RECEIVED;
//				printf("unlock\n");
				fibril_rwlock_write_unlock(socket_data->local_lock);
				// send the packet
				tcp_send_packets(socket_data->device_id, packet);
				return EOK;
			}
		}
	}
	return tcp_release_and_return(packet, EINVAL);
}

int tcp_process_syn_received(socket_core_ref socket, tcp_socket_data_ref socket_data, tcp_header_ref header, packet_t packet){
	ERROR_DECLARE;

	socket_core_ref listening_socket;
	tcp_socket_data_ref listening_socket_data;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(header);
	assert(packet);

	printf("syn_rec\n");
	if(header->acknowledge){
		// process acknowledgement
		tcp_process_acknowledgement(socket, socket_data, header);

		socket_data->next_incoming = ntohl(header->sequence_number);// + 1;
		pq_release(tcp_globals.net_phone, packet_get_id(packet));
		socket_data->state = TCP_SOCKET_ESTABLISHED;
		listening_socket = socket_cores_find(socket_data->local_sockets, socket_data->listening_socket_id);
		if(listening_socket){
			listening_socket_data = (tcp_socket_data_ref) listening_socket->specific_data;
			assert(listening_socket_data);

			// queue the received packet
			if(! ERROR_OCCURRED(dyn_fifo_push(&listening_socket->accepted, (-1 * socket->socket_id), listening_socket_data->backlog))){
				// notify the destination socket
				async_msg_5(socket->phone, NET_SOCKET_ACCEPTED, (ipcarg_t) listening_socket->socket_id, socket_data->data_fragment_size, TCP_HEADER_SIZE, 0, (ipcarg_t) socket->socket_id);
				fibril_rwlock_write_unlock(socket_data->local_lock);
				return EOK;
			}
		}
		// send FIN
		socket_data->state = TCP_SOCKET_FIN_WAIT_1;

		// create the notification packet
		ERROR_PROPAGATE(tcp_create_notification_packet(&packet, socket, socket_data, 0, 1));

		// send the packet
		ERROR_PROPAGATE(tcp_queue_packet(socket, socket_data, packet, 1));

		// flush packets
		packet = tcp_get_packets_to_send(socket, socket_data);
		fibril_rwlock_write_unlock(socket_data->local_lock);
		if(packet){
			// send the packet
			tcp_send_packets(socket_data->device_id, packet);
		}
		return EOK;
	}else{
		return tcp_release_and_return(packet, EINVAL);
	}
	return EINVAL;
}

void tcp_process_acknowledgement(socket_core_ref socket, tcp_socket_data_ref socket_data, tcp_header_ref header){
	size_t number;
	size_t length;
	packet_t packet;
	packet_t next;
	packet_t acknowledged = NULL;
	uint32_t old;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(header);

	if(header->acknowledge){
		number = ntohl(header->acknowledgement_number);
		// if more data acknowledged
		if(number != socket_data->expected){
			old = socket_data->expected;
			if(IS_IN_INTERVAL_OVERFLOW(old, socket_data->fin_outgoing, number)){
				switch(socket_data->state){
					case TCP_SOCKET_FIN_WAIT_1:
						socket_data->state = TCP_SOCKET_FIN_WAIT_2;
						break;
					case TCP_SOCKET_LAST_ACK:
					case TCP_SOCKET_CLOSING:
						// fin acknowledged - release the socket in another fibril
						tcp_prepare_timeout(tcp_release_after_timeout, socket, socket_data, 0, TCP_SOCKET_TIME_WAIT, NET_DEFAULT_TCP_TIME_WAIT_TIMEOUT, true);
						break;
					default:
						break;
				}
			}
			// update the treshold if higher than set
			if(number + ntohs(header->window) > socket_data->expected + socket_data->treshold){
				socket_data->treshold = number + ntohs(header->window) - socket_data->expected;
			}
			// set new expected sequence number
			socket_data->expected = number;
			socket_data->expected_count = 1;
			packet = socket_data->outgoing;
			while(pq_get_order(packet, &number, &length) == EOK){
				if(IS_IN_INTERVAL_OVERFLOW((uint32_t) old, (uint32_t)(number + length), (uint32_t) socket_data->expected)){
					next = pq_detach(packet);
					if(packet == socket_data->outgoing){
						socket_data->outgoing = next;
					}
					// add to acknowledged or release
					if(pq_add(&acknowledged, packet, 0, 0) != EOK){
						pq_release(tcp_globals.net_phone, packet_get_id(packet));
					}
					packet = next;
				}else if(old < socket_data->expected){
					break;
				}
			}
			// release acknowledged
			if(acknowledged){
				pq_release(tcp_globals.net_phone, packet_get_id(acknowledged));
			}
			return;
		// if the same as the previous time
		}else if(number == socket_data->expected){
			// increase the counter
			++ socket_data->expected_count;
			if(socket_data->expected_count == TCP_FAST_RETRANSMIT_COUNT){
				socket_data->expected_count = 1;
				// TODO retransmit lock
				//tcp_retransmit_packet(socket, socket_data, number);
			}
		}
	}
}

int tcp_message(ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count){
	ERROR_DECLARE;

	packet_t packet;

	assert(call);
	assert(answer);
	assert(answer_count);

	*answer_count = 0;
	switch(IPC_GET_METHOD(*call)){
		case NET_TL_RECEIVED:
			//fibril_rwlock_read_lock(&tcp_globals.lock);
			if(! ERROR_OCCURRED(packet_translate(tcp_globals.net_phone, &packet, IPC_GET_PACKET(call)))){
				ERROR_CODE = tcp_received_msg(IPC_GET_DEVICE(call), packet, SERVICE_TCP, IPC_GET_ERROR(call));
			}
			//fibril_rwlock_read_unlock(&tcp_globals.lock);
			return ERROR_CODE;
		case IPC_M_CONNECT_TO_ME:
			return tcp_process_client_messages(callid, * call);
	}
	return ENOTSUP;
}

void tcp_refresh_socket_data(tcp_socket_data_ref socket_data){
	assert(socket_data);

	bzero(socket_data, sizeof(*socket_data));
	socket_data->state = TCP_SOCKET_INITIAL;
	socket_data->device_id = DEVICE_INVALID_ID;
	socket_data->window = NET_DEFAULT_TCP_WINDOW;
	socket_data->treshold = socket_data->window;
	socket_data->last_outgoing = TCP_INITIAL_SEQUENCE_NUMBER;
	socket_data->timeout = NET_DEFAULT_TCP_INITIAL_TIMEOUT;
	socket_data->acknowledged = socket_data->last_outgoing;
	socket_data->next_outgoing = socket_data->last_outgoing + 1;
	socket_data->expected = socket_data->next_outgoing;
}

void tcp_initialize_socket_data(tcp_socket_data_ref socket_data){
	assert(socket_data);

	tcp_refresh_socket_data(socket_data);
	fibril_mutex_initialize(&socket_data->operation.mutex);
	fibril_condvar_initialize(&socket_data->operation.condvar);
	socket_data->data_fragment_size = MAX_TCP_FRAGMENT_SIZE;
}

int tcp_process_client_messages(ipc_callid_t callid, ipc_call_t call){
	int res;
	bool keep_on_going = true;
	socket_cores_t local_sockets;
	int app_phone = IPC_GET_PHONE(&call);
	struct sockaddr * addr;
	size_t addrlen;
	fibril_rwlock_t lock;
	ipc_call_t answer;
	int answer_count;
	tcp_socket_data_ref socket_data;
	socket_core_ref socket;
	packet_dimension_ref packet_dimension;

	/*
	 * Accept the connection
	 *  - Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	res = EOK;
	answer_count = 0;

	socket_cores_initialize(&local_sockets);
	fibril_rwlock_initialize(&lock);

	while(keep_on_going){

		// answer the call
		answer_call(callid, res, &answer, answer_count);

		// refresh data
		refresh_answer(&answer, &answer_count);

		// get the next call
		callid = async_get_call(&call);

		// process the call
		switch(IPC_GET_METHOD(call)){
			case IPC_M_PHONE_HUNGUP:
				keep_on_going = false;
				res = EHANGUP;
				break;
			case NET_SOCKET:
				socket_data = (tcp_socket_data_ref) malloc(sizeof(*socket_data));
				if(! socket_data){
					res = ENOMEM;
				}else{
					tcp_initialize_socket_data(socket_data);
					socket_data->local_lock = &lock;
					socket_data->local_sockets = &local_sockets;
					fibril_rwlock_write_lock(&lock);
					*SOCKET_SET_SOCKET_ID(answer) = SOCKET_GET_SOCKET_ID(call);
					res = socket_create(&local_sockets, app_phone, socket_data, SOCKET_SET_SOCKET_ID(answer));
					fibril_rwlock_write_unlock(&lock);
					if(res == EOK){
						if(tl_get_ip_packet_dimension(tcp_globals.ip_phone, &tcp_globals.dimensions, DEVICE_INVALID_ID, &packet_dimension) == EOK){
							*SOCKET_SET_DATA_FRAGMENT_SIZE(answer) = ((packet_dimension->content < socket_data->data_fragment_size) ? packet_dimension->content : socket_data->data_fragment_size);
						}
//						*SOCKET_SET_DATA_FRAGMENT_SIZE(answer) = MAX_TCP_FRAGMENT_SIZE;
						*SOCKET_SET_HEADER_SIZE(answer) = TCP_HEADER_SIZE;
						answer_count = 3;
					}else{
						free(socket_data);
					}
				}
				break;
			case NET_SOCKET_BIND:
				res = data_receive((void **) &addr, &addrlen);
				if(res == EOK){
					fibril_rwlock_write_lock(&tcp_globals.lock);
					fibril_rwlock_write_lock(&lock);
					res = socket_bind(&local_sockets, &tcp_globals.sockets, SOCKET_GET_SOCKET_ID(call), addr, addrlen, TCP_FREE_PORTS_START, TCP_FREE_PORTS_END, tcp_globals.last_used_port);
					if(res == EOK){
						socket = socket_cores_find(&local_sockets, SOCKET_GET_SOCKET_ID(call));
						if(socket){
							socket_data = (tcp_socket_data_ref) socket->specific_data;
							assert(socket_data);
							socket_data->state = TCP_SOCKET_LISTEN;
						}
					}
					fibril_rwlock_write_unlock(&lock);
					fibril_rwlock_write_unlock(&tcp_globals.lock);
					free(addr);
				}
				break;
			case NET_SOCKET_LISTEN:
				fibril_rwlock_read_lock(&tcp_globals.lock);
//				fibril_rwlock_write_lock(&tcp_globals.lock);
				fibril_rwlock_write_lock(&lock);
				res = tcp_listen_message(&local_sockets, SOCKET_GET_SOCKET_ID(call), SOCKET_GET_BACKLOG(call));
				fibril_rwlock_write_unlock(&lock);
//				fibril_rwlock_write_unlock(&tcp_globals.lock);
				fibril_rwlock_read_unlock(&tcp_globals.lock);
				break;
			case NET_SOCKET_CONNECT:
				res = data_receive((void **) &addr, &addrlen);
				if(res == EOK){
					// the global lock may be released in the tcp_connect_message() function
					fibril_rwlock_write_lock(&tcp_globals.lock);
					fibril_rwlock_write_lock(&lock);
					res = tcp_connect_message(&local_sockets, SOCKET_GET_SOCKET_ID(call), addr, addrlen);
					if(res != EOK){
						fibril_rwlock_write_unlock(&lock);
						fibril_rwlock_write_unlock(&tcp_globals.lock);
						free(addr);
					}
				}
				break;
			case NET_SOCKET_ACCEPT:
				fibril_rwlock_read_lock(&tcp_globals.lock);
				fibril_rwlock_write_lock(&lock);
				res = tcp_accept_message(&local_sockets, SOCKET_GET_SOCKET_ID(call), SOCKET_GET_NEW_SOCKET_ID(call), SOCKET_SET_DATA_FRAGMENT_SIZE(answer), &addrlen);
				fibril_rwlock_write_unlock(&lock);
				fibril_rwlock_read_unlock(&tcp_globals.lock);
				if(res > 0){
					*SOCKET_SET_SOCKET_ID(answer) = res;
					*SOCKET_SET_ADDRESS_LENGTH(answer) = addrlen;
					answer_count = 3;
				}
				break;
			case NET_SOCKET_SEND:
				fibril_rwlock_read_lock(&tcp_globals.lock);
				fibril_rwlock_write_lock(&lock);
				res = tcp_send_message(&local_sockets, SOCKET_GET_SOCKET_ID(call), SOCKET_GET_DATA_FRAGMENTS(call), SOCKET_SET_DATA_FRAGMENT_SIZE(answer), SOCKET_GET_FLAGS(call));
				if(res != EOK){
					fibril_rwlock_write_unlock(&lock);
					fibril_rwlock_read_unlock(&tcp_globals.lock);
				}else{
					answer_count = 2;
				}
				break;
			case NET_SOCKET_SENDTO:
				res = data_receive((void **) &addr, &addrlen);
				if(res == EOK){
					fibril_rwlock_read_lock(&tcp_globals.lock);
					fibril_rwlock_write_lock(&lock);
					res = tcp_send_message(&local_sockets, SOCKET_GET_SOCKET_ID(call), SOCKET_GET_DATA_FRAGMENTS(call), SOCKET_SET_DATA_FRAGMENT_SIZE(answer), SOCKET_GET_FLAGS(call));
					if(res != EOK){
						fibril_rwlock_write_unlock(&lock);
						fibril_rwlock_read_unlock(&tcp_globals.lock);
					}else{
						answer_count = 2;
					}
					free(addr);
				}
				break;
			case NET_SOCKET_RECV:
				fibril_rwlock_read_lock(&tcp_globals.lock);
				fibril_rwlock_write_lock(&lock);
				res = tcp_recvfrom_message(&local_sockets, SOCKET_GET_SOCKET_ID(call), SOCKET_GET_FLAGS(call), NULL);
				fibril_rwlock_write_unlock(&lock);
				fibril_rwlock_read_unlock(&tcp_globals.lock);
				if(res > 0){
					*SOCKET_SET_READ_DATA_LENGTH(answer) = res;
					answer_count = 1;
					res = EOK;
				}
				break;
			case NET_SOCKET_RECVFROM:
				fibril_rwlock_read_lock(&tcp_globals.lock);
				fibril_rwlock_write_lock(&lock);
				res = tcp_recvfrom_message(&local_sockets, SOCKET_GET_SOCKET_ID(call), SOCKET_GET_FLAGS(call), &addrlen);
				fibril_rwlock_write_unlock(&lock);
				fibril_rwlock_read_unlock(&tcp_globals.lock);
				if(res > 0){
					*SOCKET_SET_READ_DATA_LENGTH(answer) = res;
					*SOCKET_SET_ADDRESS_LENGTH(answer) = addrlen;
					answer_count = 3;
					res = EOK;
				}
				break;
			case NET_SOCKET_CLOSE:
				fibril_rwlock_write_lock(&tcp_globals.lock);
				fibril_rwlock_write_lock(&lock);
				res = tcp_close_message(&local_sockets, SOCKET_GET_SOCKET_ID(call));
				if(res != EOK){
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

	// release the application phone
	ipc_hangup(app_phone);

	printf("release\n");
	// release all local sockets
	socket_cores_release(tcp_globals.net_phone, &local_sockets, &tcp_globals.sockets, tcp_free_socket_data);

	return EOK;
}

int tcp_timeout(void * data){
	tcp_timeout_ref timeout = data;
	int keep_write_lock = false;
	socket_core_ref socket;
	tcp_socket_data_ref socket_data;

	assert(timeout);

	// sleep the given timeout
	async_usleep(timeout->timeout);
	// lock the globals
	if(timeout->globals_read_only){
		fibril_rwlock_read_lock(&tcp_globals.lock);
	}else{
		fibril_rwlock_write_lock(&tcp_globals.lock);
	}
	// find the pending operation socket
	socket = socket_port_find(&tcp_globals.sockets, timeout->port, timeout->key, timeout->key_length);
	if(socket && (socket->socket_id == timeout->socket_id)){
		socket_data = (tcp_socket_data_ref) socket->specific_data;
		assert(socket_data);
		if(socket_data->local_sockets == timeout->local_sockets){
			fibril_rwlock_write_lock(socket_data->local_lock);
			if(timeout->sequence_number){
				// increase the timeout counter;
				++ socket_data->timeout_count;
				if(socket_data->timeout_count == TCP_MAX_TIMEOUTS){
					// TODO release as connection lost
					//tcp_refresh_socket_data(socket_data);
					fibril_rwlock_write_unlock(socket_data->local_lock);
				}else{
					// retransmit
//					tcp_retransmit_packet(socket, socket_data, timeout->sequence_number);
					fibril_rwlock_write_unlock(socket_data->local_lock);
				}
			}else{
				fibril_mutex_lock(&socket_data->operation.mutex);
				// set the timeout operation result if state not changed
				if(socket_data->state == timeout->state){
					socket_data->operation.result = ETIMEOUT;
					// notify the main fibril
					fibril_condvar_signal(&socket_data->operation.condvar);
					// keep the global write lock
					keep_write_lock = true;
				}else{
					// operation is ok, do nothing
					// unlocking from now on, so the unlock order does not matter...
					fibril_rwlock_write_unlock(socket_data->local_lock);
				}
				fibril_mutex_unlock(&socket_data->operation.mutex);
			}
		}
	}
	// unlock only if no socket
	if(timeout->globals_read_only){
		fibril_rwlock_read_unlock(&tcp_globals.lock);
	}else if(! keep_write_lock){
		// release if not desired
		fibril_rwlock_write_unlock(&tcp_globals.lock);
	}
	// release the timeout structure
	free(timeout);
	return EOK;
}

int tcp_release_after_timeout(void * data){
	tcp_timeout_ref timeout = data;
	socket_core_ref socket;
	tcp_socket_data_ref socket_data;
	fibril_rwlock_t * local_lock;

	assert(timeout);

	// sleep the given timeout
	async_usleep(timeout->timeout);
	// lock the globals
	fibril_rwlock_write_lock(&tcp_globals.lock);
	// find the pending operation socket
	socket = socket_port_find(&tcp_globals.sockets, timeout->port, timeout->key, timeout->key_length);
	if(socket && (socket->socket_id == timeout->socket_id)){
		socket_data = (tcp_socket_data_ref) socket->specific_data;
		assert(socket_data);
		if(socket_data->local_sockets == timeout->local_sockets){
			local_lock = socket_data->local_lock;
			fibril_rwlock_write_lock(local_lock);
			socket_destroy(tcp_globals.net_phone, timeout->socket_id, timeout->local_sockets, &tcp_globals.sockets, tcp_free_socket_data);
			fibril_rwlock_write_unlock(local_lock);
		}
	}
	// unlock the globals
	fibril_rwlock_write_unlock(&tcp_globals.lock);
	// release the timeout structure
	free(timeout);
	return EOK;
}

void tcp_retransmit_packet(socket_core_ref socket, tcp_socket_data_ref socket_data, size_t sequence_number){
	packet_t packet;
	packet_t copy;
	size_t data_length;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	// sent packet?
	packet = pq_find(socket_data->outgoing, sequence_number);
	printf("retransmit %d\n", packet_get_id(packet));
	if(packet){
		pq_get_order(packet, NULL, &data_length);
		copy = tcp_prepare_copy(socket, socket_data, packet, data_length, sequence_number);
		fibril_rwlock_write_unlock(socket_data->local_lock);
//		printf("r send %d\n", packet_get_id(packet));
		if(copy){
			tcp_send_packets(socket_data->device_id, copy);
		}
	}else{
		fibril_rwlock_write_unlock(socket_data->local_lock);
	}
}

int tcp_listen_message(socket_cores_ref local_sockets, int socket_id, int backlog){
	socket_core_ref socket;
	tcp_socket_data_ref socket_data;

	assert(local_sockets);

	if(backlog < 0){
		return EINVAL;
	}
	// find the socket
	socket = socket_cores_find(local_sockets, socket_id);
	if(! socket){
		return ENOTSOCK;
	}
	// get the socket specific data
	socket_data = (tcp_socket_data_ref) socket->specific_data;
	assert(socket_data);
	// set the backlog
	socket_data->backlog = backlog;
	return EOK;
}

int tcp_connect_message(socket_cores_ref local_sockets, int socket_id, struct sockaddr * addr, socklen_t addrlen){
	ERROR_DECLARE;

	socket_core_ref socket;

	assert(local_sockets);
	assert(addr);
	assert(addrlen > 0);

	// find the socket
	socket = socket_cores_find(local_sockets, socket_id);
	if(! socket){
		return ENOTSOCK;
	}
	if(ERROR_OCCURRED(tcp_connect_core(socket, local_sockets, addr, addrlen))){
		tcp_free_socket_data(socket);
		// unbind if bound
		if(socket->port > 0){
			socket_ports_exclude(&tcp_globals.sockets, socket->port);
			socket->port = 0;
		}
	}
	return ERROR_CODE;
}

int tcp_connect_core(socket_core_ref socket, socket_cores_ref local_sockets, struct sockaddr * addr, socklen_t addrlen){
	ERROR_DECLARE;

	tcp_socket_data_ref socket_data;
	packet_t packet;

	assert(socket);
	assert(addr);
	assert(addrlen > 0);

	// get the socket specific data
	socket_data = (tcp_socket_data_ref) socket->specific_data;
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	if((socket_data->state != TCP_SOCKET_INITIAL)
		&& ((socket_data->state != TCP_SOCKET_LISTEN) || (socket->port <= 0))){
		return EINVAL;
	}
	// get the destination port
	ERROR_PROPAGATE(tl_get_address_port(addr, addrlen, &socket_data->dest_port));
	if(socket->port <= 0){
		// try to find a free port
		ERROR_PROPAGATE(socket_bind_free_port(&tcp_globals.sockets, socket, TCP_FREE_PORTS_START, TCP_FREE_PORTS_END, tcp_globals.last_used_port));
		// set the next port as the search starting port number
		tcp_globals.last_used_port = socket->port;
	}
	ERROR_PROPAGATE(ip_get_route_req(tcp_globals.ip_phone, IPPROTO_TCP, addr, addrlen, &socket_data->device_id, &socket_data->pseudo_header, &socket_data->headerlen));

	// create the notification packet
	ERROR_PROPAGATE(tcp_create_notification_packet(&packet, socket, socket_data, 1, 0));

	// unlock the globals and wait for an operation
	fibril_rwlock_write_unlock(&tcp_globals.lock);

	socket_data->addr = addr;
	socket_data->addrlen = addrlen;
	// send the packet
	if(ERROR_OCCURRED(tcp_queue_packet(socket, socket_data, packet, 1))
		|| ERROR_OCCURRED(tcp_prepare_timeout(tcp_timeout, socket, socket_data, 0, TCP_SOCKET_INITIAL, NET_DEFAULT_TCP_INITIAL_TIMEOUT, false))){
		socket_data->addr = NULL;
		socket_data->addrlen = 0;
		fibril_rwlock_write_lock(&tcp_globals.lock);
	}else{
		packet = tcp_get_packets_to_send(socket, socket_data);
		if(packet){
			fibril_mutex_lock(&socket_data->operation.mutex);
			fibril_rwlock_write_unlock(socket_data->local_lock);
			// send the packet
			printf("connecting %d\n", packet_get_id(packet));
			tcp_send_packets(socket_data->device_id, packet);
			// wait for a reply
			fibril_condvar_wait(&socket_data->operation.condvar, &socket_data->operation.mutex);
			ERROR_CODE = socket_data->operation.result;
			if(ERROR_CODE != EOK){
				socket_data->addr = NULL;
				socket_data->addrlen = 0;
			}
		}else{
			socket_data->addr = NULL;
			socket_data->addrlen = 0;
			ERROR_CODE = EINTR;
		}
	}

	fibril_mutex_unlock(&socket_data->operation.mutex);

	// return the result
	return ERROR_CODE;
}

int tcp_queue_prepare_packet(socket_core_ref socket, tcp_socket_data_ref socket_data, packet_t packet, size_t data_length){
	ERROR_DECLARE;

	tcp_header_ref header;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	// get tcp header
	header = (tcp_header_ref) packet_get_data(packet);
	if(! header){
		return NO_DATA;
	}
	header->destination_port = htons(socket_data->dest_port);
	header->source_port = htons(socket->port);
	header->sequence_number = htonl(socket_data->next_outgoing);
	if(ERROR_OCCURRED(packet_set_addr(packet, NULL, (uint8_t *) socket_data->addr, socket_data->addrlen))){
		return tcp_release_and_return(packet, EINVAL);
	}
	// remember the outgoing FIN
	if(header->finalize){
		socket_data->fin_outgoing = socket_data->next_outgoing;
	}
	return EOK;
}

int tcp_queue_packet(socket_core_ref socket, tcp_socket_data_ref socket_data, packet_t packet, size_t data_length){
	ERROR_DECLARE;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	ERROR_PROPAGATE(tcp_queue_prepare_packet(socket, socket_data, packet, data_length));

	if(ERROR_OCCURRED(pq_add(&socket_data->outgoing, packet, socket_data->next_outgoing, data_length))){
		return tcp_release_and_return(packet, ERROR_CODE);
	}
	socket_data->next_outgoing += data_length;
	return EOK;
}

packet_t tcp_get_packets_to_send(socket_core_ref socket, tcp_socket_data_ref socket_data){
	ERROR_DECLARE;

	packet_t packet;
	packet_t copy;
	packet_t sending = NULL;
	packet_t previous = NULL;
	size_t data_length;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	packet = pq_find(socket_data->outgoing, socket_data->last_outgoing + 1);
	while(packet){
		pq_get_order(packet, NULL, &data_length);
		// send only if fits into the window
		// respecting the possible overflow
		if(IS_IN_INTERVAL_OVERFLOW((uint32_t) socket_data->last_outgoing, (uint32_t)(socket_data->last_outgoing + data_length), (uint32_t)(socket_data->expected + socket_data->treshold))){
			copy = tcp_prepare_copy(socket, socket_data, packet, data_length, socket_data->last_outgoing + 1);
			if(! copy){
				return sending;
			}
			if(! sending){
				sending = copy;
			}else{
				if(ERROR_OCCURRED(pq_insert_after(previous, copy))){
					pq_release(tcp_globals.net_phone, packet_get_id(copy));
					return sending;
				}
			}
			previous = copy;
			packet = pq_next(packet);
			// overflow occurred ?
			if((! packet) && (socket_data->last_outgoing > socket_data->next_outgoing)){
				printf("gpts overflow\n");
				// continue from the beginning
				packet = socket_data->outgoing;
			}
			socket_data->last_outgoing += data_length;
		}else{
			break;
		}
	}
	return sending;
}

packet_t tcp_send_prepare_packet(socket_core_ref socket, tcp_socket_data_ref socket_data, packet_t packet, size_t data_length, size_t sequence_number){
	ERROR_DECLARE;

	tcp_header_ref header;
	uint32_t checksum;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	// adjust the pseudo header
	if(ERROR_OCCURRED(ip_client_set_pseudo_header_data_length(socket_data->pseudo_header, socket_data->headerlen, packet_get_data_length(packet)))){
		pq_release(tcp_globals.net_phone, packet_get_id(packet));
		return NULL;
	}

	// get the header
	header = (tcp_header_ref) packet_get_data(packet);
	if(! header){
		pq_release(tcp_globals.net_phone, packet_get_id(packet));
		return NULL;
	}
	assert(ntohl(header->sequence_number) == sequence_number);

	// adjust the header
	if(socket_data->next_incoming){
		header->acknowledgement_number = htonl(socket_data->next_incoming);
		header->acknowledge = 1;
	}
	header->window = htons(socket_data->window);

	// checksum
	header->checksum = 0;
	checksum = compute_checksum(0, socket_data->pseudo_header, socket_data->headerlen);
	checksum = compute_checksum(checksum, (uint8_t *) packet_get_data(packet), packet_get_data_length(packet));
	header->checksum = htons(flip_checksum(compact_checksum(checksum)));
	// prepare the packet
	if(ERROR_OCCURRED(ip_client_prepare_packet(packet, IPPROTO_TCP, 0, 0, 0, 0))
	// prepare the timeout
		|| ERROR_OCCURRED(tcp_prepare_timeout(tcp_timeout, socket, socket_data, sequence_number, socket_data->state, socket_data->timeout, true))){
		pq_release(tcp_globals.net_phone, packet_get_id(packet));
		return NULL;
	}
	return packet;
}

packet_t tcp_prepare_copy(socket_core_ref socket, tcp_socket_data_ref socket_data, packet_t packet, size_t data_length, size_t sequence_number){
	packet_t copy;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	// make a copy of the packet
	copy = packet_get_copy(tcp_globals.net_phone, packet);
	if(! copy){
		return NULL;
	}

	return tcp_send_prepare_packet(socket, socket_data, copy, data_length, sequence_number);
}

void tcp_send_packets(device_id_t device_id, packet_t packet){
	packet_t next;

	while(packet){
		next = pq_detach(packet);
		ip_send_msg(tcp_globals.ip_phone, device_id, packet, SERVICE_TCP, 0);
		packet = next;
	}
}

void tcp_prepare_operation_header(socket_core_ref socket, tcp_socket_data_ref socket_data, tcp_header_ref header, int synchronize, int finalize){
	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);
	assert(header);

	bzero(header, sizeof(*header));
	header->source_port = htons(socket->port);
	header->source_port = htons(socket_data->dest_port);
	header->header_length = TCP_COMPUTE_HEADER_LENGTH(sizeof(*header));
	header->synchronize = synchronize;
	header->finalize = finalize;
}

int tcp_prepare_timeout(int (*timeout_function)(void * tcp_timeout_t), socket_core_ref socket, tcp_socket_data_ref socket_data, size_t sequence_number, tcp_socket_state_t state, suseconds_t timeout, int globals_read_only){
	tcp_timeout_ref operation_timeout;
	fid_t fibril;

	assert(socket);
	assert(socket_data);
	assert(socket->specific_data == socket_data);

	// prepare the timeout with key bundle structure
	operation_timeout = malloc(sizeof(*operation_timeout) + socket->key_length + 1);
	if(! operation_timeout){
		return ENOMEM;
	}
	bzero(operation_timeout, sizeof(*operation_timeout));
	operation_timeout->globals_read_only = globals_read_only;
	operation_timeout->port = socket->port;
	operation_timeout->local_sockets = socket_data->local_sockets;
	operation_timeout->socket_id = socket->socket_id;
	operation_timeout->timeout = timeout;
	operation_timeout->sequence_number = sequence_number;
	operation_timeout->state = state;

	// copy the key
	operation_timeout->key = ((char *) operation_timeout) + sizeof(*operation_timeout);
	operation_timeout->key_length = socket->key_length;
	memcpy(operation_timeout->key, socket->key, socket->key_length);
	operation_timeout->key[operation_timeout->key_length] = '\0';

	// prepare the timeouting thread
	fibril = fibril_create(timeout_function, operation_timeout);
	if(! fibril){
		free(operation_timeout);
		return EPARTY;
	}
//	fibril_mutex_lock(&socket_data->operation.mutex);
	// start the timeouting fibril
	fibril_add_ready(fibril);
	//socket_data->state = state;
	return EOK;
}

int tcp_recvfrom_message(socket_cores_ref local_sockets, int socket_id, int flags, size_t * addrlen){
	ERROR_DECLARE;

	socket_core_ref socket;
	tcp_socket_data_ref socket_data;
	int packet_id;
	packet_t packet;
	size_t length;

	assert(local_sockets);

	// find the socket
	socket = socket_cores_find(local_sockets, socket_id);
	if(! socket){
		return ENOTSOCK;
	}
	// get the socket specific data
	if(! socket->specific_data){
		return NO_DATA;
	}
	socket_data = (tcp_socket_data_ref) socket->specific_data;

	// check state
	if((socket_data->state != TCP_SOCKET_ESTABLISHED) && (socket_data->state != TCP_SOCKET_CLOSE_WAIT)){
		return ENOTCONN;
	}

	// send the source address if desired
	if(addrlen){
		ERROR_PROPAGATE(data_reply(socket_data->addr, socket_data->addrlen));
		*addrlen = socket_data->addrlen;
	}

	// get the next received packet
	packet_id = dyn_fifo_value(&socket->received);
	if(packet_id < 0){
		return NO_DATA;
	}
	ERROR_PROPAGATE(packet_translate(tcp_globals.net_phone, &packet, packet_id));

	// reply the packets
	ERROR_PROPAGATE(socket_reply_packets(packet, &length));

	// release the packet
	dyn_fifo_pop(&socket->received);
	pq_release(tcp_globals.net_phone, packet_get_id(packet));
	// return the total length
	return (int) length;
}

int tcp_send_message(socket_cores_ref local_sockets, int socket_id, int fragments, size_t * data_fragment_size, int flags){
	ERROR_DECLARE;

	socket_core_ref socket;
	tcp_socket_data_ref socket_data;
	packet_dimension_ref packet_dimension;
	packet_t packet;
	size_t total_length;
	tcp_header_ref header;
	int index;
	int result;

	assert(local_sockets);
	assert(data_fragment_size);

	// find the socket
	socket = socket_cores_find(local_sockets, socket_id);
	if(! socket){
		return ENOTSOCK;
	}
	// get the socket specific data
	if(! socket->specific_data){
		return NO_DATA;
	}
	socket_data = (tcp_socket_data_ref) socket->specific_data;

	// check state
	if((socket_data->state != TCP_SOCKET_ESTABLISHED) && (socket_data->state != TCP_SOCKET_CLOSE_WAIT)){
		return ENOTCONN;
	}

	ERROR_PROPAGATE(tl_get_ip_packet_dimension(tcp_globals.ip_phone, &tcp_globals.dimensions, socket_data->device_id, &packet_dimension));

	*data_fragment_size = ((packet_dimension->content < socket_data->data_fragment_size) ? packet_dimension->content : socket_data->data_fragment_size);

	for(index = 0; index < fragments; ++ index){
		// read the data fragment
		result = tl_socket_read_packet_data(tcp_globals.net_phone, &packet, TCP_HEADER_SIZE, packet_dimension, socket_data->addr, socket_data->addrlen);
		if(result < 0){
			return result;
		}
		total_length = (size_t) result;
		// prefix the tcp header
		header = PACKET_PREFIX(packet, tcp_header_t);
		if(! header){
			return tcp_release_and_return(packet, ENOMEM);
		}
		tcp_prepare_operation_header(socket, socket_data, header, 0, 0);
		ERROR_PROPAGATE(tcp_queue_packet(socket, socket_data, packet, 0));
	}

	// flush packets
	packet = tcp_get_packets_to_send(socket, socket_data);
	fibril_rwlock_write_unlock(socket_data->local_lock);
	fibril_rwlock_read_unlock(&tcp_globals.lock);
	if(packet){
		// send the packet
		tcp_send_packets(socket_data->device_id, packet);
	}

	return EOK;
}

int tcp_close_message(socket_cores_ref local_sockets, int socket_id){
	ERROR_DECLARE;

	socket_core_ref socket;
	tcp_socket_data_ref socket_data;
	packet_t packet;

	// find the socket
	socket = socket_cores_find(local_sockets, socket_id);
	if(! socket){
		return ENOTSOCK;
	}
	// get the socket specific data
	socket_data = (tcp_socket_data_ref) socket->specific_data;
	assert(socket_data);

	// check state
	switch(socket_data->state){
		case TCP_SOCKET_ESTABLISHED:
			socket_data->state = TCP_SOCKET_FIN_WAIT_1;
			break;
		case TCP_SOCKET_CLOSE_WAIT:
			socket_data->state = TCP_SOCKET_LAST_ACK;
			break;
//		case TCP_SOCKET_LISTEN:
		default:
			// just destroy
			if(! ERROR_OCCURRED(socket_destroy(tcp_globals.net_phone, socket_id, local_sockets, &tcp_globals.sockets, tcp_free_socket_data))){
				fibril_rwlock_write_unlock(socket_data->local_lock);
				fibril_rwlock_write_unlock(&tcp_globals.lock);
			}
			return ERROR_CODE;
	}
	// send FIN
	// TODO should I wait to complete?

	// create the notification packet
	ERROR_PROPAGATE(tcp_create_notification_packet(&packet, socket, socket_data, 0, 1));

	// send the packet
	ERROR_PROPAGATE(tcp_queue_packet(socket, socket_data, packet, 1));

	// flush packets
	packet = tcp_get_packets_to_send(socket, socket_data);
	fibril_rwlock_write_unlock(socket_data->local_lock);
	fibril_rwlock_write_unlock(&tcp_globals.lock);
	if(packet){
		// send the packet
		tcp_send_packets(socket_data->device_id, packet);
	}
	return EOK;
}

int tcp_create_notification_packet(packet_t * packet, socket_core_ref socket, tcp_socket_data_ref socket_data, int synchronize, int finalize){
	ERROR_DECLARE;

	packet_dimension_ref packet_dimension;
	tcp_header_ref header;

	assert(packet);

	// get the device packet dimension
	ERROR_PROPAGATE(tl_get_ip_packet_dimension(tcp_globals.ip_phone, &tcp_globals.dimensions, socket_data->device_id, &packet_dimension));
	// get a new packet
	*packet = packet_get_4(tcp_globals.net_phone, TCP_HEADER_SIZE, packet_dimension->addr_len, packet_dimension->prefix, packet_dimension->suffix);
	if(! * packet){
		return ENOMEM;
	}
	// allocate space in the packet
	header = PACKET_SUFFIX(*packet, tcp_header_t);
	if(! header){
		tcp_release_and_return(*packet, ENOMEM);
	}

	tcp_prepare_operation_header(socket, socket_data, header, synchronize, finalize);
	return EOK;
}

int tcp_accept_message(socket_cores_ref local_sockets, int socket_id, int new_socket_id, size_t * data_fragment_size, size_t * addrlen){
	ERROR_DECLARE;

	socket_core_ref accepted;
	socket_core_ref socket;
	tcp_socket_data_ref socket_data;
	packet_dimension_ref packet_dimension;

	assert(local_sockets);
	assert(data_fragment_size);
	assert(addrlen);

	// find the socket
	socket = socket_cores_find(local_sockets, socket_id);
	if(! socket){
		return ENOTSOCK;
	}
	// get the socket specific data
	socket_data = (tcp_socket_data_ref) socket->specific_data;
	assert(socket_data);

	// check state
	if(socket_data->state != TCP_SOCKET_LISTEN){
		return EINVAL;
	}

	do{
		socket_id = dyn_fifo_value(&socket->accepted);
		if(socket_id < 0){
			return ENOTSOCK;
		}
		socket_id *= -1;

		accepted = socket_cores_find(local_sockets, socket_id);
		if(! accepted){
			return ENOTSOCK;
		}
		// get the socket specific data
		socket_data = (tcp_socket_data_ref) accepted->specific_data;
		assert(socket_data);
		// TODO can it be in another state?
		if(socket_data->state == TCP_SOCKET_ESTABLISHED){
			ERROR_PROPAGATE(data_reply(socket_data->addr, socket_data->addrlen));
			ERROR_PROPAGATE(tl_get_ip_packet_dimension(tcp_globals.ip_phone, &tcp_globals.dimensions, socket_data->device_id, &packet_dimension));
			*addrlen = socket_data->addrlen;
			*data_fragment_size = ((packet_dimension->content < socket_data->data_fragment_size) ? packet_dimension->content : socket_data->data_fragment_size);
			if(new_socket_id > 0){
				ERROR_PROPAGATE(socket_cores_update(local_sockets, accepted->socket_id, new_socket_id));
				accepted->socket_id = new_socket_id;
			}
		}
		dyn_fifo_pop(&socket->accepted);
	}while(socket_data->state != TCP_SOCKET_ESTABLISHED);
	printf("ret accept %d\n", accepted->socket_id);
	return accepted->socket_id;
}

void tcp_free_socket_data(socket_core_ref socket){
	tcp_socket_data_ref socket_data;

	assert(socket);

	printf("destroy_socket %d\n", socket->socket_id);

	// get the socket specific data
	socket_data = (tcp_socket_data_ref) socket->specific_data;
	assert(socket_data);
	//free the pseudo header
	if(socket_data->pseudo_header){
		if(socket_data->headerlen){
			printf("d pseudo\n");
			free(socket_data->pseudo_header);
			socket_data->headerlen = 0;
		}
		socket_data->pseudo_header = NULL;
	}
	socket_data->headerlen = 0;
	// free the address
	if(socket_data->addr){
		if(socket_data->addrlen){
			printf("d addr\n");
			free(socket_data->addr);
			socket_data->addrlen = 0;
		}
		socket_data->addr = NULL;
	}
	socket_data->addrlen = 0;
}

int tcp_release_and_return(packet_t packet, int result){
	pq_release(tcp_globals.net_phone, packet_get_id(packet));
	return result;
}

/** @}
 */
