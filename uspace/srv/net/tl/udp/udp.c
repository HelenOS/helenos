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

/** @addtogroup udp
 * @{
 */

/** @file
 * UDP module implementation.
 * @see udp.h
 */

#include <async.h>
#include <fibril_synch.h>
#include <malloc.h>
#include <stdio.h>
#include <ipc/services.h>
#include <ipc/net.h>
#include <ipc/tl.h>
#include <ipc/socket.h>
#include <adt/dynamic_fifo.h>
#include <errno.h>

#include <net/socket_codes.h>
#include <net/ip_protocols.h>
#include <net/in.h>
#include <net/in6.h>
#include <net/inet.h>
#include <net/modules.h>

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

#include "udp.h"
#include "udp_header.h"

/** UDP module name. */
#define NAME  "udp"

/** Default UDP checksum computing. */
#define NET_DEFAULT_UDP_CHECKSUM_COMPUTING	true

/** Default UDP autobind when sending via unbound sockets. */
#define NET_DEFAULT_UDP_AUTOBINDING	true

/** Maximum UDP fragment size. */
#define MAX_UDP_FRAGMENT_SIZE		65535

/** Free ports pool start. */
#define UDP_FREE_PORTS_START		1025

/** Free ports pool end. */
#define UDP_FREE_PORTS_END		65535

/** UDP global data.  */
udp_globals_t udp_globals;

/** Releases the packet and returns the result.
 *
 * @param[in] packet	The packet queue to be released.
 * @param[in] result	The result to be returned.
 * @return		The result parameter.
 */
static int udp_release_and_return(packet_t *packet, int result)
{
	pq_release_remote(udp_globals.net_sess, packet_get_id(packet));
	return result;
}

/** Processes the received UDP packet queue.
 *
 * Notifies the destination socket application.
 * Releases the packet on error or sends an ICMP error notification.
 *
 * @param[in] device_id	The receiving device identifier.
 * @param[in,out] packet The received packet queue.
 * @param[in] error	The packet error reporting service. Prefixes the
 *			received packet.
 * @return		EOK on success.
 * @return		EINVAL if the packet is not valid.
 * @return		EINVAL if the stored packet address is not the
 *			an_addr_t.
 * @return		EINVAL if the packet does not contain any data.
 * @return 		NO_DATA if the packet content is shorter than the user
 *			datagram header.
 * @return		ENOMEM if there is not enough memory left.
 * @return		EADDRNOTAVAIL if the destination socket does not exist.
 * @return		Other error codes as defined for the
 *			ip_client_process_packet() function.
 */
