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
 */

#include <async.h>
#include <atomic.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <stdint.h>
#include <str.h>
#include <ipc/services.h>
#include <ipc/net.h>
#include <ipc/tl.h>
#include <ipc/icmp.h>
#include <sys/time.h>
#include <sys/types.h>
#include <byteorder.h>
#include <errno.h>
#include <adt/hash_table.h>

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
#include <icmp_remote.h>
#include <il_remote.h>
#include <ip_client.h>
#include <ip_interface.h>
#include <net_interface.h>
#include <tl_remote.h>
#include <tl_skel.h>
#include <icmp_header.h>

/** ICMP module name */
#define NAME  "icmp"

/** Number of replies hash table keys */
#define REPLY_KEYS  2

/** Number of replies hash table buckets */
#define REPLY_BUCKETS  1024

/**
 * Original datagram length in bytes transfered to the error
 * notification message.
 */
#define ICMP_KEEP_LENGTH  8

/** Compute the ICMP datagram checksum.
 *
 * @param[in,out] header ICMP datagram header.
 * @param[in]     length Total datagram length.
 *
 * @return Computed checksum.
 *
 */
#define ICMP_CHECKSUM(header, length) \
	htons(ip_checksum((uint8_t *) (header), (length)))

/** An echo request datagrams pattern. */
#define ICMP_ECHO_TEXT  "ICMP hello from HelenOS."

/** ICMP reply data. */
typedef struct {
	/** Hash table link */
	link_t link;
	
	/** Reply identification and sequence */
	icmp_param_t id;
	icmp_param_t sequence;
	
	/** Reply signaling */
	fibril_condvar_t condvar;
	
	/** Reply result */
	int result;
} icmp_reply_t;

/** Global data */
static async_sess_t *net_sess = NULL;
static async_sess_t *ip_sess = NULL;
static bool error_reporting = true;
static bool echo_replying = true;
static packet_dimension_t icmp_dimension;

/** ICMP client identification counter */
static atomic_t icmp_client;

/** ICMP identifier and sequence number (client-specific) */
static fibril_local icmp_param_t icmp_id;
static fibril_local icmp_param_t icmp_seq;

/** Reply hash table */
static fibril_mutex_t reply_lock;
static hash_table_t replies;

static hash_index_t replies_hash(unsigned long key[])
{
	/*
	 * ICMP identifier and sequence numbers
	 * are 16-bit values.
	 */
	hash_index_t index = ((key[0] & 0xffff) << 16) | (key[1] & 0xffff);
	return (index % REPLY_BUCKETS);
}

static int replies_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	icmp_reply_t *reply =
	    hash_table_get_instance(item, icmp_reply_t, link);
	
	if (keys == 1)
		return (reply->id == key[0]);
	else
		return ((reply->id == key[0]) && (reply->sequence == key[1]));
}

static void replies_remove_callback(link_t *item)
{
}

static hash_table_operations_t reply_ops = {
	.hash = replies_hash,
	.compare = replies_compare,
	.remove_callback = replies_remove_callback
};

/** Release the packet and return the result.
 *
 * @param[in] packet Packet queue to be released.
 *
 */
static void icmp_release(packet_t *packet)
{
	pq_release_remote(net_sess, packet_get_id(packet));
}

/** Send the ICMP message.
 *
 * Set the message type and code and compute the checksum.
 * Error messages are sent only if allowed in the configuration.
 * Release the packet on errors.
 *
 * @param[in] type          Message type.
 * @param[in] code          Message code.
 * @param[in] packet        Message packet to be sent.
 * @param[in] header        ICMP header.
 * @param[in] error         Error service to be announced. Should be
 *                          SERVICE_ICMP or zero.
 * @param[in] ttl           Time to live.
 * @param[in] tos           Type of service.
 * @param[in] dont_fragment Disable fragmentation.
 *
 * @return EOK on success.
 * @return EPERM if the error message is not allowed.
 *
 */
