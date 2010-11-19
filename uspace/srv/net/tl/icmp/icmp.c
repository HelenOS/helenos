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

/** @addtogroup icmp
 * @{
 */

/** @file
 * ICMP module implementation.
 * @see icmp.h
 */

#include "icmp.h"
#include "icmp_module.h"

#include <async.h>
#include <atomic.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <stdint.h>
#include <str.h>
#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/net.h>
#include <ipc/tl.h>
#include <ipc/icmp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <byteorder.h>
#include <errno.h>

#include <net/socket_codes.h>
#include <net/ip_protocols.h>
#include <net/inet.h>
#include <net/modules.h>
#include <net/icmp_api.h>
#include <net/icmp_codes.h>
#include <net/icmp_common.h>

#include <packet_client.h>
#include <packet_remote.h>
#include <net_checksum.h>
#include <icmp_client.h>
#include <icmp_interface.h>
#include <il_interface.h>
#include <ip_client.h>
#include <ip_interface.h>
#include <net_interface.h>
#include <tl_interface.h>
#include <tl_local.h>
#include <icmp_header.h>

/** ICMP module name. */
#define NAME	"ICMP protocol"

/** Default ICMP error reporting. */
#define NET_DEFAULT_ICMP_ERROR_REPORTING	true

/** Default ICMP echo replying. */
#define NET_DEFAULT_ICMP_ECHO_REPLYING		true

/** Original datagram length in bytes transfered to the error notification
 * message.
 */
#define ICMP_KEEP_LENGTH	8

/** Free identifier numbers pool start. */
#define ICMP_FREE_IDS_START	1

/** Free identifier numbers pool end. */
#define ICMP_FREE_IDS_END	UINT16_MAX

/** Computes the ICMP datagram checksum.
 *
 * @param[in,out] header The ICMP datagram header.
 * @param[in] length	The total datagram length.
 * @returns		The computed checksum.
 */
#define ICMP_CHECKSUM(header, length) \
	htons(ip_checksum((uint8_t *) (header), (length)))

/** An echo request datagrams pattern. */
#define ICMP_ECHO_TEXT		"Hello from HelenOS."

/** Computes an ICMP reply data key.
 *
 * @param[in] id	The message identifier.
 * @param[in] sequence	The message sequence number.
 * @returns		The computed ICMP reply data key.
 */
#define ICMP_GET_REPLY_KEY(id, sequence) \
	(((id) << 16) | (sequence & 0xFFFF))


/** ICMP global data. */
icmp_globals_t	icmp_globals;

INT_MAP_IMPLEMENT(icmp_replies, icmp_reply_t);
INT_MAP_IMPLEMENT(icmp_echo_data, icmp_echo_t);

/** Releases the packet and returns the result.
 *
 * @param[in] packet	The packet queue to be released.
 * @param[in] result	The result to be returned.
 * @returns		The result parameter.
 */
static int icmp_release_and_return(packet_t packet, int result)
{
	pq_release_remote(icmp_globals.net_phone, packet_get_id(packet));
	return result;
}

/** Sends the ICMP message.
 *
 * Sets the message type and code and computes the checksum.
 * Error messages are sent only if allowed in the configuration.
 * Releases the packet on errors.
 *
 * @param[in] type	The message type.
 * @param[in] code	The message code.
 * @param[in] packet	The message packet to be sent.
 * @param[in] header	The ICMP header.
 * @param[in] error	The error service to be announced. Should be
 *			SERVICE_ICMP or zero.
 * @param[in] ttl	The time to live.
 * @param[in] tos	The type of service.
 * @param[in] dont_fragment The value indicating whether the datagram must not
 *			be fragmented. Is used as a MTU discovery.
 * @returns		EOK on success.
 * @returns		EPERM if the error message is not allowed.
 */
