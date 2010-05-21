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

/** @addtogroup net_tl
 *  @{
 */

/** @file
 *  Transport layer common functions implementation.
 *  @see tl_common.h
 */

#include <async.h>
#include <ipc/services.h>

#include <net_err.h>
#include <packet/packet.h>
#include <packet/packet_client.h>
#include <packet_remote.h>
#include <net_device.h>
#include <icmp_interface.h>
#include <in.h>
#include <in6.h>
#include <inet.h>
#include <ip_local.h>
#include <ip_remote.h>
#include <socket_codes.h>
#include <socket_errno.h>
#include <ip_interface.h>
#include <tl_interface.h>
#include <tl_common.h>

DEVICE_MAP_IMPLEMENT(packet_dimensions, packet_dimension_t);

int tl_get_address_port(const struct sockaddr * addr, int addrlen, uint16_t * port){
	const struct sockaddr_in * address_in;
	const struct sockaddr_in6 * address_in6;

	if((addrlen <= 0) || ((size_t) addrlen < sizeof(struct sockaddr))){
		return EINVAL;
	}
	switch(addr->sa_family){
		case AF_INET:
			if(addrlen != sizeof(struct sockaddr_in)){
				return EINVAL;
			}
			address_in = (struct sockaddr_in *) addr;
			*port = ntohs(address_in->sin_port);
			break;
		case AF_INET6:
			if(addrlen != sizeof(struct sockaddr_in6)){
				return EINVAL;
			}
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
 * @param[in]  ip_phone          The IP moduel phone for (semi)remote calls.
 * @param[in]  packet_dimensions The packet dimensions cache.
 * @param[in]  device_id         The device identifier.
 * @param[out] packet_dimension  The IP packet dimensions.
 *
 * @return EOK on success.
 * @return EBADMEM if the packet_dimension parameter is NULL.
 * @return ENOMEM if there is not enough memory left.
 * @return EINVAL if the packet_dimensions cache is not valid.
 * @return Other codes as defined for the ip_packet_size_req() function.
 *
 */
int tl_get_ip_packet_dimension(int ip_phone,
    packet_dimensions_ref packet_dimensions, device_id_t device_id,
    packet_dimension_ref *packet_dimension)
{
	ERROR_DECLARE;
	
	if (!packet_dimension)
		return EBADMEM;
	
	*packet_dimension = packet_dimensions_find(packet_dimensions, device_id);
	if (!*packet_dimension) {
		/* Ask for and remember them if not found */
		*packet_dimension = malloc(sizeof(**packet_dimension));
		if(!*packet_dimension)
			return ENOMEM;
		
		if (ERROR_OCCURRED(ip_packet_size_req(ip_phone, device_id,
		    *packet_dimension))) {
			free(*packet_dimension);
			return ERROR_CODE;
		}
		
		ERROR_CODE = packet_dimensions_add(packet_dimensions, device_id,
		    *packet_dimension);
		if (ERROR_CODE < 0) {
			free(*packet_dimension);
			return ERROR_CODE;
		}
	}
	
	return EOK;
}

int tl_update_ip_packet_dimension(packet_dimensions_ref packet_dimensions, device_id_t device_id, size_t content){
	packet_dimension_ref packet_dimension;

	packet_dimension = packet_dimensions_find(packet_dimensions, device_id);
	if(! packet_dimension){
		return ENOENT;
	}
	packet_dimension->content = content;
	if(device_id != DEVICE_INVALID_ID){
		packet_dimension = packet_dimensions_find(packet_dimensions, DEVICE_INVALID_ID);
		if(packet_dimension){
			if(packet_dimension->content >= content){
				packet_dimension->content = content;
			}else{
				packet_dimensions_exclude(packet_dimensions, DEVICE_INVALID_ID);
			}
		}
	}
	return EOK;
}

int tl_set_address_port(struct sockaddr * addr, int addrlen, uint16_t port){
	struct sockaddr_in * address_in;
	struct sockaddr_in6 * address_in6;
	size_t length;

	if(addrlen < 0){
		return EINVAL;
	}
	length = (size_t) addrlen;
	if(length < sizeof(struct sockaddr)){
		return EINVAL;
	}
	switch(addr->sa_family){
		case AF_INET:
			if(length != sizeof(struct sockaddr_in)){
				return EINVAL;
			}
			address_in = (struct sockaddr_in *) addr;
			address_in->sin_port = htons(port);
			return EOK;
		case AF_INET6:
			if(length != sizeof(struct sockaddr_in6)){
				return EINVAL;
			}
			address_in6 = (struct sockaddr_in6 *) addr;
			address_in6->sin6_port = htons(port);
			return EOK;
		default:
			return EAFNOSUPPORT;
	}
}

int tl_prepare_icmp_packet(int packet_phone, int icmp_phone, packet_t packet, services_t error){
	packet_t next;
	uint8_t * src;
	int length;

	// detach the first packet and release the others
	next = pq_detach(packet);
	if (next)
		pq_release_remote(packet_phone, packet_get_id(next));
	
	length = packet_get_addr(packet, &src, NULL);
	if((length > 0)
		&& (! error)
		&& (icmp_phone >= 0)
	// set both addresses to the source one (avoids the source address deletion before setting the destination one)
		&& (packet_set_addr(packet, src, src, (size_t) length) == EOK)){
		return EOK;
	}else{
		pq_release_remote(packet_phone, packet_get_id(packet));
	}
	return ENOENT;
}

int tl_socket_read_packet_data(int packet_phone, packet_ref packet, size_t prefix, const packet_dimension_ref dimension, const struct sockaddr * addr, socklen_t addrlen){
	ERROR_DECLARE;

	ipc_callid_t callid;
	size_t length;
	void * data;

	if(! dimension){
		return EINVAL;
	}
	// get the data length
	if(! async_data_write_receive(&callid, &length)){
		return EINVAL;
	}
	// get a new packet
	*packet = packet_get_4_remote(packet_phone, length, dimension->addr_len, prefix + dimension->prefix, dimension->suffix);
	if(! packet){
		return ENOMEM;
	}
	// allocate space in the packet
	data = packet_suffix(*packet, length);
	if(! data){
		pq_release_remote(packet_phone, packet_get_id(*packet));
		return ENOMEM;
	}
	// read the data into the packet
	if(ERROR_OCCURRED(async_data_write_finalize(callid, data, length))
	// set the packet destination address
		|| ERROR_OCCURRED(packet_set_addr(*packet, NULL, (uint8_t *) addr, addrlen))){
		pq_release_remote(packet_phone, packet_get_id(*packet));
		return ERROR_CODE;
	}
	return (int) length;
}

/** @}
 */