static int icmp_send_packet(icmp_type_t type, icmp_code_t code,
    packet_t *packet, icmp_header_t *header, services_t error, ip_ttl_t ttl,
    ip_tos_t tos, bool dont_fragment)
{
	/* Do not send an error if disabled */
	if ((error) && (!error_reporting)) {
		icmp_release(packet);
		return EPERM;
	}
	
	header->type = type;
	header->code = code;
	
	/*
	 * The checksum needs to be calculated
	 * with a virtual checksum field set to
	 * zero.
	 */
	header->checksum = 0;
	header->checksum = ICMP_CHECKSUM(header,
	    packet_get_data_length(packet));
	
	int rc = ip_client_prepare_packet(packet, IPPROTO_ICMP, ttl, tos,
	    dont_fragment, 0);
	if (rc != EOK) {
		icmp_release(packet);
		return rc;
	}
	
	return ip_send_msg(ip_sess, -1, packet, SERVICE_ICMP, error);
}

/** Prepare the ICMP error packet.
 *
 * Truncate the original packet if longer than ICMP_KEEP_LENGTH bytes.
 * Prefix and return the ICMP header.
 *
 * @param[in,out] packet Original packet.
 *
 * @return The prefixed ICMP header.
 * @return NULL on errors.
 *
 */
static icmp_header_t *icmp_prepare_packet(packet_t *packet)
{
	size_t total_length = packet_get_data_length(packet);
	if (total_length <= 0)
		return NULL;
	
	size_t header_length = ip_client_header_length(packet);
	if (header_length <= 0)
		return NULL;
	
	/* Truncate if longer than 64 bits (without the IP header) */
	if ((total_length > header_length + ICMP_KEEP_LENGTH) &&
	    (packet_trim(packet, 0,
	    total_length - header_length - ICMP_KEEP_LENGTH) != EOK))
		return NULL;
	
	icmp_header_t *header = PACKET_PREFIX(packet, icmp_header_t);
	if (!header)
		return NULL;
	
	bzero(header, sizeof(*header));
	return header;
}

/** Request an echo message.
 *
 * Send a packet with specified parameters to the target host
 * and wait for the reply upto the given timeout.
 * Block the caller until the reply or the timeout occurs.
 *
 * @param[in] id            Message identifier.
 * @param[in] sequence      Message sequence parameter.
 * @param[in] size          Message data length in bytes.
 * @param[in] timeout       Timeout in miliseconds.
 * @param[in] ttl           Time to live.
 * @param[in] tos           Type of service.
 * @param[in] dont_fragment Disable fragmentation.
 * @param[in] addr          Target host address.
 * @param[in] addrlen       Torget host address length.
 *
 * @return ICMP_ECHO on success.
 * @return ETIMEOUT if the reply has not arrived before the
 *         timeout.
 * @return ICMP type of the received error notification.
 * @return EINVAL if the addrlen parameter is less or equal to
 *         zero.
 * @return ENOMEM if there is not enough memory left.
 *
 */