static int icmp_send_packet(icmp_type_t type, icmp_code_t code, packet_t packet,
    icmp_header_t *header, services_t error, ip_ttl_t ttl, ip_tos_t tos,
    int dont_fragment)
{
	int rc;

	/* Do not send an error if disabled */
	if (error && !icmp_globals.error_reporting)
		return icmp_release_and_return(packet, EPERM);

	header->type = type;
	header->code = code;
	header->checksum = 0;
	header->checksum = ICMP_CHECKSUM(header,
	    packet_get_data_length(packet));
	
	rc = ip_client_prepare_packet(packet, IPPROTO_ICMP, ttl, tos,
	    dont_fragment, 0);
	if (rc != EOK)
		return icmp_release_and_return(packet, rc);

	return ip_send_msg(icmp_globals.ip_phone, -1, packet, SERVICE_ICMP,
	    error);
}

/** Prepares the ICMP error packet.
 *
 * Truncates the original packet if longer than ICMP_KEEP_LENGTH bytes.
 * Prefixes and returns the ICMP header.
 *
 * @param[in,out] packet The original packet.
 * @returns The prefixed ICMP header.
 * @returns NULL on errors.
 */
static icmp_header_t *icmp_prepare_packet(packet_t packet)
{
	icmp_header_t *header;
	size_t header_length;
	size_t total_length;

	total_length = packet_get_data_length(packet);
	if (total_length <= 0)
		return NULL;

	header_length = ip_client_header_length(packet);
	if (header_length <= 0)
		return NULL;

	/* Truncate if longer than 64 bits (without the IP header) */
	if ((total_length > header_length + ICMP_KEEP_LENGTH) &&
	    (packet_trim(packet, 0,
	    total_length - header_length - ICMP_KEEP_LENGTH) != EOK)) {
		return NULL;
	}

	header = PACKET_PREFIX(packet, icmp_header_t);
	if (!header)
		return NULL;

	bzero(header, sizeof(*header));
	return header;
}

/** Requests an echo message.
 *
 * Sends a packet with specified parameters to the target host and waits for
 * the reply upto the given timeout.
 * Blocks the caller until the reply or the timeout occurs.
 *
 * @param[in] id	The message identifier.
 * @param[in] sequence	The message sequence parameter.
 * @param[in] size	The message data length in bytes.
 * @param[in] timeout	The timeout in miliseconds.
 * @param[in] ttl	The time to live.
 * @param[in] tos	The type of service.
 * @param[in] dont_fragment The value indicating whether the datagram must not
 *			be fragmented. Is used as a MTU discovery.
 * @param[in] addr	The target host address.
 * @param[in] addrlen	The torget host address length.
 * @returns		ICMP_ECHO on success.
 * @returns		ETIMEOUT if the reply has not arrived before the
 *			timeout.
 * @returns		ICMP type of the received error notification.
 * @returns		EINVAL if the addrlen parameter is less or equal to
 *			zero.
 * @returns		ENOMEM if there is not enough memory left.
 * @returns		EPARTY if there was an internal error.
 */