static int udp_process_packet(nic_device_id_t device_id, packet_t *packet,
    services_t error)
{
	size_t length;
	size_t offset;
	int result;
	udp_header_t *header;
	socket_core_t *socket;
	packet_t *next_packet;
	size_t total_length;
	uint32_t checksum;
	int fragments;
	packet_t *tmp_packet;
	icmp_type_t type;
	icmp_code_t code;
	void *ip_header;
	struct sockaddr *src;
	struct sockaddr *dest;
	packet_dimension_t *packet_dimension;
	int rc;

	switch (error) {
	case SERVICE_NONE:
		break;
	case SERVICE_ICMP:
		/* Ignore error */
		// length = icmp_client_header_length(packet);

		/* Process error */
		result = icmp_client_process_packet(packet, &type,
		    &code, NULL, NULL);
		if (result < 0)
			return udp_release_and_return(packet, result);
		length = (size_t) result;
		rc = packet_trim(packet, length, 0);
		if (rc != EOK)
			return udp_release_and_return(packet, rc);
		break;
	default:
		return udp_release_and_return(packet, ENOTSUP);
	}

	/* TODO process received ipopts? */
	result = ip_client_process_packet(packet, NULL, NULL, NULL, NULL, NULL);
	if (result < 0)
		return udp_release_and_return(packet, result);
	offset = (size_t) result;

	length = packet_get_data_length(packet);
	if (length <= 0)
		return udp_release_and_return(packet, EINVAL);
	if (length < UDP_HEADER_SIZE + offset)
		return udp_release_and_return(packet, NO_DATA);

	/* Trim all but UDP header */
	rc = packet_trim(packet, offset, 0);
	if (rc != EOK)
		return udp_release_and_return(packet, rc);

	/* Get UDP header */
	header = (udp_header_t *) packet_get_data(packet);
	if (!header)
		return udp_release_and_return(packet, NO_DATA);

	/* Find the destination socket */
	socket = socket_port_find(&udp_globals.sockets,
	    ntohs(header->destination_port), (uint8_t *) SOCKET_MAP_KEY_LISTENING, 0);
	if (!socket) {
		if (tl_prepare_icmp_packet(udp_globals.net_sess,
		    udp_globals.icmp_sess, packet, error) == EOK) {
			icmp_destination_unreachable_msg(udp_globals.icmp_sess,
			    ICMP_PORT_UNREACH, 0, packet);
		}
		return EADDRNOTAVAIL;
	}

	/* Count the received packet fragments */
	next_packet = packet;
	fragments = 0;
	total_length = ntohs(header->total_length);

	/* Compute header checksum if set */
	if (header->checksum && !error) {
		result = packet_get_addr(packet, (uint8_t **) &src,
		    (uint8_t **) &dest);
		if (result <= 0)
			return udp_release_and_return(packet, result);
		
		rc = ip_client_get_pseudo_header(IPPROTO_UDP, src, result, dest,
		    result, total_length, &ip_header, &length);
		if (rc != EOK) {
			return udp_release_and_return(packet, rc);
		} else {
			checksum = compute_checksum(0, ip_header, length);
			/*
			 * The udp header checksum will be added with the first
			 * fragment later.
			 */
			free(ip_header);
		}
	} else {
		header->checksum = 0;
		checksum = 0;
	}

	do {
		fragments++;
		length = packet_get_data_length(next_packet);
		if (length <= 0)
			return udp_release_and_return(packet, NO_DATA);

		if (total_length < length) {
			rc = packet_trim(next_packet, 0, length - total_length);
			if (rc != EOK)
				return udp_release_and_return(packet, rc);

			/* Add partial checksum if set */
			if (header->checksum) {
				checksum = compute_checksum(checksum,
				    packet_get_data(packet),
				    packet_get_data_length(packet));
			}

			/* Relese the rest of the packet fragments */
			tmp_packet = pq_next(next_packet);
			while (tmp_packet) {
				next_packet = pq_detach(tmp_packet);
				pq_release_remote(udp_globals.net_sess,
				    packet_get_id(tmp_packet));
				tmp_packet = next_packet;
			}

			/* Exit the loop */
			break;
		}
		total_length -= length;

		/* Add partial checksum if set */
		if (header->checksum) {
			checksum = compute_checksum(checksum,
			    packet_get_data(packet),
			    packet_get_data_length(packet));
		}

	} while ((next_packet = pq_next(next_packet)) && (total_length > 0));

	/* Verify checksum */
	if (header->checksum) {
		if (flip_checksum(compact_checksum(checksum)) !=
		    IP_CHECKSUM_ZERO) {
			if (tl_prepare_icmp_packet(udp_globals.net_sess,
			    udp_globals.icmp_sess, packet, error) == EOK) {
				/* Checksum error ICMP */
				icmp_parameter_problem_msg(
				    udp_globals.icmp_sess, ICMP_PARAM_POINTER,
				    ((size_t) ((void *) &header->checksum)) -
				    ((size_t) ((void *) header)), packet);
			}
			return EINVAL;
		}
	}

	/* Queue the received packet */
	rc = dyn_fifo_push(&socket->received, packet_get_id(packet),
	    SOCKET_MAX_RECEIVED_SIZE);
	if (rc != EOK)
		return udp_release_and_return(packet, rc);
		
	rc = tl_get_ip_packet_dimension(udp_globals.ip_sess,
	    &udp_globals.dimensions, device_id, &packet_dimension);
	if (rc != EOK)
		return udp_release_and_return(packet, rc);

	/* Notify the destination socket */
	fibril_rwlock_write_unlock(&udp_globals.lock);
	
	async_exch_t *exch = async_exchange_begin(socket->sess);
	async_msg_5(exch, NET_SOCKET_RECEIVED, (sysarg_t) socket->socket_id,
	    packet_dimension->content, 0, 0, (sysarg_t) fragments);
	async_exchange_end(exch);

	return EOK;
}