static int icmp_echo(icmp_param_t id, icmp_param_t sequence, size_t size,
    mseconds_t timeout, ip_ttl_t ttl, ip_tos_t tos, bool dont_fragment,
    const struct sockaddr *addr, socklen_t addrlen)
{
	if (addrlen <= 0)
		return EINVAL;
	
	size_t length = (size_t) addrlen;
	
	packet_t *packet = packet_get_4_remote(net_sess, size,
	    icmp_dimension.addr_len, ICMP_HEADER_SIZE + icmp_dimension.prefix,
	    icmp_dimension.suffix);
	if (!packet)
		return ENOMEM;
	
	/* Prepare the requesting packet, set the destination address. */
	int rc = packet_set_addr(packet, NULL, (const uint8_t *) addr, length);
	if (rc != EOK) {
		icmp_release(packet);
		return rc;
	}
	
	/* Allocate space in the packet */
	uint8_t *data = (uint8_t *) packet_suffix(packet, size);
	if (!data) {
		icmp_release(packet);
		return ENOMEM;
	}
	
	/* Fill the data */
	length = 0;
	while (size > length + sizeof(ICMP_ECHO_TEXT)) {
		memcpy(data + length, ICMP_ECHO_TEXT, sizeof(ICMP_ECHO_TEXT));
		length += sizeof(ICMP_ECHO_TEXT);
	}
	memcpy(data + length, ICMP_ECHO_TEXT, size - length);
	
	/* Prefix the header */
	icmp_header_t *header = PACKET_PREFIX(packet, icmp_header_t);
	if (!header) {
		icmp_release(packet);
		return ENOMEM;
	}
	
	bzero(header, sizeof(icmp_header_t));
	header->un.echo.identifier = id;
	header->un.echo.sequence_number = sequence;
	
	/* Prepare the reply structure */
	icmp_reply_t *reply = malloc(sizeof(icmp_reply_t));
	if (!reply) {
		icmp_release(packet);
		return ENOMEM;
	}
	
	reply->id = id;
	reply->sequence = sequence;
	fibril_condvar_initialize(&reply->condvar);
	
	/* Add the reply to the replies hash table */
	fibril_mutex_lock(&reply_lock);
	
	unsigned long key[REPLY_KEYS] = {id, sequence};
	hash_table_insert(&replies, key, &reply->link);
	
	/* Send the request */
	icmp_send_packet(ICMP_ECHO, 0, packet, header, 0, ttl, tos,
	    dont_fragment);
	
	/* Wait for the reply. Timeout in microseconds. */
	rc = fibril_condvar_wait_timeout(&reply->condvar, &reply_lock,
	    timeout * 1000);
	if (rc == EOK)
		rc = reply->result;
	
	/* Remove the reply from the replies hash table */
	hash_table_remove(&replies, key, REPLY_KEYS);
	
	fibril_mutex_unlock(&reply_lock);
	
	free(reply);
	
	return rc;
}

static int icmp_destination_unreachable(icmp_code_t code, icmp_param_t mtu,
    packet_t *packet)
{
	icmp_header_t *header = icmp_prepare_packet(packet);
	if (!header) {
		icmp_release(packet);
		return ENOMEM;
	}
	
	if (mtu)
		header->un.frag.mtu = mtu;
	
	return icmp_send_packet(ICMP_DEST_UNREACH, code, packet, header,
	    SERVICE_ICMP, 0, 0, false);
}

static int icmp_source_quench(packet_t *packet)
{
	icmp_header_t *header = icmp_prepare_packet(packet);
	if (!header) {
		icmp_release(packet);
		return ENOMEM;
	}
	
	return icmp_send_packet(ICMP_SOURCE_QUENCH, 0, packet, header,
	    SERVICE_ICMP, 0, 0, false);
}

static int icmp_time_exceeded(icmp_code_t code, packet_t *packet)
{
	icmp_header_t *header = icmp_prepare_packet(packet);
	if (!header) {
		icmp_release(packet);
		return ENOMEM;
	}
	
	return icmp_send_packet(ICMP_TIME_EXCEEDED, code, packet, header,
	    SERVICE_ICMP, 0, 0, false);
}

static int icmp_parameter_problem(icmp_code_t code, icmp_param_t pointer,
    packet_t *packet)
{
	icmp_header_t *header = icmp_prepare_packet(packet);
	if (!header) {
		icmp_release(packet);
		return ENOMEM;
	}
	
	header->un.param.pointer = pointer;
	return icmp_send_packet(ICMP_PARAMETERPROB, code, packet, header,
	    SERVICE_ICMP, 0, 0, false);
}

/** Try to set the pending reply result as the received message type.
 *
 * If the reply data is not present, the reply timed out and the other fibril
 * is already awake. The packet is released.
 *
 * @param[in] packet The received reply message.
 * @param[in] header The ICMP message header.
 * @param[in] type   The received reply message type.
 * @param[in] code   The received reply message code.
 *
 */
static void icmp_process_echo_reply(packet_t *packet, icmp_header_t *header,
    icmp_type_t type, icmp_code_t code)
{
	unsigned long key[REPLY_KEYS] =
	    {header->un.echo.identifier, header->un.echo.sequence_number};
	
	/* The packet is no longer needed */
	icmp_release(packet);
	
	/* Find the pending reply */
	fibril_mutex_lock(&reply_lock);
	
	link_t *link = hash_table_find(&replies, key);
	if (link != NULL) {
		icmp_reply_t *reply =
		   hash_table_get_instance(link, icmp_reply_t, link);
		
		reply->result = type;
		fibril_condvar_signal(&reply->condvar);
	}
	
	fibril_mutex_unlock(&reply_lock);
}