static int icmp_echo(icmp_param_t id, icmp_param_t sequence, size_t size,
    mseconds_t timeout, ip_ttl_t ttl, ip_tos_t tos, int dont_fragment,
    const struct sockaddr * addr, socklen_t addrlen)
{
	icmp_header_t *header;
	packet_t packet;
	size_t length;
	uint8_t *data;
	icmp_reply_t *reply;
	int reply_key;
	int index;
	int rc;

	if (addrlen <= 0)
		return EINVAL;

	length = (size_t) addrlen;
	/* TODO do not ask all the time */
	rc = ip_packet_size_req(icmp_globals.ip_phone, -1,
	    &icmp_globals.packet_dimension);
	if (rc != EOK)
		return rc;

	packet = packet_get_4_remote(icmp_globals.net_phone, size,
	    icmp_globals.packet_dimension.addr_len,
	    ICMP_HEADER_SIZE + icmp_globals.packet_dimension.prefix,
	    icmp_globals.packet_dimension.suffix);
	if (!packet)
		return ENOMEM;

	/* Prepare the requesting packet, set the destination address. */
	rc = packet_set_addr(packet, NULL, (const uint8_t *) addr, length);
	if (rc != EOK)
		return icmp_release_and_return(packet, rc);

	/* Allocate space in the packet */
	data = (uint8_t *) packet_suffix(packet, size);
	if (!data)
		return icmp_release_and_return(packet, ENOMEM);

	/* Fill the data */
	length = 0;
	while (size > length + sizeof(ICMP_ECHO_TEXT)) {
		memcpy(data + length, ICMP_ECHO_TEXT, sizeof(ICMP_ECHO_TEXT));
		length += sizeof(ICMP_ECHO_TEXT);
	}
	memcpy(data + length, ICMP_ECHO_TEXT, size - length);

	/* Prefix the header */
	header = PACKET_PREFIX(packet, icmp_header_t);
	if (!header)
		return icmp_release_and_return(packet, ENOMEM);

	bzero(header, sizeof(*header));
	header->un.echo.identifier = id;
	header->un.echo.sequence_number = sequence;

	/* Prepare the reply structure */
	reply = malloc(sizeof(*reply));
	if (!reply)
		return icmp_release_and_return(packet, ENOMEM);

	fibril_mutex_initialize(&reply->mutex);
	fibril_mutex_lock(&reply->mutex);
	fibril_condvar_initialize(&reply->condvar);
	reply_key = ICMP_GET_REPLY_KEY(header->un.echo.identifier,
	    header->un.echo.sequence_number);
	index = icmp_replies_add(&icmp_globals.replies, reply_key, reply);
	if (index < 0) {
		free(reply);
		return icmp_release_and_return(packet, index);
	}

	/* Unlock the globals so that we can wait for the reply */
	fibril_rwlock_write_unlock(&icmp_globals.lock);

	/* Send the request */
	icmp_send_packet(ICMP_ECHO, 0, packet, header, 0, ttl, tos,
	    dont_fragment);

	/* Wait for the reply. Timeout in microseconds. */
	rc = fibril_condvar_wait_timeout(&reply->condvar, &reply->mutex,
	    timeout * 1000);
	if (rc == EOK)
		rc = reply->result;

	/* Drop the reply mutex before locking the globals again */
	fibril_mutex_unlock(&reply->mutex);
	fibril_rwlock_write_lock(&icmp_globals.lock);

	/* Destroy the reply structure */
	icmp_replies_exclude_index(&icmp_globals.replies, index);

	return rc;
}

static int icmp_destination_unreachable_msg_local(int icmp_phone,
    icmp_code_t code, icmp_param_t mtu, packet_t packet)
{
	icmp_header_t *header;

	header = icmp_prepare_packet(packet);
	if (!header)
		return icmp_release_and_return(packet, ENOMEM);

	if (mtu)
		header->un.frag.mtu = mtu;

	return icmp_send_packet(ICMP_DEST_UNREACH, code, packet, header,
	    SERVICE_ICMP, 0, 0, 0);
}

static int icmp_source_quench_msg_local(int icmp_phone, packet_t packet)
{
	icmp_header_t *header;

	header = icmp_prepare_packet(packet);
	if (!header)
		return icmp_release_and_return(packet, ENOMEM);

	return icmp_send_packet(ICMP_SOURCE_QUENCH, 0, packet, header,
	    SERVICE_ICMP, 0, 0, 0);
}

static int icmp_time_exceeded_msg_local(int icmp_phone, icmp_code_t code,
    packet_t packet)
{
	icmp_header_t *header;

	header = icmp_prepare_packet(packet);
	if (!header)
		return icmp_release_and_return(packet, ENOMEM);

	return icmp_send_packet(ICMP_TIME_EXCEEDED, code, packet, header,
	    SERVICE_ICMP, 0, 0, 0);
}

static int icmp_parameter_problem_msg_local(int icmp_phone, icmp_code_t code,
    icmp_param_t pointer, packet_t packet)
{
	icmp_header_t *header;

	header = icmp_prepare_packet(packet);
	if (!header)
		return icmp_release_and_return(packet, ENOMEM);

	header->un.param.pointer = pointer;
	return icmp_send_packet(ICMP_PARAMETERPROB, code, packet, header,
	    SERVICE_ICMP, 0, 0, 0);
}

/** Initializes the ICMP module.
 *
 * @param[in] client_connection The client connection processing function. The
 *			module skeleton propagates its own one.
 * @returns		EOK on success.
 * @returns		ENOMEM if there is not enough memory left.
 */