/** Processes the received UDP packet queue.
 *
 * Is used as an entry point from the underlying IP module.
 * Locks the global lock and calls udp_process_packet() function.
 *
 * @param[in] device_id	The receiving device identifier.
 * @param[in,out] packet The received packet queue.
 * @param receiver	The target service. Ignored parameter.
 * @param[in] error	The packet error reporting service. Prefixes the
 *			received packet.
 * @return		EOK on success.
 * @return		Other error codes as defined for the
 *			udp_process_packet() function.
 */
static int udp_received_msg(nic_device_id_t device_id, packet_t *packet,
    services_t receiver, services_t error)
{
	int result;

	fibril_rwlock_write_lock(&udp_globals.lock);
	result = udp_process_packet(device_id, packet, error);
	if (result != EOK)
		fibril_rwlock_write_unlock(&udp_globals.lock);

	return result;
}

/** Process IPC messages from the IP module
 *
 * @param[in]     iid   Message identifier.
 * @param[in,out] icall Message parameters.
 * @param[in]     arg   Local argument.
 *
 */
static void udp_receiver(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	packet_t *packet;
	int rc;
	
	while (true) {
		switch (IPC_GET_IMETHOD(*icall)) {
		case NET_TL_RECEIVED:
			rc = packet_translate_remote(udp_globals.net_sess, &packet,
			    IPC_GET_PACKET(*icall));
			if (rc == EOK)
				rc = udp_received_msg(IPC_GET_DEVICE(*icall), packet,
				    SERVICE_UDP, IPC_GET_ERROR(*icall));
			
			async_answer_0(iid, (sysarg_t) rc);
			break;
		default:
			async_answer_0(iid, (sysarg_t) ENOTSUP);
		}
		
		iid = async_get_call(icall);
	}
}

/** Initialize the UDP module.
 *
 * @param[in] sess Network module session.
 *
 * @return EOK on success.
 * @return ENOMEM if there is not enough memory left.
 *
 */
int tl_initialize(async_sess_t *sess)
{
	measured_string_t names[] = {
		{
			(uint8_t *) "UDP_CHECKSUM_COMPUTING",
			22
		},
		{
			(uint8_t *) "UDP_AUTOBINDING",
			15
		}
	};
	measured_string_t *configuration;
	size_t count = sizeof(names) / sizeof(measured_string_t);
	uint8_t *data;
	
	fibril_rwlock_initialize(&udp_globals.lock);
	fibril_rwlock_write_lock(&udp_globals.lock);
	
	udp_globals.net_sess = sess;
	udp_globals.icmp_sess = icmp_connect_module();
	
	udp_globals.ip_sess = ip_bind_service(SERVICE_IP, IPPROTO_UDP,
	     SERVICE_UDP, udp_receiver);
	if (udp_globals.ip_sess == NULL) {
	    fibril_rwlock_write_unlock(&udp_globals.lock);
	    return ENOENT;
	}
	
	/* Read default packet dimensions */
	int rc = ip_packet_size_req(udp_globals.ip_sess, -1,
	    &udp_globals.packet_dimension);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&udp_globals.lock);
		return rc;
	}
	
	rc = socket_ports_initialize(&udp_globals.sockets);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&udp_globals.lock);
		return rc;
	}
	
	rc = packet_dimensions_initialize(&udp_globals.dimensions);
	if (rc != EOK) {
		socket_ports_destroy(&udp_globals.sockets, free);
		fibril_rwlock_write_unlock(&udp_globals.lock);
		return rc;
	}
	
	udp_globals.packet_dimension.prefix += sizeof(udp_header_t);
	udp_globals.packet_dimension.content -= sizeof(udp_header_t);
	udp_globals.last_used_port = UDP_FREE_PORTS_START - 1;

	udp_globals.checksum_computing = NET_DEFAULT_UDP_CHECKSUM_COMPUTING;
	udp_globals.autobinding = NET_DEFAULT_UDP_AUTOBINDING;

	/* Get configuration */
	configuration = &names[0];
	rc = net_get_conf_req(udp_globals.net_sess, &configuration, count,
	    &data);
	if (rc != EOK) {
		socket_ports_destroy(&udp_globals.sockets, free);
		fibril_rwlock_write_unlock(&udp_globals.lock);
		return rc;
	}
	
	if (configuration) {
		if (configuration[0].value)
			udp_globals.checksum_computing =
			    (configuration[0].value[0] == 'y');
		
		if (configuration[1].value)
			udp_globals.autobinding =
			    (configuration[1].value[0] == 'y');

		net_free_settings(configuration, data);
	}

	fibril_rwlock_write_unlock(&udp_globals.lock);
	return EOK;
}

