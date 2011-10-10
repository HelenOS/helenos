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

/** @addtogroup libnet
 * @{
 */

/** @file
 * Transport layer common functions implementation.
 * @see tl_common.h
 */

#include <tl_common.h>
#include <packet_client.h>
#include <packet_remote.h>
#include <icmp_remote.h>
#include <ip_remote.h>
#include <ip_interface.h>
#include <tl_remote.h>
#include <net/socket_codes.h>
#include <net/in.h>
#include <net/in6.h>
#include <net/inet.h>
#include <net/device.h>
#include <net/packet.h>
#include <async.h>
#include <ipc/services.h>
#include <errno.h>

DEVICE_MAP_IMPLEMENT(packet_dimensions, packet_dimension_t);

/** Gets the address port.
 *
 * Supports AF_INET and AF_INET6 address families.
 *
 * @param[in,out] addr	The address to be updated.
 * @param[in] addrlen	The address length.
 * @param[out] port	The set port.
 * @return		EOK on success.
 * @return		EINVAL if the address length does not match the address
 *			family.
 * @return		EAFNOSUPPORT if the address family is not supported.
 */
int
tl_get_address_port(const struct sockaddr *addr, int addrlen, uint16_t *port)
{
	const struct sockaddr_in *address_in;
	const struct sockaddr_in6 *address_in6;

	if ((addrlen <= 0) || ((size_t) addrlen < sizeof(struct sockaddr)))
		return EINVAL;

	switch (addr->sa_family) {
	case AF_INET:
		if (addrlen != sizeof(struct sockaddr_in))
			return EINVAL;

		address_in = (struct sockaddr_in *) addr;
		*port = ntohs(address_in->sin_port);
		break;
	
	case AF_INET6:
		if (addrlen != sizeof(struct sockaddr_in6))
			return EINVAL;

		address_in6 = (struct sockaddr_in6 *) addr;
		*port = ntohs(address_in6->sin6_port);
		break;
	
	default:
		return EAFNOSUPPORT;
	}

	return EOK;
}

/** Get IP packet dimensions.
 *
 * Try to search a cache and query the IP module if not found.
 * The reply is cached then.
 *
 * @param[in]  sess              IP module session.
 * @param[in]  packet_dimensions Packet dimensions cache.
 * @param[in]  device_id         Device identifier.
 * @param[out] packet_dimension  IP packet dimensions.
 *
 * @return EOK on success.
 * @return EBADMEM if the packet_dimension parameter is NULL.
 * @return ENOMEM if there is not enough memory left.
 * @return EINVAL if the packet_dimensions cache is not valid.
 * @return Other codes as defined for the ip_packet_size_req()
 *         function.
 *
 */
int tl_get_ip_packet_dimension(async_sess_t *sess,
    packet_dimensions_t *packet_dimensions, nic_device_id_t device_id,
    packet_dimension_t **packet_dimension)
{
	if (!packet_dimension)
		return EBADMEM;
	
	*packet_dimension = packet_dimensions_find(packet_dimensions,
	    device_id);
	if (!*packet_dimension) {
		/* Ask for and remember them if not found */
		*packet_dimension = malloc(sizeof(**packet_dimension));
		if (!*packet_dimension)
			return ENOMEM;
		
		int rc = ip_packet_size_req(sess, device_id, *packet_dimension);
		if (rc != EOK) {
			free(*packet_dimension);
			return rc;
		}
		
		rc = packet_dimensions_add(packet_dimensions, device_id,
		    *packet_dimension);
		if (rc < 0) {
			free(*packet_dimension);
			return rc;
		}
	}
	
	return EOK;
}

/** Updates IP device packet dimensions cache.
 *
 * @param[in,out] packet_dimensions The packet dimensions cache.
 * @param[in] device_id	The device identifier.
 * @param[in] content	The new maximum content size.
 * @return		EOK on success.
 * @return		ENOENT if the packet dimension is not cached.
 */
int
tl_update_ip_packet_dimension(packet_dimensions_t *packet_dimensions,
    nic_device_id_t device_id, size_t content)
{
	packet_dimension_t *packet_dimension;

	packet_dimension = packet_dimensions_find(packet_dimensions, device_id);
	if (!packet_dimension)
		return ENOENT;

	packet_dimension->content = content;

	if (device_id != NIC_DEVICE_INVALID_ID) {
		packet_dimension = packet_dimensions_find(packet_dimensions,
		    NIC_DEVICE_INVALID_ID);

		if (packet_dimension) {
			if (packet_dimension->content >= content)
				packet_dimension->content = content;
			else
				packet_dimensions_exclude(packet_dimensions,
				    NIC_DEVICE_INVALID_ID, free);
		}
	}

	return EOK;
}