int icmp_initialize(async_client_conn_t client_connection)
{
	measured_string_t names[] = {
		{
			(char *) "ICMP_ERROR_REPORTING",
			20
		},
		{
			(char *) "ICMP_ECHO_REPLYING",
			18
		}
	};
	measured_string_t *configuration;
	size_t count = sizeof(names) / sizeof(measured_string_t);
	char *data;
	int rc;

	fibril_rwlock_initialize(&icmp_globals.lock);
	fibril_rwlock_write_lock(&icmp_globals.lock);
	icmp_replies_initialize(&icmp_globals.replies);
	icmp_echo_data_initialize(&icmp_globals.echo_data);
	
	icmp_globals.ip_phone = ip_bind_service(SERVICE_IP, IPPROTO_ICMP,
	    SERVICE_ICMP, client_connection);
	if (icmp_globals.ip_phone < 0) {
		fibril_rwlock_write_unlock(&icmp_globals.lock);
		return icmp_globals.ip_phone;
	}
	
	rc = ip_packet_size_req(icmp_globals.ip_phone, -1,
	    &icmp_globals.packet_dimension);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&icmp_globals.lock);
		return rc;
	}

	icmp_globals.packet_dimension.prefix += ICMP_HEADER_SIZE;
	icmp_globals.packet_dimension.content -= ICMP_HEADER_SIZE;

	icmp_globals.error_reporting = NET_DEFAULT_ICMP_ERROR_REPORTING;
	icmp_globals.echo_replying = NET_DEFAULT_ICMP_ECHO_REPLYING;

	/* Get configuration */
	configuration = &names[0];
	rc = net_get_conf_req(icmp_globals.net_phone, &configuration, count,
	    &data);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&icmp_globals.lock);
		return rc;
	}
	
	if (configuration) {
		if (configuration[0].value) {
			icmp_globals.error_reporting =
			    (configuration[0].value[0] == 'y');
		}
		if (configuration[1].value) {
			icmp_globals.echo_replying =
			    (configuration[1].value[0] == 'y');
		}
		net_free_settings(configuration, data);
	}

	fibril_rwlock_write_unlock(&icmp_globals.lock);
	return EOK;
}

/** Tries to set the pending reply result as the received message type.
 *
 * If the reply data is not present, the reply timed out and the other fibril
 * is already awake.
 * Releases the packet.
 *
 * @param[in] packet	The received reply message.
 * @param[in] header	The ICMP message header.
 * @param[in] type	The received reply message type.
 * @param[in] code	The received reply message code.
 */
static void  icmp_process_echo_reply(packet_t packet, icmp_header_t *header,
    icmp_type_t type, icmp_code_t code)
{
	int reply_key;
	icmp_reply_t *reply;

	/* Compute the reply key */
	reply_key = ICMP_GET_REPLY_KEY(header->un.echo.identifier,
	    header->un.echo.sequence_number);
	pq_release_remote(icmp_globals.net_phone, packet_get_id(packet));

	/* Find the pending reply */
	fibril_rwlock_write_lock(&icmp_globals.lock);
	reply = icmp_replies_find(&icmp_globals.replies, reply_key);
	if (reply) {
		reply->result = type;
		fibril_condvar_signal(&reply->condvar);
	}
	fibril_rwlock_write_unlock(&icmp_globals.lock);
}

/** Processes the received ICMP packet.
 *
 * Notifies the destination socket application.
 *
 * @param[in,out] packet The received packet.
 * @param[in] error	The packet error reporting service. Prefixes the
 *			received packet.
 * @returns		EOK on success.
 * @returns		EINVAL if the packet is not valid.
 * @returns		EINVAL if the stored packet address is not the an_addr_t.
 * @returns		EINVAL if the packet does not contain any data.
 * @returns		NO_DATA if the packet content is shorter than the user
 *			datagram header.
 * @returns		ENOMEM if there is not enough memory left.
 * @returns		EADDRNOTAVAIL if the destination socket does not exist.
 * @returns		Other error codes as defined for the
 *			ip_client_process_packet() function.
 */