/** Sends data from the socket to the remote address.
 *
 * Binds the socket to a free port if not already connected/bound.
 * Handles the NET_SOCKET_SENDTO message.
 * Supports AF_INET and AF_INET6 address families.
 *
 * @param[in,out] local_sockets The application local sockets.
 * @param[in] socket_id	Socket identifier.
 * @param[in] addr	The destination address.
 * @param[in] addrlen	The address length.
 * @param[in] fragments	The number of data fragments.
 * @param[out] data_fragment_size The data fragment size in bytes.
 * @param[in] flags	Various send flags.
 * @return		EOK on success.
 * @return		EAFNOTSUPPORT if the address family is not supported.
 * @return		ENOTSOCK if the socket is not found.
 * @return		EINVAL if the address is invalid.
 * @return		ENOTCONN if the sending socket is not and cannot be
 *			bound.
 * @return		ENOMEM if there is not enough memory left.
 * @return		Other error codes as defined for the
 *			socket_read_packet_data() function.
 * @return		Other error codes as defined for the
 *			ip_client_prepare_packet() function.
 * @return		Other error codes as defined for the ip_send_msg()
 *			function.
 */
static int udp_sendto_message(socket_cores_t *local_sockets, int socket_id,
    const struct sockaddr *addr, socklen_t addrlen, int fragments,
    size_t *data_fragment_size, int flags)
{
	socket_core_t *socket;
	packet_t *packet;
	packet_t *next_packet;
	udp_header_t *header;
	int index;
	size_t total_length;
	int result;
	uint16_t dest_port;
	uint32_t checksum;
	void *ip_header;
	size_t headerlen;
	nic_device_id_t device_id;
	packet_dimension_t *packet_dimension;
	size_t size;
	int rc;

	/* In case of error, do not update the data fragment size. */
	*data_fragment_size = 0;
	
	rc = tl_get_address_port(addr, addrlen, &dest_port);
	if (rc != EOK)
		return rc;

	socket = socket_cores_find(local_sockets, socket_id);
	if (!socket)
		return ENOTSOCK;

	if ((socket->port <= 0) && udp_globals.autobinding) {
		/* Bind the socket to a random free port if not bound */
		rc = socket_bind_free_port(&udp_globals.sockets, socket,
		    UDP_FREE_PORTS_START, UDP_FREE_PORTS_END,
		    udp_globals.last_used_port);
		if (rc != EOK)
			return rc;
		/* Set the next port as the search starting port number */
		udp_globals.last_used_port = socket->port;
	}

	if (udp_globals.checksum_computing) {
		rc = ip_get_route_req(udp_globals.ip_sess, IPPROTO_UDP, addr,
		    addrlen, &device_id, &ip_header, &headerlen);
		if (rc != EOK)
			return rc;
		/* Get the device packet dimension */
//		rc = tl_get_ip_packet_dimension(udp_globals.ip_sess,
//		    &udp_globals.dimensions, device_id, &packet_dimension);
//		if (rc != EOK)
//			return rc;
	}
//	} else {
		/* Do not ask all the time */
		rc = ip_packet_size_req(udp_globals.ip_sess, -1,
		    &udp_globals.packet_dimension);
		if (rc != EOK)
			return rc;
		packet_dimension = &udp_globals.packet_dimension;
//	}

	/*
	 * Update the data fragment size based on what the lower layers can
	 * handle without fragmentation, but not more than the maximum allowed
	 * for UDP.
	 */
	size = MAX_UDP_FRAGMENT_SIZE;
	if (packet_dimension->content < size)
	    size = packet_dimension->content;
	*data_fragment_size = size;

	/* Read the first packet fragment */
	result = tl_socket_read_packet_data(udp_globals.net_sess, &packet,
	    UDP_HEADER_SIZE, packet_dimension, addr, addrlen);
	if (result < 0)
		return result;

	total_length = (size_t) result;
	if (udp_globals.checksum_computing)
		checksum = compute_checksum(0, packet_get_data(packet),
		    packet_get_data_length(packet));
	else
		checksum = 0;

	/* Prefix the UDP header */
	header = PACKET_PREFIX(packet, udp_header_t);
	if (!header)
		return udp_release_and_return(packet, ENOMEM);

	bzero(header, sizeof(*header));

	/* Read the rest of the packet fragments */
	for (index = 1; index < fragments; index++) {
		result = tl_socket_read_packet_data(udp_globals.net_sess,
		    &next_packet, 0, packet_dimension, addr, addrlen);
		if (result < 0)
			return udp_release_and_return(packet, result);

		rc = pq_add(&packet, next_packet, index, 0);
		if (rc != EOK)
			return udp_release_and_return(packet, rc);

		total_length += (size_t) result;
		if (udp_globals.checksum_computing) {
			checksum = compute_checksum(checksum,
			    packet_get_data(next_packet),
			    packet_get_data_length(next_packet));
		}
	}

	/* Set the UDP header */
	header->source_port = htons((socket->port > 0) ? socket->port : 0);
	header->destination_port = htons(dest_port);
	header->total_length = htons(total_length + sizeof(*header));
	header->checksum = 0;

	if (udp_globals.checksum_computing) {
		/* Update the pseudo header */
		rc = ip_client_set_pseudo_header_data_length(ip_header,
		    headerlen, total_length + UDP_HEADER_SIZE);
		if (rc != EOK) {
			free(ip_header);
			return udp_release_and_return(packet, rc);
		}

		/* Finish the checksum computation */
		checksum = compute_checksum(checksum, ip_header, headerlen);
		checksum = compute_checksum(checksum, (uint8_t *) header,
		    sizeof(*header));
		header->checksum =
		    htons(flip_checksum(compact_checksum(checksum)));
		free(ip_header);
	} else
		device_id = NIC_DEVICE_INVALID_ID;

	/* Prepare the first packet fragment */
	rc = ip_client_prepare_packet(packet, IPPROTO_UDP, 0, 0, 0, 0);
	if (rc != EOK)
		return udp_release_and_return(packet, rc);

	/* Release the UDP global lock on success. */
	fibril_rwlock_write_unlock(&udp_globals.lock);

	/* Send the packet */
	ip_send_msg(udp_globals.ip_sess, device_id, packet, SERVICE_UDP, 0);

	return EOK;
}