/** Sets the address port.
 *
 * Supports AF_INET and AF_INET6 address families.
 *
 * @param[in,out] addr	The address to be updated.
 * @param[in] addrlen	The address length.
 * @param[in] port	The port to be set.
 * @return		EOK on success.
 * @return		EINVAL if the address length does not match the address
 *			family.
 * @return		EAFNOSUPPORT if the address family is not supported.
 */
int tl_set_address_port(struct sockaddr * addr, int addrlen, uint16_t port)
{
	struct sockaddr_in *address_in;
	struct sockaddr_in6 *address_in6;
	size_t length;

	if (addrlen < 0)
		return EINVAL;
	
	length = (size_t) addrlen;
	if (length < sizeof(struct sockaddr))
		return EINVAL;

	switch (addr->sa_family) {
	case AF_INET:
		if (length != sizeof(struct sockaddr_in))
			return EINVAL;
		address_in = (struct sockaddr_in *) addr;
		address_in->sin_port = htons(port);
		return EOK;
	
	case AF_INET6:
		if (length != sizeof(struct sockaddr_in6))
				return EINVAL;
		address_in6 = (struct sockaddr_in6 *) addr;
		address_in6->sin6_port = htons(port);
		return EOK;
	
	default:
		return EAFNOSUPPORT;
	}
}

/** Prepares the packet for ICMP error notification.
 *
 * Keep the first packet and release all the others.
 * Release all the packets on error.
 *
 * @param[in] packet_sess Packet server module session.
 * @param[in] icmp_sess   ICMP module phone.
 * @param[in] packet      Packet to be send.
 * @param[in] error       Packet error reporting service. Prefixes the
 *                        received packet.
 *
 * @return EOK on success.
 * @return ENOENT if no packet may be sent.
 *
 */
int tl_prepare_icmp_packet(async_sess_t *packet_sess, async_sess_t *icmp_sess,
    packet_t *packet, services_t error)
{
	/* Detach the first packet and release the others */
	packet_t *next = pq_detach(packet);
	if (next)
		pq_release_remote(packet_sess, packet_get_id(next));
	
	uint8_t *src;
	int length = packet_get_addr(packet, &src, NULL);
	if ((length > 0) && (!error) && (icmp_sess) &&
	    /*
	     * Set both addresses to the source one (avoids the source address
	     * deletion before setting the destination one)
	     */
	    (packet_set_addr(packet, src, src, (size_t) length) == EOK)) {
		return EOK;
	} else
		pq_release_remote(packet_sess, packet_get_id(packet));
	
	return ENOENT;
}

/** Receives data from the socket into a packet.
 *
 * @param[in]  sess      Packet server module session.
 * @param[out] packet    New created packet.
 * @param[in]  prefix    Reserved packet data prefix length.
 * @param[in]  dimension Packet dimension.
 * @param[in]  addr      Destination address.
 * @param[in]  addrlen   Address length.
 *
 * @return Number of bytes received.
 * @return EINVAL if the client does not send data.
 * @return ENOMEM if there is not enough memory left.
 * @return Other error codes as defined for the
 *         async_data_read_finalize() function.
 *
 */
int tl_socket_read_packet_data(async_sess_t *sess, packet_t **packet,
    size_t prefix, const packet_dimension_t *dimension,
    const struct sockaddr *addr, socklen_t addrlen)
{
	ipc_callid_t callid;
	size_t length;
	void *data;
	int rc;

	if (!dimension)
		return EINVAL;

	/* Get the data length */
	if (!async_data_write_receive(&callid, &length))
		return EINVAL;

	/* Get a new packet */
	*packet = packet_get_4_remote(sess, length, dimension->addr_len,
	    prefix + dimension->prefix, dimension->suffix);
	if (!packet)
		return ENOMEM;

	/* Allocate space in the packet */
	data = packet_suffix(*packet, length);
	if (!data) {
		pq_release_remote(sess, packet_get_id(*packet));
		return ENOMEM;
	}

	/* Read the data into the packet */
	rc = async_data_write_finalize(callid, data, length);
	if (rc != EOK) {
		pq_release_remote(sess, packet_get_id(*packet));
		return rc;
	}
	
	/* Set the packet destination address */
	rc = packet_set_addr(*packet, NULL, (uint8_t *) addr, addrlen);
	if (rc != EOK) {
		pq_release_remote(sess, packet_get_id(*packet));
		return rc;
	}

	return (int) length;
}

/** @}
 */