static int icmp_process_packet(packet_t packet, services_t error)
{
	size_t length;
	uint8_t *src;
	int addrlen;
	int result;
	void *data;
	icmp_header_t *header;
	icmp_type_t type;
	icmp_code_t code;
	int rc;

	switch (error) {
	case SERVICE_NONE:
		break;
	case SERVICE_ICMP:
		/* Process error */
		result = icmp_client_process_packet(packet, &type, &code, NULL,
		    NULL);
		if (result < 0)
			return result;
		length = (size_t) result;
		/* Remove the error header */
		rc = packet_trim(packet, length, 0);
		if (rc != EOK)
			return rc;
		break;
	default:
		return ENOTSUP;
	}

	/* Get rid of the IP header */
	length = ip_client_header_length(packet);
	rc = packet_trim(packet, length, 0);
	if (rc != EOK)
		return rc;

	length = packet_get_data_length(packet);
	if (length <= 0)
		return EINVAL;

	if (length < ICMP_HEADER_SIZE)
		return EINVAL;

	data = packet_get_data(packet);
	if (!data)
		return EINVAL;

	/* Get ICMP header */
	header = (icmp_header_t *) data;

	if (header->checksum) {
		while (ICMP_CHECKSUM(header, length) != IP_CHECKSUM_ZERO) {
			/*
			 * Set the original message type on error notification.
			 * Type swap observed in Qemu.
			 */
			if (error) {
				switch (header->type) {
				case ICMP_ECHOREPLY:
					header->type = ICMP_ECHO;
					continue;
				}
			}
			return EINVAL;
		}
	}

	switch (header->type) {
	case ICMP_ECHOREPLY:
		if (error)
			icmp_process_echo_reply(packet, header, type, code);
		else
			icmp_process_echo_reply(packet, header, ICMP_ECHO, 0);

		return EOK;

	case ICMP_ECHO:
		if (error) {
			icmp_process_echo_reply(packet, header, type, code);
			return EOK;
		}
		
		/* Do not send a reply if disabled */
		if (icmp_globals.echo_replying) {
			addrlen = packet_get_addr(packet, &src, NULL);

			/*
			 * Set both addresses to the source one (avoids the
			 * source address deletion before setting the
			 * destination one).
			 */
			if ((addrlen > 0) && (packet_set_addr(packet, src, src,
			    (size_t) addrlen) == EOK)) {
				/* Send the reply */
				icmp_send_packet(ICMP_ECHOREPLY, 0, packet,
				    header, 0, 0, 0, 0);
				return EOK;
			}

			return EINVAL;
		}

		return EPERM;

	case ICMP_DEST_UNREACH:
	case ICMP_SOURCE_QUENCH:
	case ICMP_REDIRECT:
	case ICMP_ALTERNATE_ADDR:
	case ICMP_ROUTER_ADV:
	case ICMP_ROUTER_SOL:
	case ICMP_TIME_EXCEEDED:
	case ICMP_PARAMETERPROB:
	case ICMP_CONVERSION_ERROR:
	case ICMP_REDIRECT_MOBILE:
	case ICMP_SKIP:
	case ICMP_PHOTURIS:
		ip_received_error_msg(icmp_globals.ip_phone, -1, packet,
		    SERVICE_IP, SERVICE_ICMP);
		return EOK;

	default:
		return ENOTSUP;
	}
}

/** Processes the received ICMP packet.
 *
 * Is used as an entry point from the underlying IP module.
 * Releases the packet on error.
 *
 * @param device_id	The device identifier. Ignored parameter.
 * @param[in,out] packet The received packet.
 * @param receiver	The target service. Ignored parameter.
 * @param[in] error	The packet error reporting service. Prefixes the
 *			received packet.
 * @returns		EOK on success.
 * @returns		Other error codes as defined for the
 *			icmp_process_packet() function.
 */
static int icmp_received_msg_local(device_id_t device_id, packet_t packet,
    services_t receiver, services_t error)
{
	int rc;

	rc = icmp_process_packet(packet, error);
	if (rc != EOK)
		return icmp_release_and_return(packet, rc);

	return EOK;
}

/** Processes the generic client messages.
 *
 * @param[in] call	The message parameters.
 * @returns		EOK on success.
 * @returns		ENOTSUP if the message is not known.
 * @returns		Other error codes as defined for the packet_translate()
 *			function.
 * @returns		Other error codes as defined for the
 *			icmp_destination_unreachable_msg_local() function.
 * @returns		Other error codes as defined for the
 *			icmp_source_quench_msg_local() function.
 * @returns		Other error codes as defined for the
 *			icmp_time_exceeded_msg_local() function.
 * @returns		Other error codes as defined for the
 *			icmp_parameter_problem_msg_local() function.
 *
 * @see icmp_interface.h
 */