/** Receives data to the socket.
 *
 * Handles the NET_SOCKET_RECVFROM message.
 * Replies the source address as well.
 *
 * @param[in] local_sockets The application local sockets.
 * @param[in] socket_id	Socket identifier.
 * @param[in] flags	Various receive flags.
 * @param[out] addrlen	The source address length.
 * @return		The number of bytes received.
 * @return		ENOTSOCK if the socket is not found.
 * @return		NO_DATA if there are no received packets or data.
 * @return		ENOMEM if there is not enough memory left.
 * @return		EINVAL if the received address is not an IP address.
 * @return		Other error codes as defined for the packet_translate()
 *			function.
 * @return		Other error codes as defined for the data_reply()
 *			function.
 */
static int udp_recvfrom_message(socket_cores_t *local_sockets, int socket_id,
    int flags, size_t *addrlen)
{
	socket_core_t *socket;
	int packet_id;
	packet_t *packet;
	udp_header_t *header;
	struct sockaddr *addr;
	size_t length;
	uint8_t *data;
	int result;
	int rc;

	/* Find the socket */
	socket = socket_cores_find(local_sockets, socket_id);
	if (!socket)
		return ENOTSOCK;

	/* Get the next received packet */
	packet_id = dyn_fifo_value(&socket->received);
	if (packet_id < 0)
		return NO_DATA;
	
	rc = packet_translate_remote(udp_globals.net_sess, &packet, packet_id);
	if (rc != EOK) {
		(void) dyn_fifo_pop(&socket->received);
		return rc;
	}

	/* Get UDP header */
	data = packet_get_data(packet);
	if (!data) {
		(void) dyn_fifo_pop(&socket->received);
		return udp_release_and_return(packet, NO_DATA);
	}
	header = (udp_header_t *) data;

	/* Set the source address port */
	result = packet_get_addr(packet, (uint8_t **) &addr, NULL);
	rc = tl_set_address_port(addr, result, ntohs(header->source_port));
	if (rc != EOK) {
		(void) dyn_fifo_pop(&socket->received);
		return udp_release_and_return(packet, rc);
	}
	*addrlen = (size_t) result;

	/* Send the source address */
	rc = data_reply(addr, *addrlen);
	switch (rc) {
	case EOK:
		break;
	case EOVERFLOW:
		return rc;
	default:
		(void) dyn_fifo_pop(&socket->received);
		return udp_release_and_return(packet, rc);
	}

	/* Trim the header */
	rc = packet_trim(packet, UDP_HEADER_SIZE, 0);
	if (rc != EOK) {
		(void) dyn_fifo_pop(&socket->received);
		return udp_release_and_return(packet, rc);
	}

	/* Reply the packets */
	rc = socket_reply_packets(packet, &length);
	switch (rc) {
	case EOK:
		break;
	case EOVERFLOW:
		return rc;
	default:
		(void) dyn_fifo_pop(&socket->received);
		return udp_release_and_return(packet, rc);
	}

	(void) dyn_fifo_pop(&socket->received);

	/* Release the packet and return the total length */
	return udp_release_and_return(packet, (int) length);
}