/** Process the received ICMP packet.
 *
 * Notify the destination socket application.
 *
 * @param[in,out] packet Received packet.
 * @param[in]     error  Packet error reporting service to prefix
 *                       the received packet.
 *
 * @return EOK on success.
 * @return EINVAL if the packet is not valid.
 * @return EINVAL if the stored packet address is not the an_addr_t.
 * @return EINVAL if the packet does not contain any data.
 * @return NO_DATA if the packet content is shorter than the user
 *         datagram header.
 * @return ENOMEM if there is not enough memory left.
 * @return EADDRNOTAVAIL if the destination socket does not exist.
 * @return Other error codes as defined for the
 *         ip_client_process_packet() function.
 *
 */
static int icmp_process_packet(packet_t *packet, services_t error)
{
	icmp_type_t type;
	icmp_code_t code;
	int rc;
	
	switch (error) {
	case SERVICE_NONE:
		break;
	case SERVICE_ICMP:
		/* Process error */
		rc = icmp_client_process_packet(packet, &type, &code, NULL, NULL);
		if (rc < 0)
			return rc;
		
		/* Remove the error header */
		rc = packet_trim(packet, (size_t) rc, 0);
		if (rc != EOK)
			return rc;
		
		break;
	default:
		return ENOTSUP;
	}
	
	/* Get rid of the IP header */
	size_t length = ip_client_header_length(packet);
	rc = packet_trim(packet, length, 0);
	if (rc != EOK)
		return rc;
	
	length = packet_get_data_length(packet);
	if (length <= 0)
		return EINVAL;
	
	if (length < ICMP_HEADER_SIZE)
		return EINVAL;
	
	void *data = packet_get_data(packet);
	if (!data)
		return EINVAL;
	
	/* Get ICMP header */
	icmp_header_t *header = (icmp_header_t *) data;
	
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
		if (echo_replying) {
			uint8_t *src;
			int addrlen = packet_get_addr(packet, &src, NULL);
			
			/*
			 * Set both addresses to the source one (avoid the
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
		ip_received_error_msg(ip_sess, -1, packet,
		    SERVICE_IP, SERVICE_ICMP);
		return EOK;
	
	default:
		return ENOTSUP;
	}
}

/** Process IPC messages from the IP module
 *
 * @param[in]     iid   Message identifier.
 * @param[in,out] icall Message parameters.
 * @param[in]     arg   Local argument.
 *
 */
static void icmp_receiver(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	packet_t *packet;
	int rc;
	
	while (true) {
		if (!IPC_GET_IMETHOD(*icall))
			break;
		
		switch (IPC_GET_IMETHOD(*icall)) {
		case NET_TL_RECEIVED:
			rc = packet_translate_remote(net_sess, &packet,
			    IPC_GET_PACKET(*icall));
			if (rc == EOK) {
				rc = icmp_process_packet(packet, IPC_GET_ERROR(*icall));
				if (rc != EOK)
					icmp_release(packet);
			}
			
			async_answer_0(iid, (sysarg_t) rc);
			break;
		default:
			async_answer_0(iid, (sysarg_t) ENOTSUP);
		}
		
		iid = async_get_call(icall);
	}
}

/** Initialize the ICMP module.
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
			(uint8_t *) "ICMP_ERROR_REPORTING",
			20
		},
		{
			(uint8_t *) "ICMP_ECHO_REPLYING",
			18
		}
	};
	measured_string_t *configuration;
	size_t count = sizeof(names) / sizeof(measured_string_t);
	uint8_t *data;
	
	if (!hash_table_create(&replies, REPLY_BUCKETS, REPLY_KEYS, &reply_ops))
		return ENOMEM;
	
	fibril_mutex_initialize(&reply_lock);
	atomic_set(&icmp_client, 0);
	
	net_sess = sess;
	ip_sess = ip_bind_service(SERVICE_IP, IPPROTO_ICMP, SERVICE_ICMP,
	    icmp_receiver);
	if (ip_sess == NULL)
		return ENOENT;
	
	int rc = ip_packet_size_req(ip_sess, -1, &icmp_dimension);
	if (rc != EOK)
		return rc;
	
	icmp_dimension.prefix += ICMP_HEADER_SIZE;
	icmp_dimension.content -= ICMP_HEADER_SIZE;
	
	/* Get configuration */
	configuration = &names[0];
	rc = net_get_conf_req(net_sess, &configuration, count, &data);
	if (rc != EOK)
		return rc;
	
	if (configuration) {
		if (configuration[0].value)
			error_reporting = (configuration[0].value[0] == 'y');
		
		if (configuration[1].value)
			echo_replying = (configuration[1].value[0] == 'y');
		
		net_free_settings(configuration, data);
	}
	
	return EOK;
}

/** Per-connection initialization
 *
 * Initialize client-specific global variables.
 *
 */
void tl_connection(void)
{
	icmp_id = (icmp_param_t) atomic_postinc(&icmp_client);
	icmp_seq = 1;
}

/** Process the ICMP message.
 *
 * @param[in]  callid Message identifier.
 * @param[in]  call   Message parameters.
 * @param[out] answer Answer.
 * @param[out] count  Number of arguments of the answer.
 *
 * @return EOK on success.
 * @return ENOTSUP if the message is not known.
 * @return Other error codes as defined for the packet_translate()
 *         function.
 * @return Other error codes as defined for the
 *         icmp_destination_unreachable() function.
 * @return Other error codes as defined for the
 *         icmp_source_quench() function.
 * @return Other error codes as defined for the
 *         icmp_time_exceeded() function.
 * @return Other error codes as defined for the
 *         icmp_parameter_problem() function.
 *
 * @see icmp_remote.h
 * @see IS_NET_ICMP_MESSAGE()
 *
 */
int tl_message(ipc_callid_t callid, ipc_call_t *call,
    ipc_call_t *answer, size_t *count)
{
	struct sockaddr *addr;
	size_t size;
	packet_t *packet;
	int rc;
	
	*count = 0;
	
	switch (IPC_GET_IMETHOD(*call)) {
	case NET_ICMP_ECHO:
		rc = async_data_write_accept((void **) &addr, false, 0, 0, 0, &size);
		if (rc != EOK)
			return rc;
		
		rc = icmp_echo(icmp_id, icmp_seq, ICMP_GET_SIZE(*call),
		    ICMP_GET_TIMEOUT(*call), ICMP_GET_TTL(*call),
		    ICMP_GET_TOS(*call), ICMP_GET_DONT_FRAGMENT(*call),
		    addr, (socklen_t) size);
		
		free(addr);
		icmp_seq++;
		return rc;
	
	case NET_ICMP_DEST_UNREACH:
		rc = packet_translate_remote(net_sess, &packet,
		    IPC_GET_PACKET(*call));
		if (rc != EOK)
			return rc;
		
		return icmp_destination_unreachable(ICMP_GET_CODE(*call),
		    ICMP_GET_MTU(*call), packet);
	
	case NET_ICMP_SOURCE_QUENCH:
		rc = packet_translate_remote(net_sess, &packet,
		    IPC_GET_PACKET(*call));
		if (rc != EOK)
			return rc;
		
		return icmp_source_quench(packet);
	
	case NET_ICMP_TIME_EXCEEDED:
		rc = packet_translate_remote(net_sess, &packet,
		    IPC_GET_PACKET(*call));
		if (rc != EOK)
			return rc;
		
		return icmp_time_exceeded(ICMP_GET_CODE(*call), packet);
	
	case NET_ICMP_PARAMETERPROB:
		rc = packet_translate_remote(net_sess, &packet,
		    IPC_GET_PACKET(*call));
		if (rc != EOK)
			return rc;
		
		return icmp_parameter_problem(ICMP_GET_CODE(*call),
		    ICMP_GET_POINTER(*call), packet);
	}
	
	return ENOTSUP;
}

int main(int argc, char *argv[])
{
	/* Start the module */
	return tl_module_start(SERVICE_ICMP);
}

/** @}
 */