static int icmp_process_message(ipc_call_t *call)
{
	packet_t packet;
	int rc;

	switch (IPC_GET_METHOD(*call)) {
	case NET_ICMP_DEST_UNREACH:
		rc = packet_translate_remote(icmp_globals.net_phone, &packet,
		    IPC_GET_PACKET(call));
		if (rc != EOK)
			return rc;
		return icmp_destination_unreachable_msg_local(0,
		    ICMP_GET_CODE(call), ICMP_GET_MTU(call), packet);
	case NET_ICMP_SOURCE_QUENCH:
		rc = packet_translate_remote(icmp_globals.net_phone, &packet,
		    IPC_GET_PACKET(call));
		if (rc != EOK)
			return rc;
		return icmp_source_quench_msg_local(0, packet);
	case NET_ICMP_TIME_EXCEEDED:
		rc = packet_translate_remote(icmp_globals.net_phone, &packet,
		    IPC_GET_PACKET(call));
		if (rc != EOK)
			return rc;
		return icmp_time_exceeded_msg_local(0, ICMP_GET_CODE(call),
		    packet);
	case NET_ICMP_PARAMETERPROB:
		rc = packet_translate_remote(icmp_globals.net_phone, &packet,
		    IPC_GET_PACKET(call));
		if (rc != EOK)
			return rc;
		return icmp_parameter_problem_msg_local(0, ICMP_GET_CODE(call),
		    ICMP_GET_POINTER(call), packet);
	default:
		return ENOTSUP;
	}
}

/** Assigns a new identifier for the connection.
 *
 * Fills the echo data parameter with the assigned values.
 *
 * @param[in,out] echo_data The echo data to be bound.
 * @returns		Index of the inserted echo data.
 * @returns		EBADMEM if the echo_data parameter is NULL.
 * @returns		ENOTCONN if no free identifier have been found.
 */
static int icmp_bind_free_id(icmp_echo_t *echo_data)
{
	icmp_param_t index;

	if (!echo_data)
		return EBADMEM;

	/* From the last used one */
	index = icmp_globals.last_used_id;
	do {
		index++;
		/* til the range end */
		if (index >= ICMP_FREE_IDS_END) {
			/* start from the range beginning */
			index = ICMP_FREE_IDS_START - 1;
			do {
				index++;
				/* til the last used one */
				if (index >= icmp_globals.last_used_id) {
					/* none found */
					return ENOTCONN;
				}
			} while(icmp_echo_data_find(&icmp_globals.echo_data,
			    index) != NULL);

			/* Found, break immediately */
			break;
		}
	} while (icmp_echo_data_find(&icmp_globals.echo_data, index) != NULL);

	echo_data->identifier = index;
	echo_data->sequence_number = 0;

	return icmp_echo_data_add(&icmp_globals.echo_data, index, echo_data);
}

/** Processes the client messages.
 *
 * Remembers the assigned identifier and sequence numbers.
 * Runs until the client module disconnects.
 *
 * @param[in] callid	The message identifier.
 * @param[in] call	The message parameters.
 * @returns EOK.
 *
 * @see icmp_interface.h
 * @see icmp_api.h
 */