/** Process the socket client messages.
 *
 * Run until the client module disconnects.
 *
 * @see socket.h
 *
 * @param[in] sess   Callback session.
 * @param[in] callid Message identifier.
 * @param[in] call   Message parameters.
 *
 * @return EOK on success.
 *
 */
static int udp_process_client_messages(async_sess_t *sess, ipc_callid_t callid,
    ipc_call_t call)
{
	int res;
	socket_cores_t local_sockets;
	struct sockaddr *addr;
	int socket_id;
	size_t addrlen;
	size_t size;
	ipc_call_t answer;
	size_t answer_count;
	packet_dimension_t *packet_dimension;

	/*
	 * Accept the connection
	 *  - Answer the first IPC_M_CONNECT_TO_ME call.
	 */
	res = EOK;
	answer_count = 0;

	/*
	 * The client connection is only in one fibril and therefore no
	 * additional locks are needed.
	 */

	socket_cores_initialize(&local_sockets);

	while (true) {

		/* Answer the call */
		answer_call(callid, res, &answer, answer_count);

		/* Refresh data */
		refresh_answer(&answer, &answer_count);

		/* Get the next call */
		callid = async_get_call(&call);

		/* Process the call */
		if (!IPC_GET_IMETHOD(call)) {
			res = EHANGUP;
			break;
		}
		
		switch (IPC_GET_IMETHOD(call)) {
		case NET_SOCKET:
			socket_id = SOCKET_GET_SOCKET_ID(call);
			res = socket_create(&local_sockets, sess, NULL,
			    &socket_id);
			SOCKET_SET_SOCKET_ID(answer, socket_id);

			if (res != EOK)
				break;
			
			size = MAX_UDP_FRAGMENT_SIZE;
			if (tl_get_ip_packet_dimension(udp_globals.ip_sess,
			    &udp_globals.dimensions, NIC_DEVICE_INVALID_ID,
			    &packet_dimension) == EOK) {
				if (packet_dimension->content < size)
					size = packet_dimension->content;
			}
			SOCKET_SET_DATA_FRAGMENT_SIZE(answer, size);
			SOCKET_SET_HEADER_SIZE(answer, UDP_HEADER_SIZE);
			answer_count = 3;
			break;

		case NET_SOCKET_BIND:
			res = async_data_write_accept((void **) &addr, false,
			    0, 0, 0, &addrlen);
			if (res != EOK)
				break;
			fibril_rwlock_write_lock(&udp_globals.lock);
			res = socket_bind(&local_sockets, &udp_globals.sockets,
			    SOCKET_GET_SOCKET_ID(call), addr, addrlen,
			    UDP_FREE_PORTS_START, UDP_FREE_PORTS_END,
			    udp_globals.last_used_port);
			fibril_rwlock_write_unlock(&udp_globals.lock);
			free(addr);
			break;

		case NET_SOCKET_SENDTO:
			res = async_data_write_accept((void **) &addr, false,
			    0, 0, 0, &addrlen);
			if (res != EOK)
				break;

			fibril_rwlock_write_lock(&udp_globals.lock);
			res = udp_sendto_message(&local_sockets,
			    SOCKET_GET_SOCKET_ID(call), addr, addrlen,
			    SOCKET_GET_DATA_FRAGMENTS(call), &size,
			    SOCKET_GET_FLAGS(call));
			SOCKET_SET_DATA_FRAGMENT_SIZE(answer, size);

			if (res != EOK)
				fibril_rwlock_write_unlock(&udp_globals.lock);
			else
				answer_count = 2;
			
			free(addr);
			break;

		case NET_SOCKET_RECVFROM:
			fibril_rwlock_write_lock(&udp_globals.lock);
			res = udp_recvfrom_message(&local_sockets,
			     SOCKET_GET_SOCKET_ID(call), SOCKET_GET_FLAGS(call),
			     &addrlen);
			fibril_rwlock_write_unlock(&udp_globals.lock);

			if (res <= 0)
				break;

			SOCKET_SET_READ_DATA_LENGTH(answer, res);
			SOCKET_SET_ADDRESS_LENGTH(answer, addrlen);
			answer_count = 3;
			res = EOK;
			break;
			
		case NET_SOCKET_CLOSE:
			fibril_rwlock_write_lock(&udp_globals.lock);
			res = socket_destroy(udp_globals.net_sess,
			    SOCKET_GET_SOCKET_ID(call), &local_sockets,
			    &udp_globals.sockets, NULL);
			fibril_rwlock_write_unlock(&udp_globals.lock);
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

	/* Release all local sockets */
	socket_cores_release(udp_globals.net_sess, &local_sockets,
	    &udp_globals.sockets, NULL);

	return res;
}

/** Per-connection initialization
 *
 */
void tl_connection(void)
{
}

/** Processes the UDP message.
 *
 * @param[in] callid	The message identifier.
 * @param[in] call	The message parameters.
 * @param[out] answer	The message answer parameters.
 * @param[out] answer_count The last parameter for the actual answer in the
 *			answer parameter.
 * @return		EOK on success.
 * @return		ENOTSUP if the message is not known.
 *
 * @see udp_interface.h
 * @see IS_NET_UDP_MESSAGE()
 */
int tl_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, size_t *answer_count)
{
	*answer_count = 0;
	
	async_sess_t *callback =
	    async_callback_receive_start(EXCHANGE_SERIALIZE, call);
	if (callback)
		return udp_process_client_messages(callback, callid, *call);
	
	return ENOTSUP;
}

int main(int argc, char *argv[])
{
	/* Start the module */
	return tl_module_start(SERVICE_UDP);
}

/** @}
 */
