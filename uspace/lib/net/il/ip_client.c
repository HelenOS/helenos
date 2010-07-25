/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup ip
 *  @{
 */

/** @file
 *  IP client interface implementation.
 *  @see ip_client.h
 */

#include <errno.h>
#include <sys/types.h>

#include <ip_client.h>
#include <socket_errno.h>
#include <packet/packet.h>
#include <packet/packet_client.h>
#include <ip_header.h>

size_t ip_client_header_length(packet_t packet){
	ip_header_ref header;

	header = (ip_header_ref) packet_get_data(packet);
	if((! header)
		|| (packet_get_data_length(packet) < sizeof(ip_header_t))){
		return 0;
	}
	return IP_HEADER_LENGTH(header);
}

int ip_client_get_pseudo_header(ip_protocol_t protocol, struct sockaddr * src, socklen_t srclen, struct sockaddr * dest, socklen_t destlen, size_t data_length, void **header, size_t * headerlen){
	ipv4_pseudo_header_ref header_in;
	struct sockaddr_in * address_in;

	if(!(header && headerlen)){
		return EBADMEM;
	}
	if(!(src && dest && (srclen > 0) && ((size_t) srclen >= sizeof(struct sockaddr)) && (srclen == destlen) && (src->sa_family == dest->sa_family))){
		return EINVAL;
	}

	switch(src->sa_family){
		case AF_INET:
			if(srclen != sizeof(struct sockaddr_in)){
				return EINVAL;
			}
			*headerlen = sizeof(*header_in);
			header_in = (ipv4_pseudo_header_ref) malloc(*headerlen);
			if(! header_in){
				return ENOMEM;
			}
			bzero(header_in, * headerlen);
			address_in = (struct sockaddr_in *) dest;
			header_in->destination_address = address_in->sin_addr.s_addr;
			address_in = (struct sockaddr_in *) src;
			header_in->source_address = address_in->sin_addr.s_addr;
			header_in->protocol = protocol;
			header_in->data_length = htons(data_length);
			*header = header_in;
			return EOK;
		// TODO IPv6
/*		case AF_INET6:
			if(addrlen != sizeof(struct sockaddr_in6)){
				return EINVAL;
			}
			address_in6 = (struct sockaddr_in6 *) addr;
			return EOK;
*/		default:
			return EAFNOSUPPORT;
	}
}

int ip_client_prepare_packet(packet_t packet, ip_protocol_t protocol, ip_ttl_t ttl, ip_tos_t tos, int dont_fragment, size_t ipopt_length){
	ip_header_ref header;
	uint8_t * data;
	size_t padding;

	// compute the padding if IP options are set
	// multiple of 4 bytes
	padding =  ipopt_length % 4;
	if(padding){
		padding = 4 - padding;
		ipopt_length += padding;
	}

	// prefix the header
	data = (uint8_t *) packet_prefix(packet, sizeof(ip_header_t) + padding);
	if(! data){
		return ENOMEM;
	}

	// add the padding
	while(padding --){
		data[sizeof(ip_header_t) + padding] = IPOPT_NOOP;
	}

	// set the header
	header = (ip_header_ref) data;
	header->header_length = IP_COMPUTE_HEADER_LENGTH(sizeof(ip_header_t) + ipopt_length);
	header->ttl = (ttl ? ttl : IPDEFTTL); //(((ttl) <= MAXTTL) ? ttl : MAXTTL) : IPDEFTTL;
	header->tos = tos;
	header->protocol = protocol;

	if(dont_fragment){
		header->flags = IPFLAG_DONT_FRAGMENT;
	}
	return EOK;
}

int ip_client_process_packet(packet_t packet, ip_protocol_t * protocol, ip_ttl_t * ttl, ip_tos_t * tos, int * dont_fragment, size_t * ipopt_length){
	ip_header_ref header;

	header = (ip_header_ref) packet_get_data(packet);
	if((! header)
		|| (packet_get_data_length(packet) < sizeof(ip_header_t))){
		return ENOMEM;
	}

	if(protocol){
		*protocol = header->protocol;
	}
	if(ttl){
		*ttl = header->ttl;
	}
	if(tos){
		*tos = header->tos;
	}
	if(dont_fragment){
		*dont_fragment = header->flags &IPFLAG_DONT_FRAGMENT;
	}
	if(ipopt_length){
		*ipopt_length = IP_HEADER_LENGTH(header) - sizeof(ip_header_t);
		return sizeof(ip_header_t);
	}else{
		return IP_HEADER_LENGTH(header);
	}
}

int ip_client_set_pseudo_header_data_length(void *header, size_t headerlen, size_t data_length){
	ipv4_pseudo_header_ref header_in;

	if(! header){
		return EBADMEM;
	}

	if(headerlen == sizeof(ipv4_pseudo_header_t)){
		header_in = (ipv4_pseudo_header_ref) header;
		header_in->data_length = htons(data_length);
		return EOK;
	// TODO IPv6
	}else{
		return EINVAL;
	}
}

/** @}
 */