static int icmp_process_client_messages(ipc_callid_t callid, ipc_call_t call)
{
	bool keep_on_going = true;
	ipc_call_t answer;
	int answer_count;
	size_t length;
	struct sockaddr *addr;
	ipc_callid_t data_callid;
	icmp_echo_t *echo_data;
	int rc = EOK;

	/*
	 * Accept the connection
	 *  - Answer the first NET_ICMP_INIT call.
	 */
	answer_count = 0;

	echo_data = (icmp_echo_t *) malloc(sizeof(*echo_data));
	if (!echo_data)
		return ENOMEM;

	/* Assign a new identifier */
	fibril_rwlock_write_lock(&icmp_globals.lock);
	rc = icmp_bind_free_id(echo_data);
	fibril_rwlock_write_unlock(&icmp_globals.lock);
	if (rc < 0) {
		free(echo_data);
		return rc;
	}

	while (keep_on_going) {
		/* Answer the call */
		answer_call(callid, rc, &answer, answer_count);

		/* Refresh data */
		refresh_answer(&answer, &answer_count);

		/* Get the next call */
		callid = async_get_call(&call);

		/* Process the call */
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			keep_on_going = false;
			rc = EHANGUP;
			break;
		
		case NET_ICMP_ECHO:
			if (!async_data_write_receive(&data_callid, &length)) {
				rc = EINVAL;
				break;
			}
			
			addr = malloc(length);
			if (!addr) {
				rc = ENOMEM;
				break;
			}
			
			rc = async_data_write_finalize(data_callid, addr,
			    length);
			if (rc != EOK) {
				free(addr);
				break;
			}

			fibril_rwlock_write_lock(&icmp_globals.lock);
			rc = icmp_echo(echo_data->identifier,
			    echo_data->sequence_number, ICMP_GET_SIZE(call),
			    ICMP_GET_TIMEOUT(call), ICMP_GET_TTL(call),
			    ICMP_GET_TOS(call), ICMP_GET_DONT_FRAGMENT(call),
			    addr, (socklen_t) length);
			fibril_rwlock_write_unlock(&icmp_globals.lock);

			free(addr);

			if (echo_data->sequence_number < UINT16_MAX)
				echo_data->sequence_number++;
			else
				echo_data->sequence_number = 0;

			break;

		default:
			rc = icmp_process_message(&call);
		}

	}

	/* Release the identifier */
	fibril_rwlock_write_lock(&icmp_globals.lock);
	icmp_echo_data_exclude(&icmp_globals.echo_data, echo_data->identifier);
	fibril_rwlock_write_unlock(&icmp_globals.lock);

	return rc;
}

/** Processes the ICMP message.
 *
 * @param[in] callid	The message identifier.
 * @param[in] call	The message parameters.
 * @param[out] answer	The message answer parameters.
 * @param[out] answer_count The last parameter for the actual answer in the
 *			answer parameter.
 * @returns		EOK on success.
 * @returns		ENOTSUP if the message is not known.
 *
 * @see icmp_interface.h
 * @see IS_NET_ICMP_MESSAGE()
 */
int icmp_message_standalone(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, int *answer_count)
{
	packet_t packet;
	int rc;

	*answer_count = 0;
	switch (IPC_GET_METHOD(*call)) {
	case NET_TL_RECEIVED:
		rc = packet_translate_remote(icmp_globals.net_phone, &packet,
		    IPC_GET_PACKET(call));
		if (rc != EOK)
			return rc;
		return icmp_received_msg_local(IPC_GET_DEVICE(call), packet,
		    SERVICE_ICMP, IPC_GET_ERROR(call));
	
	case NET_ICMP_INIT:
		return icmp_process_client_messages(callid, * call);
	
	default:
		return icmp_process_message(call);
	}

	return ENOTSUP;
}


/** Default thread for new connections.
 *
 * @param[in] iid The initial message identifier.
 * @param[in] icall The initial message call structure.
 *
 */
static void tl_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	/*
	 * Accept the connection
	 *  - Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	ipc_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t answer;
		int answer_count;
		
		/* Clear the answer structure */
		refresh_answer(&answer, &answer_count);
		
		/* Fetch the next message */
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		/* Process the message */
		int res = tl_module_message_standalone(callid, &call, &answer,
		    &answer_count);
		
		/*
		 * End if told to either by the message or the processing
		 * result.
		 */
		if ((IPC_GET_METHOD(call) == IPC_M_PHONE_HUNGUP) ||
		    (res == EHANGUP))
			return;
		
		/* Answer the message */
		answer_call(callid, res, &answer, answer_count);
	}
}

/** Starts the module.
 *
 * @returns		EOK on success.
 * @returns		Other error codes as defined for each specific module
 *			start function.
 */
int main(int argc, char *argv[])
{
	int rc;
	
	/* Start the module */
	rc = tl_module_start_standalone(tl_client_connection);
	return rc;
}

/** @}
 */

