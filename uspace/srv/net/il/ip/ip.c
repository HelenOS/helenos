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
 * @{
 */

/** @file
 * IP module implementation.
 * @see arp.h
 */

#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <str.h>
#include <ipc/services.h>
#include <ipc/net.h>
#include <ipc/nil.h>
#include <ipc/il.h>
#include <ipc/ip.h>
#include <sys/types.h>
#include <byteorder.h>
#include "ip.h"

#include <adt/measured_strings.h>
#include <adt/module_map.h>

#include <packet_client.h>
#include <net/socket_codes.h>
#include <net/in.h>
#include <net/in6.h>
#include <net/inet.h>
#include <net/modules.h>
#include <net/device.h>
#include <net/packet.h>
#include <net/icmp_codes.h>

#include <arp_interface.h>
#include <net_checksum.h>
#include <icmp_client.h>
#include <icmp_remote.h>
#include <ip_client.h>
#include <ip_interface.h>
#include <ip_header.h>
#include <net_interface.h>
#include <nil_remote.h>
#include <tl_remote.h>
#include <packet_remote.h>
#include <il_remote.h>
#include <il_skel.h>

/** IP module name. */
#define NAME			"ip"

/** IP version 4. */
#define IPV4			4

/** Default network interface IP version. */
#define NET_DEFAULT_IPV		IPV4

/** Default network interface IP routing. */
#define NET_DEFAULT_IP_ROUTING	false

/** Minimum IP packet content. */
#define IP_MIN_CONTENT		576

/** ARP module name. */
#define ARP_NAME		"arp"

/** ARP module filename. */
#define ARP_FILENAME		"/srv/arp"

/** IP packet address length. */
#define IP_ADDR			sizeof(struct sockaddr_in6)

/** IP packet prefix length. */
#define IP_PREFIX		sizeof(ip_header_t)

/** IP packet suffix length. */
#define IP_SUFFIX		0

/** IP packet maximum content length. */
#define IP_MAX_CONTENT		65535

/** The IP localhost address. */
#define IPV4_LOCALHOST_ADDRESS	htonl((127 << 24) + 1)

/** IP global data. */
ip_globals_t ip_globals;

DEVICE_MAP_IMPLEMENT(ip_netifs, ip_netif_t);
INT_MAP_IMPLEMENT(ip_protos, ip_proto_t);
GENERIC_FIELD_IMPLEMENT(ip_routes, ip_route_t);

static void ip_receiver(ipc_callid_t, ipc_call_t *, void *);

/** Release the packet and returns the result.
 *
 * @param[in] packet Packet queue to be released.
 * @param[in] result Result to be returned.
 *
 * @return Result parameter.
 *
 */
static int ip_release_and_return(packet_t *packet, int result)
{
	pq_release_remote(ip_globals.net_sess, packet_get_id(packet));
	return result;
}

/** Return the ICMP session.
 *
 * Search the registered protocols.
 *
 * @return Found ICMP session.
 * @return NULL if the ICMP is not registered.
 *
 */
static async_sess_t *ip_get_icmp_session(void)
{
	fibril_rwlock_read_lock(&ip_globals.protos_lock);
	ip_proto_t *proto = ip_protos_find(&ip_globals.protos, IPPROTO_ICMP);
	async_sess_t *sess = proto ? proto->sess : NULL;
	fibril_rwlock_read_unlock(&ip_globals.protos_lock);
	
	return sess;
}

/** Prepares the ICMP notification packet.
 *
 * Releases additional packets and keeps only the first one.
 *
 * @param[in] packet	The packet or the packet queue to be reported as faulty.
 * @param[in] header	The first packet IP header. May be NULL.
 * @return		EOK on success.
 * @return		EINVAL if there are no data in the packet.
 * @return		EINVAL if the packet is a fragment.
 * @return		ENOMEM if the packet is too short to contain the IP
 *			header.
 * @return		EAFNOSUPPORT if the address family is not supported.
 * @return		EPERM if the protocol is not allowed to send ICMP
 *			notifications. The ICMP protocol itself.
 * @return		Other error codes as defined for the packet_set_addr().
 */
static int ip_prepare_icmp(packet_t *packet, ip_header_t *header)
{
	packet_t *next;
	struct sockaddr *dest;
	struct sockaddr_in dest_in;
	socklen_t addrlen;

	/* Detach the first packet and release the others */
	next = pq_detach(packet);
	if (next)
		pq_release_remote(ip_globals.net_sess, packet_get_id(next));

	if (!header) {
		if (packet_get_data_length(packet) <= sizeof(ip_header_t))
			return ENOMEM;

		/* Get header */
		header = (ip_header_t *) packet_get_data(packet);
		if (!header)
			return EINVAL;

	}

	/* Only for the first fragment */
	if (IP_FRAGMENT_OFFSET(header))
		return EINVAL;

	/* Not for the ICMP protocol */
	if (header->protocol == IPPROTO_ICMP)
		return EPERM;

	/* Set the destination address */
	switch (GET_IP_HEADER_VERSION(header)) {
	case IPVERSION:
		addrlen = sizeof(dest_in);
		bzero(&dest_in, addrlen);
		dest_in.sin_family = AF_INET;
		memcpy(&dest_in.sin_addr.s_addr, &header->source_address,
		    sizeof(header->source_address));
		dest = (struct sockaddr *) &dest_in;
		break;

	default:
		return EAFNOSUPPORT;
	}

	return packet_set_addr(packet, NULL, (uint8_t *) dest, addrlen);
}

/** Prepare the ICMP notification packet.
 *
 * Release additional packets and keep only the first one.
 * All packets are released on error.
 *
 * @param[in] error  Packet error service.
 * @param[in] packet Packet or the packet queue to be reported as faulty.
 * @param[in] header First packet IP header. May be NULL.
 *
 * @return Found ICMP session.
 * @return NULL if the error parameter is set.
 * @return NULL if the ICMP session is not found.
 * @return NULL if the ip_prepare_icmp() fails.
 *
 */
static async_sess_t *ip_prepare_icmp_and_get_session(services_t error,
    packet_t *packet, ip_header_t *header)
{
	async_sess_t *sess = ip_get_icmp_session();
	
	if ((error) || (!sess) || (ip_prepare_icmp(packet, header))) {
		pq_release_remote(ip_globals.net_sess, packet_get_id(packet));
		return NULL;
	}
	
	return sess;
}

int il_initialize(async_sess_t *net_sess)
{
	fibril_rwlock_initialize(&ip_globals.lock);
	fibril_rwlock_write_lock(&ip_globals.lock);
	fibril_rwlock_initialize(&ip_globals.protos_lock);
	fibril_rwlock_initialize(&ip_globals.netifs_lock);
	
	ip_globals.net_sess = net_sess;
	ip_globals.packet_counter = 0;
	ip_globals.gateway.address.s_addr = 0;
	ip_globals.gateway.netmask.s_addr = 0;
	ip_globals.gateway.gateway.s_addr = 0;
	ip_globals.gateway.netif = NULL;
	
	int rc = ip_netifs_initialize(&ip_globals.netifs);
	if (rc != EOK)
		goto out;
	rc = ip_protos_initialize(&ip_globals.protos);
	if (rc != EOK)
		goto out;
	rc = modules_initialize(&ip_globals.modules);
	if (rc != EOK)
		goto out;
	rc = add_module(NULL, &ip_globals.modules, (uint8_t *) ARP_NAME,
	    (uint8_t *) ARP_FILENAME, SERVICE_ARP, 0, arp_connect_module);

out:
	fibril_rwlock_write_unlock(&ip_globals.lock);

	return rc;
}

/** Initializes a new network interface specific data.
 *
 * Connects to the network interface layer module, reads the netif
 * configuration, starts an ARP module if needed and sets the netif routing
 * table.
 * 
 * The device identifier and the nil service has to be set.
 *
 * @param[in,out] ip_netif Network interface specific data.
 * @return		EOK on success.
 * @return		ENOTSUP if DHCP is configured.
 * @return		ENOTSUP if IPv6 is configured.
 * @return		EINVAL if any of the addresses is invalid.
 * @return		EINVAL if the used ARP module is not known.
 * @return		ENOMEM if there is not enough memory left.
 * @return		Other error codes as defined for the
 *			net_get_device_conf_req() function.
 * @return		Other error codes as defined for the bind_service()
 *			function.
 * @return		Other error codes as defined for the specific
 *			arp_device_req() function.
 * @return		Other error codes as defined for the
 *			nil_packet_size_req() function.
 */
static int ip_netif_initialize(ip_netif_t *ip_netif)
{
	measured_string_t names[] = {
		{
			(uint8_t *) "IPV",
			3
		},
		{
			(uint8_t *) "IP_CONFIG",
			9
		},
		{
			(uint8_t *) "IP_ADDR",
			7
		},
		{
			(uint8_t *) "IP_NETMASK",
			10
		},
		{
			(uint8_t *) "IP_GATEWAY",
			10
		},
		{
			(uint8_t *) "IP_BROADCAST",
			12
		},
		{
			(uint8_t *) "ARP",
			3
		},
		{
			(uint8_t *) "IP_ROUTING",
			10
		}
	};
	measured_string_t *configuration;
	size_t count = sizeof(names) / sizeof(measured_string_t);
	uint8_t *data;
	measured_string_t address;
	ip_route_t *route;
	in_addr_t gateway;
	int index;
	int rc;

	ip_netif->arp = NULL;
	route = NULL;
	ip_netif->ipv = NET_DEFAULT_IPV;
	ip_netif->dhcp = false;
	ip_netif->routing = NET_DEFAULT_IP_ROUTING;
	configuration = &names[0];
	
	/* Get configuration */
	rc = net_get_device_conf_req(ip_globals.net_sess, ip_netif->device_id,
	    &configuration, count, &data);
	if (rc != EOK)
		return rc;
	
	if (configuration) {
		if (configuration[0].value)
			ip_netif->ipv = strtol((char *) configuration[0].value, NULL, 0);
		
		ip_netif->dhcp = !str_lcmp((char *) configuration[1].value, "dhcp",
		    configuration[1].length);
		
		if (ip_netif->dhcp) {
			// TODO dhcp
			net_free_settings(configuration, data);
			return ENOTSUP;
		} else if (ip_netif->ipv == IPV4) {
			route = (ip_route_t *) malloc(sizeof(ip_route_t));
			if (!route) {
				net_free_settings(configuration, data);
				return ENOMEM;
			}
			route->address.s_addr = 0;
			route->netmask.s_addr = 0;
			route->gateway.s_addr = 0;
			route->netif = ip_netif;
			index = ip_routes_add(&ip_netif->routes, route);
			if (index < 0) {
				net_free_settings(configuration, data);
				free(route);
				return index;
			}
			
			if ((inet_pton(AF_INET, (char *) configuration[2].value,
			    (uint8_t *) &route->address.s_addr) != EOK) ||
			    (inet_pton(AF_INET, (char *) configuration[3].value,
			    (uint8_t *) &route->netmask.s_addr) != EOK) ||
			    (inet_pton(AF_INET, (char *) configuration[4].value,
			    (uint8_t *) &gateway.s_addr) == EINVAL) ||
			    (inet_pton(AF_INET, (char *) configuration[5].value,
			    (uint8_t *) &ip_netif->broadcast.s_addr) == EINVAL))
			    {
				net_free_settings(configuration, data);
				return EINVAL;
			}
		} else {
			// TODO ipv6 in separate module
			net_free_settings(configuration, data);
			return ENOTSUP;
		}
		
		if (configuration[6].value) {
			ip_netif->arp = get_running_module(&ip_globals.modules,
			    configuration[6].value);
			if (!ip_netif->arp) {
				printf("Failed to start the arp %s\n",
				    configuration[6].value);
				net_free_settings(configuration, data);
				return EINVAL;
			}
		}
		
		if (configuration[7].value)
			ip_netif->routing = (configuration[7].value[0] == 'y');
		
		net_free_settings(configuration, data);
	}
	
	/* Bind netif service which also initializes the device */
	ip_netif->sess = nil_bind_service(ip_netif->service,
	    (sysarg_t) ip_netif->device_id, SERVICE_IP,
	    ip_receiver);
	if (ip_netif->sess == NULL) {
		printf("Failed to contact the nil service %d\n",
		    ip_netif->service);
		return ENOENT;
	}
	
	/* Has to be after the device netif module initialization */
	if (ip_netif->arp) {
		if (route) {
			address.value = (uint8_t *) &route->address.s_addr;
			address.length = sizeof(in_addr_t);
			
			rc = arp_device_req(ip_netif->arp->sess,
			    ip_netif->device_id, SERVICE_IP, ip_netif->service,
			    &address);
			if (rc != EOK)
				return rc;
		} else {
			ip_netif->arp = 0;
		}
	}
	
	/* Get packet dimensions */
	rc = nil_packet_size_req(ip_netif->sess, ip_netif->device_id,
	    &ip_netif->packet_dimension);
	if (rc != EOK)
		return rc;
	
	if (ip_netif->packet_dimension.content < IP_MIN_CONTENT) {
		printf("Maximum transmission unit %zu bytes is too small, at "
		    "least %d bytes are needed\n",
		    ip_netif->packet_dimension.content, IP_MIN_CONTENT);
		ip_netif->packet_dimension.content = IP_MIN_CONTENT;
	}
	
	index = ip_netifs_add(&ip_globals.netifs, ip_netif->device_id, ip_netif);
	if (index < 0)
		return index;
	
	if (gateway.s_addr) {
		/* The default gateway */
		ip_globals.gateway.address.s_addr = 0;
		ip_globals.gateway.netmask.s_addr = 0;
		ip_globals.gateway.gateway.s_addr = gateway.s_addr;
		ip_globals.gateway.netif = ip_netif;
		
		char defgateway[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, (uint8_t *) &gateway.s_addr,
		    defgateway, INET_ADDRSTRLEN);
		printf("%s: Default gateway (%s)\n", NAME, defgateway);
	}
	
	return EOK;
}

static int ip_device_req_local(nic_device_id_t device_id, services_t netif)
{
	ip_netif_t *ip_netif;
	ip_route_t *route;
	int index;
	int rc;

	ip_netif = (ip_netif_t *) malloc(sizeof(ip_netif_t));
	if (!ip_netif)
		return ENOMEM;

	rc = ip_routes_initialize(&ip_netif->routes);
	if (rc != EOK) {
		free(ip_netif);
		return rc;
	}
	
	ip_netif->device_id = device_id;
	ip_netif->service = netif;
	ip_netif->state = NIC_STATE_STOPPED;
	
	fibril_rwlock_write_lock(&ip_globals.netifs_lock);

	rc = ip_netif_initialize(ip_netif);
	if (rc != EOK) {
		fibril_rwlock_write_unlock(&ip_globals.netifs_lock);
		ip_routes_destroy(&ip_netif->routes, free);
		free(ip_netif);
		return rc;
	}
	if (ip_netif->arp)
		ip_netif->arp->usage++;

	/* Print the settings */
	printf("%s: Device registered (id: %d, ipv: %d, conf: %s)\n",
	    NAME, ip_netif->device_id, ip_netif->ipv,
	    ip_netif->dhcp ? "dhcp" : "static");
	
	// TODO ipv6 addresses
	
	char address[INET_ADDRSTRLEN];
	char netmask[INET_ADDRSTRLEN];
	char gateway[INET_ADDRSTRLEN];
	
	for (index = 0; index < ip_routes_count(&ip_netif->routes); index++) {
		route = ip_routes_get_index(&ip_netif->routes, index);
		if (route) {
			inet_ntop(AF_INET, (uint8_t *) &route->address.s_addr,
			    address, INET_ADDRSTRLEN);
			inet_ntop(AF_INET, (uint8_t *) &route->netmask.s_addr,
			    netmask, INET_ADDRSTRLEN);
			inet_ntop(AF_INET, (uint8_t *) &route->gateway.s_addr,
			    gateway, INET_ADDRSTRLEN);
			printf("%s: Route %d (address: %s, netmask: %s, "
			    "gateway: %s)\n", NAME, index, address, netmask,
			    gateway);
		}
	}
	
	inet_ntop(AF_INET, (uint8_t *) &ip_netif->broadcast.s_addr, address,
	    INET_ADDRSTRLEN);
	fibril_rwlock_write_unlock(&ip_globals.netifs_lock);

	printf("%s: Broadcast (%s)\n", NAME, address);

	return EOK;
}

/** Searches the network interfaces if there is a suitable route.
 *
 * @param[in] netif	The network interface to be searched for routes. May be
 *			NULL.
 * @param[in] destination The destination address.
 * @return		The found route.
 * @return		NULL if no route was found.
 */
static ip_route_t *ip_netif_find_route(ip_netif_t *netif,
    in_addr_t destination)
{
	int index;
	ip_route_t *route;
	
	if (!netif)
		return NULL;
	
	/* Start with the first one (the direct route) */
	for (index = 0; index < ip_routes_count(&netif->routes); index++) {
		route = ip_routes_get_index(&netif->routes, index);
		if ((route) &&
		    ((route->address.s_addr & route->netmask.s_addr) ==
		    (destination.s_addr & route->netmask.s_addr)))
			return route;
	}

	return NULL;
}

/** Searches all network interfaces if there is a suitable route.
 *
 * @param[in] destination The destination address.
 * @return		The found route.
 * @return		NULL if no route was found.
 */
static ip_route_t *ip_find_route(in_addr_t destination) {
	int index;
	ip_route_t *route;
	ip_netif_t *netif;

	/* Start with the last netif - the newest one */
	index = ip_netifs_count(&ip_globals.netifs) - 1;
	while (index >= 0) {
		netif = ip_netifs_get_index(&ip_globals.netifs, index);
		if (netif && (netif->state == NIC_STATE_ACTIVE)) {
			route = ip_netif_find_route(netif, destination);
			if (route)
				return route;
		}
		index--;
	}

	return &ip_globals.gateway;
}

/** Returns the network interface's IP address.
 *
 * @param[in] netif	The network interface.
 * @return		The IP address.
 * @return		NULL if no IP address was found.
 */
static in_addr_t *ip_netif_address(ip_netif_t *netif)
{
	ip_route_t *route;

	route = ip_routes_get_index(&netif->routes, 0);
	return route ? &route->address : NULL;
}

/** Copies the fragment header.
 *
 * Copies only the header itself and relevant IP options.
 *
 * @param[out] last	The created header.
 * @param[in] first	The original header to be copied.
 */
static void ip_create_last_header(ip_header_t *last, ip_header_t *first)
{
	ip_option_t *option;
	size_t next;
	size_t length;

	/* Copy first itself */
	memcpy(last, first, sizeof(ip_header_t));
	length = sizeof(ip_header_t);
	next = sizeof(ip_header_t);

	/* Process all IP options */
	while (next < GET_IP_HEADER_LENGTH(first)) {
		option = (ip_option_t *) (((uint8_t *) first) + next);
		/* Skip end or noop */
		if ((option->type == IPOPT_END) ||
		    (option->type == IPOPT_NOOP)) {
			next++;
		} else {
			/* Copy if told so or skip */
			if (IPOPT_COPIED(option->type)) {
				memcpy(((uint8_t *) last) + length,
				    ((uint8_t *) first) + next, option->length);
				length += option->length;
			}
			/* Next option */
			next += option->length;
		}
	}

	/* Align 4 byte boundary */
	if (length % 4) {
		bzero(((uint8_t *) last) + length, 4 - (length % 4));
		SET_IP_HEADER_LENGTH(last, (length / 4 + 1));
	} else {
		SET_IP_HEADER_LENGTH(last, (length / 4));
	}

	last->header_checksum = 0;
}

/** Prepares the outgoing packet or the packet queue.
 *
 * The packet queue is a fragmented packet
 * Updates the first packet's IP header.
 * Prefixes the additional packets with fragment headers.
 *
 * @param[in] source	The source address.
 * @param[in] dest	The destination address.
 * @param[in,out] packet The packet to be sent.
 * @param[in] destination The destination hardware address.
 * @return		EOK on success.
 * @return		EINVAL if the packet is too small to contain the IP
 *			header.
 * @return		EINVAL if the packet is too long than the IP allows.
 * @return		ENOMEM if there is not enough memory left.
 * @return		Other error codes as defined for the packet_set_addr()
 *			function.
 */
static int ip_prepare_packet(in_addr_t *source, in_addr_t dest,
    packet_t *packet, measured_string_t *destination)
{
	size_t length;
	ip_header_t *header;
	ip_header_t *last_header;
	ip_header_t *middle_header;
	packet_t *next;
	int rc;

	length = packet_get_data_length(packet);
	if ((length < sizeof(ip_header_t)) || (length > IP_MAX_CONTENT))
		return EINVAL;

	header = (ip_header_t *) packet_get_data(packet);
	if (destination) {
		rc = packet_set_addr(packet, NULL, (uint8_t *) destination->value,
		    destination->length);
	} else {
		rc = packet_set_addr(packet, NULL, NULL, 0);
	}
	if (rc != EOK)
		return rc;
	
	SET_IP_HEADER_VERSION(header, IPV4);
	SET_IP_HEADER_FRAGMENT_OFFSET_HIGH(header, 0);
	header->fragment_offset_low = 0;
	header->header_checksum = 0;
	if (source)
		header->source_address = source->s_addr;
	header->destination_address = dest.s_addr;

	fibril_rwlock_write_lock(&ip_globals.lock);
	ip_globals.packet_counter++;
	header->identification = htons(ip_globals.packet_counter);
	fibril_rwlock_write_unlock(&ip_globals.lock);

	if (pq_next(packet)) {
		last_header = (ip_header_t *) malloc(IP_HEADER_LENGTH(header));
		if (!last_header)
			return ENOMEM;
		ip_create_last_header(last_header, header);
		next = pq_next(packet);
		while (pq_next(next)) {
			middle_header = (ip_header_t *) packet_prefix(next,
			    IP_HEADER_LENGTH(last_header));
			if (!middle_header) {
				free(last_header);
				return ENOMEM;
			}

			memcpy(middle_header, last_header,
			    IP_HEADER_LENGTH(last_header));
			SET_IP_HEADER_FLAGS(header,
			    (GET_IP_HEADER_FLAGS(header) | IPFLAG_MORE_FRAGMENTS));
			middle_header->total_length =
			    htons(packet_get_data_length(next));
			SET_IP_HEADER_FRAGMENT_OFFSET_HIGH(middle_header,
			    IP_COMPUTE_FRAGMENT_OFFSET_HIGH(length));
			middle_header->fragment_offset_low =
			    IP_COMPUTE_FRAGMENT_OFFSET_LOW(length);
			middle_header->header_checksum =
			    IP_HEADER_CHECKSUM(middle_header);
			if (destination) {
				rc = packet_set_addr(next, NULL,
				    (uint8_t *) destination->value,
				    destination->length);
				if (rc != EOK) {
				    	free(last_header);
					return rc;
				}
			}
			length += packet_get_data_length(next);
			next = pq_next(next);
		}

		middle_header = (ip_header_t *) packet_prefix(next,
		    IP_HEADER_LENGTH(last_header));
		if (!middle_header) {
			free(last_header);
			return ENOMEM;
		}

		memcpy(middle_header, last_header,
		    IP_HEADER_LENGTH(last_header));
		middle_header->total_length =
		    htons(packet_get_data_length(next));
		SET_IP_HEADER_FRAGMENT_OFFSET_HIGH(middle_header,
		    IP_COMPUTE_FRAGMENT_OFFSET_HIGH(length));
		middle_header->fragment_offset_low =
		    IP_COMPUTE_FRAGMENT_OFFSET_LOW(length);
		middle_header->header_checksum =
		    IP_HEADER_CHECKSUM(middle_header);
		if (destination) {
			rc = packet_set_addr(next, NULL,
			    (uint8_t *) destination->value,
			    destination->length);
			if (rc != EOK) {
				free(last_header);
				return rc;
			}
		}
		length += packet_get_data_length(next);
		free(last_header);
		SET_IP_HEADER_FLAGS(header,
		    (GET_IP_HEADER_FLAGS(header) | IPFLAG_MORE_FRAGMENTS));
	}

	header->total_length = htons(length);
	/* Unnecessary for all protocols */
	header->header_checksum = IP_HEADER_CHECKSUM(header);

	return EOK;
}

/** Fragments the packet from the end.
 *
 * @param[in] packet	The packet to be fragmented.
 * @param[in,out] new_packet The new packet fragment.
 * @param[in,out] header The original packet header.
 * @param[in,out] new_header The new packet fragment header.
 * @param[in] length	The new fragment length.
 * @param[in] src	The source address.
 * @param[in] dest	The destiantion address.
 * @param[in] addrlen	The address length.
 * @return		EOK on success.
 * @return		ENOMEM if the target packet is too small.
 * @return		Other error codes as defined for the packet_set_addr()
 *			function.
 * @return		Other error codes as defined for the pq_insert_after()
 *			function.
 */
static int ip_fragment_packet_data(packet_t *packet, packet_t *new_packet,
    ip_header_t *header, ip_header_t *new_header, size_t length,
    const struct sockaddr *src, const struct sockaddr *dest, socklen_t addrlen)
{
	void *data;
	size_t offset;
	int rc;

	data = packet_suffix(new_packet, length);
	if (!data)
		return ENOMEM;

	memcpy(data, ((void *) header) + IP_TOTAL_LENGTH(header) - length,
	    length);
	
	rc = packet_trim(packet, 0, length);
	if (rc != EOK)
		return rc;
	
	header->total_length = htons(IP_TOTAL_LENGTH(header) - length);
	new_header->total_length = htons(IP_HEADER_LENGTH(new_header) + length);
	offset = IP_FRAGMENT_OFFSET(header) + IP_HEADER_DATA_LENGTH(header);
	SET_IP_HEADER_FRAGMENT_OFFSET_HIGH(new_header,
	    IP_COMPUTE_FRAGMENT_OFFSET_HIGH(offset));
	new_header->fragment_offset_low =
	    IP_COMPUTE_FRAGMENT_OFFSET_LOW(offset);
	new_header->header_checksum = IP_HEADER_CHECKSUM(new_header);
	
	rc = packet_set_addr(new_packet, (const uint8_t *) src,
	    (const uint8_t *) dest, addrlen);
	if (rc != EOK)
		return rc;

	return pq_insert_after(packet, new_packet);
}

/** Prefixes a middle fragment header based on the last fragment header to the
 * packet.
 *
 * @param[in] packet	The packet to be prefixed.
 * @param[in] last	The last header to be copied.
 * @return		The prefixed middle header.
 * @return		NULL on error.
 */
static ip_header_t *ip_create_middle_header(packet_t *packet,
    ip_header_t *last)
{
	ip_header_t *middle;

	middle = (ip_header_t *) packet_suffix(packet, IP_HEADER_LENGTH(last));
	if (!middle)
		return NULL;
	memcpy(middle, last, IP_HEADER_LENGTH(last));
	SET_IP_HEADER_FLAGS(middle,
	    (GET_IP_HEADER_FLAGS(middle) | IPFLAG_MORE_FRAGMENTS));
	return middle;
}

/** Checks the packet length and fragments it if needed.
 *
 * The new fragments are queued before the original packet.
 *
 * @param[in,out] packet The packet to be checked.
 * @param[in] length	The maximum packet length.
 * @param[in] prefix	The minimum prefix size.
 * @param[in] suffix	The minimum suffix size.
 * @param[in] addr_len	The minimum address length.
 * @return		EOK on success.
 * @return		EINVAL if the packet_get_addr() function fails.
 * @return		EINVAL if the packet does not contain the IP header.
 * @return		EPERM if the packet needs to be fragmented and the
 *			fragmentation is not allowed.
 * @return		ENOMEM if there is not enough memory left.
 * @return		ENOMEM if there is no packet available.
 * @return		ENOMEM if the packet is too small to contain the IP
 *			header.
 * @return		Other error codes as defined for the packet_trim()
 *			function.
 * @return		Other error codes as defined for the
 *			ip_create_middle_header() function.
 * @return		Other error codes as defined for the
 *			ip_fragment_packet_data() function.
 */
static int
ip_fragment_packet(packet_t *packet, size_t length, size_t prefix, size_t suffix,
    socklen_t addr_len)
{
	packet_t *new_packet;
	ip_header_t *header;
	ip_header_t *middle_header;
	ip_header_t *last_header;
	struct sockaddr *src;
	struct sockaddr *dest;
	socklen_t addrlen;
	int result;
	int rc;

	result = packet_get_addr(packet, (uint8_t **) &src, (uint8_t **) &dest);
	if (result <= 0)
		return EINVAL;

	addrlen = (socklen_t) result;
	if (packet_get_data_length(packet) <= sizeof(ip_header_t))
		return ENOMEM;

	/* Get header */
	header = (ip_header_t *) packet_get_data(packet);
	if (!header)
		return EINVAL;

	/* Fragmentation forbidden? */
	if(GET_IP_HEADER_FLAGS(header) & IPFLAG_DONT_FRAGMENT)
		return EPERM;

	/* Create the last fragment */
	new_packet = packet_get_4_remote(ip_globals.net_sess, prefix, length,
	    suffix, ((addrlen > addr_len) ? addrlen : addr_len));
	if (!new_packet)
		return ENOMEM;

	/* Allocate as much as originally */
	last_header = (ip_header_t *) packet_suffix(new_packet,
	    IP_HEADER_LENGTH(header));
	if (!last_header)
		return ip_release_and_return(packet, ENOMEM);

	ip_create_last_header(last_header, header);

	/* Trim the unused space */
	rc = packet_trim(new_packet, 0,
	    IP_HEADER_LENGTH(header) - IP_HEADER_LENGTH(last_header));
	if (rc != EOK)
		return ip_release_and_return(packet, rc);

	/* Greatest multiple of 8 lower than content */
	// TODO even fragmentation?
	length = length & ~0x7;
	
	rc = ip_fragment_packet_data(packet, new_packet, header, last_header,
	    ((IP_HEADER_DATA_LENGTH(header) -
	    ((length - IP_HEADER_LENGTH(header)) & ~0x7)) %
	    ((length - IP_HEADER_LENGTH(last_header)) & ~0x7)),
	    src, dest, addrlen);
	if (rc != EOK)
		return ip_release_and_return(packet, rc);

	/* Mark the first as fragmented */
	SET_IP_HEADER_FLAGS(header,
	    (GET_IP_HEADER_FLAGS(header) | IPFLAG_MORE_FRAGMENTS));

	/* Create middle fragments */
	while (IP_TOTAL_LENGTH(header) > length) {
		new_packet = packet_get_4_remote(ip_globals.net_sess, prefix,
		    length, suffix,
		    ((addrlen >= addr_len) ? addrlen : addr_len));
		if (!new_packet)
			return ENOMEM;

		middle_header = ip_create_middle_header(new_packet,
		    last_header);
		if (!middle_header)
			return ip_release_and_return(packet, ENOMEM);

		rc = ip_fragment_packet_data(packet, new_packet, header,
		    middle_header,
		    (length - IP_HEADER_LENGTH(middle_header)) & ~0x7,
		    src, dest, addrlen);
		if (rc != EOK)
			return ip_release_and_return(packet, rc);
	}

	/* Finish the first fragment */
	header->header_checksum = IP_HEADER_CHECKSUM(header);

	return EOK;
}

/** Check the packet queue lengths and fragments the packets if needed.
 *
 * The ICMP_FRAG_NEEDED error notification may be sent if the packet needs to
 * be fragmented and the fragmentation is not allowed.
 *
 * @param[in,out] packet   Packet or the packet queue to be checked.
 * @param[in]     prefix   Minimum prefix size.
 * @param[in]     content  Maximum content size.
 * @param[in]     suffix   Minimum suffix size.
 * @param[in]     addr_len Minimum address length.
 * @param[in]     error    Error module service.
 *
 * @return The packet or the packet queue of the allowed length.
 * @return NULL if there are no packets left.
 *
 */
static packet_t *ip_split_packet(packet_t *packet, size_t prefix, size_t content,
    size_t suffix, socklen_t addr_len, services_t error)
{
	size_t length;
	packet_t *next;
	packet_t *new_packet;
	int result;
	async_sess_t *sess;
	
	next = packet;
	/* Check all packets */
	while (next) {
		length = packet_get_data_length(next);
		
		if (length <= content) {
			next = pq_next(next);
			continue;
		}

		/* Too long */
		result = ip_fragment_packet(next, content, prefix,
		    suffix, addr_len);
		if (result != EOK) {
			new_packet = pq_detach(next);
			if (next == packet) {
				/* The new first packet of the queue */
				packet = new_packet;
			}
			/* Fragmentation needed? */
			if (result == EPERM) {
				sess = ip_prepare_icmp_and_get_session(error, next, NULL);
				if (sess) {
					/* Fragmentation necessary ICMP */
					icmp_destination_unreachable_msg(sess,
					    ICMP_FRAG_NEEDED, content, next);
				}
			} else {
				pq_release_remote(ip_globals.net_sess,
				    packet_get_id(next));
			}

			next = new_packet;
			continue;
		}

		next = pq_next(next);
	}

	return packet;
}

/** Send the packet or the packet queue via the specified route.
 *
 * The ICMP_HOST_UNREACH error notification may be sent if route hardware
 * destination address is found.
 *
 * @param[in,out] packet Packet to be sent.
 * @param[in]     netif  Target network interface.
 * @param[in]     route  Target route.
 * @param[in]     src    Source address.
 * @param[in]     dest   Destination address.
 * @param[in]     error  Error module service.
 *
 * @return EOK on success.
 * @return Other error codes as defined for arp_translate_req().
 * @return Other error codes as defined for ip_prepare_packet().
 *
 */
static int ip_send_route(packet_t *packet, ip_netif_t *netif,
    ip_route_t *route, in_addr_t *src, in_addr_t dest, services_t error)
{
	measured_string_t destination;
	measured_string_t *translation;
	uint8_t *data;
	async_sess_t *sess;
	int rc;

	/* Get destination hardware address */
	if (netif->arp && (route->address.s_addr != dest.s_addr)) {
		destination.value = route->gateway.s_addr ?
		    (uint8_t *) &route->gateway.s_addr : (uint8_t *) &dest.s_addr;
		destination.length = sizeof(dest.s_addr);

		rc = arp_translate_req(netif->arp->sess, netif->device_id,
		    SERVICE_IP, &destination, &translation, &data);
		if (rc != EOK) {
			pq_release_remote(ip_globals.net_sess,
			    packet_get_id(packet));
			return rc;
		}

		if (!translation || !translation->value) {
			if (translation) {
				free(translation);
				free(data);
			}
			sess = ip_prepare_icmp_and_get_session(error, packet,
			    NULL);
			if (sess) {
				/* Unreachable ICMP if no routing */
				icmp_destination_unreachable_msg(sess,
				    ICMP_HOST_UNREACH, 0, packet);
			}
			return EINVAL;
		}

	} else {
		translation = NULL;
	}

	rc = ip_prepare_packet(src, dest, packet, translation);
	if (rc != EOK) {
		pq_release_remote(ip_globals.net_sess, packet_get_id(packet));
	} else {
		packet = ip_split_packet(packet, netif->packet_dimension.prefix,
		    netif->packet_dimension.content,
		    netif->packet_dimension.suffix,
		    netif->packet_dimension.addr_len, error);
		if (packet) {
			nil_send_msg(netif->sess, netif->device_id, packet,
			    SERVICE_IP);
		}
	}

	if (translation) {
		free(translation);
		free(data);
	}

	return rc;
}

static int ip_send_msg_local(nic_device_id_t device_id, packet_t *packet,
    services_t sender, services_t error)
{
	int addrlen;
	ip_netif_t *netif;
	ip_route_t *route;
	struct sockaddr *addr;
	struct sockaddr_in *address_in;
	in_addr_t *dest;
	in_addr_t *src;
	async_sess_t *sess;
	int rc;

	/*
	 * Addresses in the host byte order
	 * Should be the next hop address or the target destination address
	 */
	addrlen = packet_get_addr(packet, NULL, (uint8_t **) &addr);
	if (addrlen < 0)
		return ip_release_and_return(packet, addrlen);
	if ((size_t) addrlen < sizeof(struct sockaddr))
		return ip_release_and_return(packet, EINVAL);

	switch (addr->sa_family) {
	case AF_INET:
		if (addrlen != sizeof(struct sockaddr_in))
			return ip_release_and_return(packet, EINVAL);
		address_in = (struct sockaddr_in *) addr;
		dest = &address_in->sin_addr;
		if (!dest->s_addr)
			dest->s_addr = IPV4_LOCALHOST_ADDRESS;
		break;
	case AF_INET6:
	default:
		return ip_release_and_return(packet, EAFNOSUPPORT);
	}

	netif = NULL;
	route = NULL;
	fibril_rwlock_read_lock(&ip_globals.netifs_lock);

	/* Device specified? */
	if (device_id > 0) {
		netif = ip_netifs_find(&ip_globals.netifs, device_id);
		route = ip_netif_find_route(netif, *dest);
		if (netif && !route && (ip_globals.gateway.netif == netif))
			route = &ip_globals.gateway;
	}

	if (!route) {
		route = ip_find_route(*dest);
		netif = route ? route->netif : NULL;
	}
	if (!netif || !route) {
		fibril_rwlock_read_unlock(&ip_globals.netifs_lock);
		sess = ip_prepare_icmp_and_get_session(error, packet, NULL);
		if (sess) {
			/* Unreachable ICMP if no routing */
			icmp_destination_unreachable_msg(sess,
			    ICMP_NET_UNREACH, 0, packet);
		}
		return ENOENT;
	}

	if (error) {
		/*
		 * Do not send for broadcast, anycast packets or network
		 * broadcast.
		 */
		if (!dest->s_addr || !(~dest->s_addr) ||
		    !(~((dest->s_addr & ~route->netmask.s_addr) |
		    route->netmask.s_addr)) ||
		    (!(dest->s_addr & ~route->netmask.s_addr))) {
			return ip_release_and_return(packet, EINVAL);
		}
	}
	
	/* If the local host is the destination */
	if ((route->address.s_addr == dest->s_addr) &&
	    (dest->s_addr != IPV4_LOCALHOST_ADDRESS)) {
		/* Find the loopback device to deliver */
		dest->s_addr = IPV4_LOCALHOST_ADDRESS;
		route = ip_find_route(*dest);
		netif = route ? route->netif : NULL;
		if (!netif || !route) {
			fibril_rwlock_read_unlock(&ip_globals.netifs_lock);
			sess = ip_prepare_icmp_and_get_session(error, packet,
			    NULL);
			if (sess) {
				/* Unreachable ICMP if no routing */
				icmp_destination_unreachable_msg(sess,
				    ICMP_HOST_UNREACH, 0, packet);
			}
			return ENOENT;
		}
	}
	
	src = ip_netif_address(netif);
	if (!src) {
		fibril_rwlock_read_unlock(&ip_globals.netifs_lock);
		return ip_release_and_return(packet, ENOENT);
	}

	rc = ip_send_route(packet, netif, route, src, *dest, error);
	fibril_rwlock_read_unlock(&ip_globals.netifs_lock);

	return rc;
}

/** Updates the device state.
 *
 * @param[in] device_id	The device identifier.
 * @param[in] state	The new state value.
 * @return		EOK on success.
 * @return		ENOENT if device is not found.
 */
static int ip_device_state_message(nic_device_id_t device_id,
    nic_device_state_t state)
{
	ip_netif_t *netif;

	fibril_rwlock_write_lock(&ip_globals.netifs_lock);
	/* Find the device */
	netif = ip_netifs_find(&ip_globals.netifs, device_id);
	if (!netif) {
		fibril_rwlock_write_unlock(&ip_globals.netifs_lock);
		return ENOENT;
	}
	netif->state = state;
	fibril_rwlock_write_unlock(&ip_globals.netifs_lock);

	printf("%s: Device %d changed state to '%s'\n", NAME, device_id,
	    nic_device_state_to_string(state));

	return EOK;
}

/** Returns the packet destination address from the IP header.
 *
 * @param[in] header	The packet IP header to be read.
 * @return		The packet destination address.
 */
static in_addr_t ip_get_destination(ip_header_t *header)
{
	in_addr_t destination;

	// TODO search set ipopt route?
	destination.s_addr = header->destination_address;
	return destination;
}

/** Delivers the packet to the local host.
 *
 * The packet is either passed to another module or released on error.
 * The ICMP_PROT_UNREACH error notification may be sent if the protocol is not
 * found.
 *
 * @param[in] device_id	The source device identifier.
 * @param[in] packet	The packet to be delivered.
 * @param[in] header	The first packet IP header. May be NULL.
 * @param[in] error	The packet error service.
 * @return		EOK on success.
 * @return		ENOTSUP if the packet is a fragment.
 * @return		EAFNOSUPPORT if the address family is not supported.
 * @return		ENOENT if the target protocol is not found.
 * @return		Other error codes as defined for the packet_set_addr()
 *			function.
 * @return		Other error codes as defined for the packet_trim()
 *			function.
 * @return		Other error codes as defined for the protocol specific
 *			tl_received_msg() function.
 */
static int ip_deliver_local(nic_device_id_t device_id, packet_t *packet,
    ip_header_t *header, services_t error)
{
	ip_proto_t *proto;
	async_sess_t *sess;
	services_t service;
	tl_received_msg_t received_msg;
	struct sockaddr *src;
	struct sockaddr *dest;
	struct sockaddr_in src_in;
	struct sockaddr_in dest_in;
	socklen_t addrlen;
	int rc;

	if ((GET_IP_HEADER_FLAGS(header) & IPFLAG_MORE_FRAGMENTS) ||
	    IP_FRAGMENT_OFFSET(header)) {
		// TODO fragmented
		return ENOTSUP;
	}
	
	switch (GET_IP_HEADER_VERSION(header)) {
	case IPVERSION:
		addrlen = sizeof(src_in);
		bzero(&src_in, addrlen);
		src_in.sin_family = AF_INET;
		memcpy(&dest_in, &src_in, addrlen);
		memcpy(&src_in.sin_addr.s_addr, &header->source_address,
		    sizeof(header->source_address));
		memcpy(&dest_in.sin_addr.s_addr, &header->destination_address,
		    sizeof(header->destination_address));
		src = (struct sockaddr *) &src_in;
		dest = (struct sockaddr *) &dest_in;
		break;

	default:
		return ip_release_and_return(packet, EAFNOSUPPORT);
	}

	rc = packet_set_addr(packet, (uint8_t *) src, (uint8_t *) dest,
	    addrlen);
	if (rc != EOK)
		return ip_release_and_return(packet, rc);

	/* Trim padding if present */
	if (!error &&
	    (IP_TOTAL_LENGTH(header) < packet_get_data_length(packet))) {
		rc = packet_trim(packet, 0,
		    packet_get_data_length(packet) - IP_TOTAL_LENGTH(header));
		if (rc != EOK)
			return ip_release_and_return(packet, rc);
	}

	fibril_rwlock_read_lock(&ip_globals.protos_lock);

	proto = ip_protos_find(&ip_globals.protos, header->protocol);
	if (!proto) {
		fibril_rwlock_read_unlock(&ip_globals.protos_lock);
		sess = ip_prepare_icmp_and_get_session(error, packet, header);
		if (sess) {
			/* Unreachable ICMP */
			icmp_destination_unreachable_msg(sess,
			    ICMP_PROT_UNREACH, 0, packet);
		}
		return ENOENT;
	}

	if (proto->received_msg) {
		service = proto->service;
		received_msg = proto->received_msg;
		fibril_rwlock_read_unlock(&ip_globals.protos_lock);
		rc = received_msg(device_id, packet, service, error);
	} else {
		rc = tl_received_msg(proto->sess, device_id, packet,
		    proto->service, error);
		fibril_rwlock_read_unlock(&ip_globals.protos_lock);
	}

	return rc;
}

/** Processes the received packet.
 *
 * The packet is either passed to another module or released on error.
 *
 * The ICMP_PARAM_POINTER error notification may be sent if the checksum is
 * invalid.
 * The ICMP_EXC_TTL error notification may be sent if the TTL is less than two.
 * The ICMP_HOST_UNREACH error notification may be sent if no route was found.
 * The ICMP_HOST_UNREACH error notification may be sent if the packet is for
 * another host and the routing is disabled.
 *
 * @param[in] device_id	The source device identifier.
 * @param[in] packet	The received packet to be processed.
 * @return		EOK on success.
 * @return		EINVAL if the TTL is less than two.
 * @return		EINVAL if the checksum is invalid.
 * @return		EAFNOSUPPORT if the address family is not supported.
 * @return		ENOENT if no route was found.
 * @return		ENOENT if the packet is for another host and the routing
 *			is disabled.
 */
static int ip_process_packet(nic_device_id_t device_id, packet_t *packet)
{
	ip_header_t *header;
	in_addr_t dest;
	ip_route_t *route;
	async_sess_t *sess;
	struct sockaddr *addr;
	struct sockaddr_in addr_in;
	socklen_t addrlen;
	int rc;
	
	header = (ip_header_t *) packet_get_data(packet);
	if (!header)
		return ip_release_and_return(packet, ENOMEM);

	/* Checksum */
	if ((header->header_checksum) &&
	    (IP_HEADER_CHECKSUM(header) != IP_CHECKSUM_ZERO)) {
		sess = ip_prepare_icmp_and_get_session(0, packet, header);
		if (sess) {
			/* Checksum error ICMP */
			icmp_parameter_problem_msg(sess, ICMP_PARAM_POINTER,
			    ((size_t) ((void *) &header->header_checksum)) -
			    ((size_t) ((void *) header)), packet);
		}
		return EINVAL;
	}

	if (header->ttl <= 1) {
		sess = ip_prepare_icmp_and_get_session(0, packet, header);
		if (sess) {
			/* TTL exceeded ICMP */
			icmp_time_exceeded_msg(sess, ICMP_EXC_TTL, packet);
		}
		return EINVAL;
	}
	
	/* Process ipopt and get destination */
	dest = ip_get_destination(header);

	/* Set the destination address */
	switch (GET_IP_HEADER_VERSION(header)) {
	case IPVERSION:
		addrlen = sizeof(addr_in);
		bzero(&addr_in, addrlen);
		addr_in.sin_family = AF_INET;
		memcpy(&addr_in.sin_addr.s_addr, &dest, sizeof(dest));
		addr = (struct sockaddr *) &addr_in;
		break;

	default:
		return ip_release_and_return(packet, EAFNOSUPPORT);
	}

	rc = packet_set_addr(packet, NULL, (uint8_t *) &addr, addrlen);
	if (rc != EOK)
		return rc;
	
	route = ip_find_route(dest);
	if (!route) {
		sess = ip_prepare_icmp_and_get_session(0, packet, header);
		if (sess) {
			/* Unreachable ICMP */
			icmp_destination_unreachable_msg(sess,
			    ICMP_HOST_UNREACH, 0, packet);
		}
		return ENOENT;
	}

	if (route->address.s_addr == dest.s_addr) {
		/* Local delivery */
		return ip_deliver_local(device_id, packet, header, 0);
	}

	if (route->netif->routing) {
		header->ttl--;
		return ip_send_route(packet, route->netif, route, NULL, dest,
		    0);
	}

	sess = ip_prepare_icmp_and_get_session(0, packet, header);
	if (sess) {
		/* Unreachable ICMP if no routing */
		icmp_destination_unreachable_msg(sess, ICMP_HOST_UNREACH, 0,
		    packet);
	}
	
	return ENOENT;
}

/** Return the device packet dimensions for sending.
 *
 * @param[in]  device_id Device identifier.
 * @param[out] addr_len  Minimum reserved address length.
 * @param[out] prefix    Minimum reserved prefix size.
 * @param[out] content   Maximum content size.
 * @param[out] suffix    Minimum reserved suffix size.
 *
 * @return EOK on success.
 *
 */
static int ip_packet_size_message(nic_device_id_t device_id, size_t *addr_len,
    size_t *prefix, size_t *content, size_t *suffix)
{
	ip_netif_t *netif;
	int index;

	if (!addr_len || !prefix || !content || !suffix)
		return EBADMEM;

	*content = IP_MAX_CONTENT - IP_PREFIX;
	fibril_rwlock_read_lock(&ip_globals.netifs_lock);
	if (device_id < 0) {
		*addr_len = IP_ADDR;
		*prefix = 0;
		*suffix = 0;

		for (index = ip_netifs_count(&ip_globals.netifs) - 1;
		    index >= 0; index--) {
			netif = ip_netifs_get_index(&ip_globals.netifs, index);
			if (!netif)
				continue;
			
			if (netif->packet_dimension.addr_len > *addr_len)
				*addr_len = netif->packet_dimension.addr_len;
			
			if (netif->packet_dimension.prefix > *prefix)
				*prefix = netif->packet_dimension.prefix;
				
			if (netif->packet_dimension.suffix > *suffix)
				*suffix = netif->packet_dimension.suffix;
		}

		*prefix = *prefix + IP_PREFIX;
		*suffix = *suffix + IP_SUFFIX;
	} else {
		netif = ip_netifs_find(&ip_globals.netifs, device_id);
		if (!netif) {
			fibril_rwlock_read_unlock(&ip_globals.netifs_lock);
			return ENOENT;
		}

		*addr_len = (netif->packet_dimension.addr_len > IP_ADDR) ?
		    netif->packet_dimension.addr_len : IP_ADDR;
		*prefix = netif->packet_dimension.prefix + IP_PREFIX;
		*suffix = netif->packet_dimension.suffix + IP_SUFFIX;
	}
	fibril_rwlock_read_unlock(&ip_globals.netifs_lock);

	return EOK;
}

/** Updates the device content length according to the new MTU value.
 *
 * @param[in] device_id	The device identifier.
 * @param[in] mtu	The new mtu value.
 * @return		EOK on success.
 * @return		ENOENT if device is not found.
 */
static int ip_mtu_changed_message(nic_device_id_t device_id, size_t mtu)
{
	ip_netif_t *netif;

	fibril_rwlock_write_lock(&ip_globals.netifs_lock);
	netif = ip_netifs_find(&ip_globals.netifs, device_id);
	if (!netif) {
		fibril_rwlock_write_unlock(&ip_globals.netifs_lock);
		return ENOENT;
	}
	netif->packet_dimension.content = mtu;
	fibril_rwlock_write_unlock(&ip_globals.netifs_lock);

	printf("%s: Device %d changed MTU to %zu\n", NAME, device_id, mtu);

	return EOK;
}

/** Process IPC messages from the registered device driver modules
 *
 * @param[in]     iid   Message identifier.
 * @param[in,out] icall Message parameters.
 * @param[in]     arg   Local argument.
 *
 */
static void ip_receiver(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	packet_t *packet;
	int rc;
	
	while (true) {
		switch (IPC_GET_IMETHOD(*icall)) {
		case NET_IL_DEVICE_STATE:
			rc = ip_device_state_message(IPC_GET_DEVICE(*icall),
			    IPC_GET_STATE(*icall));
			async_answer_0(iid, (sysarg_t) rc);
			break;
		
		case NET_IL_RECEIVED:
			rc = packet_translate_remote(ip_globals.net_sess, &packet,
			    IPC_GET_PACKET(*icall));
			if (rc == EOK) {
				do {
					packet_t *next = pq_detach(packet);
					ip_process_packet(IPC_GET_DEVICE(*icall), packet);
					packet = next;
				} while (packet);
			}
			
			async_answer_0(iid, (sysarg_t) rc);
			break;
		
		case NET_IL_MTU_CHANGED:
			rc = ip_mtu_changed_message(IPC_GET_DEVICE(*icall),
			    IPC_GET_MTU(*icall));
			async_answer_0(iid, (sysarg_t) rc);
			break;
		case NET_IL_ADDR_CHANGED:
			async_answer_0(iid, (sysarg_t) EOK);
			break;

		default:
			async_answer_0(iid, (sysarg_t) ENOTSUP);
		}
		
		iid = async_get_call(icall);
	}
}

/** Register the transport layer protocol.
 *
 * The traffic of this protocol will be supplied using either the receive
 * function or IPC message.
 *
 * @param[in] protocol     Transport layer module protocol.
 * @param[in] service      Transport layer module service.
 * @param[in] sess         Transport layer module session.
 * @param[in] received_msg Receiving function.
 *
 * @return EOK on success.
 * @return EINVAL if the protocol parameter and/or the service
 *         parameter is zero.
 * @return EINVAL if the phone parameter is not a positive number
 *         and the tl_receive_msg is NULL.
 * @return ENOMEM if there is not enough memory left.
 *
 */
static int ip_register(int protocol, services_t service, async_sess_t *sess,
    tl_received_msg_t received_msg)
{
	ip_proto_t *proto;
	int index;

	if ((!protocol) || (!service) || ((!sess) && (!received_msg)))
		return EINVAL;
	
	proto = (ip_proto_t *) malloc(sizeof(ip_protos_t));
	if (!proto)
		return ENOMEM;

	proto->protocol = protocol;
	proto->service = service;
	proto->sess = sess;
	proto->received_msg = received_msg;

	fibril_rwlock_write_lock(&ip_globals.protos_lock);
	index = ip_protos_add(&ip_globals.protos, proto->protocol, proto);
	if (index < 0) {
		fibril_rwlock_write_unlock(&ip_globals.protos_lock);
		free(proto);
		return index;
	}
	fibril_rwlock_write_unlock(&ip_globals.protos_lock);

	printf("%s: Protocol registered (protocol: %d)\n",
	    NAME, proto->protocol);

	return EOK;
}

static int ip_add_route_req_local(nic_device_id_t device_id, in_addr_t address,
    in_addr_t netmask, in_addr_t gateway)
{
	ip_route_t *route;
	ip_netif_t *netif;
	int index;

	fibril_rwlock_write_lock(&ip_globals.netifs_lock);

	netif = ip_netifs_find(&ip_globals.netifs, device_id);
	if (!netif) {
		fibril_rwlock_write_unlock(&ip_globals.netifs_lock);
		return ENOENT;
	}

	route = (ip_route_t *) malloc(sizeof(ip_route_t));
	if (!route) {
		fibril_rwlock_write_unlock(&ip_globals.netifs_lock);
		return ENOMEM;
	}

	route->address.s_addr = address.s_addr;
	route->netmask.s_addr = netmask.s_addr;
	route->gateway.s_addr = gateway.s_addr;
	route->netif = netif;
	index = ip_routes_add(&netif->routes, route);
	if (index < 0)
		free(route);

	fibril_rwlock_write_unlock(&ip_globals.netifs_lock);
	
	return index;
}

static int ip_set_gateway_req_local(nic_device_id_t device_id,
    in_addr_t gateway)
{
	ip_netif_t *netif;

	fibril_rwlock_write_lock(&ip_globals.netifs_lock);

	netif = ip_netifs_find(&ip_globals.netifs, device_id);
	if (!netif) {
		fibril_rwlock_write_unlock(&ip_globals.netifs_lock);
		return ENOENT;
	}

	ip_globals.gateway.address.s_addr = 0;
	ip_globals.gateway.netmask.s_addr = 0;
	ip_globals.gateway.gateway.s_addr = gateway.s_addr;
	ip_globals.gateway.netif = netif;
	
	fibril_rwlock_write_unlock(&ip_globals.netifs_lock);
	
	return EOK;
}

/** Notify the IP module about the received error notification packet.
 *
 * @param[in] device_id Device identifier.
 * @param[in] packet    Received packet or the received packet queue.
 * @param[in] target    Target internetwork module service to be
 *                      delivered to.
 * @param[in] error     Packet error reporting service. Prefixes the
 *                      received packet.
 *
 * @return EOK on success.
 *
 */
static int ip_received_error_msg_local(nic_device_id_t device_id,
    packet_t *packet, services_t target, services_t error)
{
	uint8_t *data;
	int offset;
	icmp_type_t type;
	icmp_code_t code;
	ip_netif_t *netif;
	measured_string_t address;
	ip_route_t *route;
	ip_header_t *header;

	switch (error) {
	case SERVICE_ICMP:
		offset = icmp_client_process_packet(packet, &type, &code, NULL,
		    NULL);
		if (offset < 0)
			return ip_release_and_return(packet, ENOMEM);

		data = packet_get_data(packet);
		header = (ip_header_t *)(data + offset);

		/* Destination host unreachable? */
		if ((type != ICMP_DEST_UNREACH) ||
		    (code != ICMP_HOST_UNREACH)) {
			/* No, something else */
			break;
		}

		fibril_rwlock_read_lock(&ip_globals.netifs_lock);

		netif = ip_netifs_find(&ip_globals.netifs, device_id);
		if (!netif || !netif->arp) {
			fibril_rwlock_read_unlock(&ip_globals.netifs_lock);
			break;
		}

		route = ip_routes_get_index(&netif->routes, 0);

		/* From the same network? */
		if (route && ((route->address.s_addr & route->netmask.s_addr) ==
		    (header->destination_address & route->netmask.s_addr))) {
			/* Clear the ARP mapping if any */
			address.value = (uint8_t *) &header->destination_address;
			address.length = sizeof(header->destination_address);
			arp_clear_address_req(netif->arp->sess,
			    netif->device_id, SERVICE_IP, &address);
		}

		fibril_rwlock_read_unlock(&ip_globals.netifs_lock);
		break;

	default:
		return ip_release_and_return(packet, ENOTSUP);
	}

	return ip_deliver_local(device_id, packet, header, error);
}

static int ip_get_route_req_local(ip_protocol_t protocol,
    const struct sockaddr *destination, socklen_t addrlen,
    nic_device_id_t *device_id, void **header, size_t *headerlen)
{
	struct sockaddr_in *address_in;
	in_addr_t *dest;
	in_addr_t *src;
	ip_route_t *route;
	ipv4_pseudo_header_t *header_in;

	if (!destination || (addrlen <= 0))
		return EINVAL;

	if (!device_id || !header || !headerlen)
		return EBADMEM;

	if ((size_t) addrlen < sizeof(struct sockaddr))
		return EINVAL;

	switch (destination->sa_family) {
	case AF_INET:
		if (addrlen != sizeof(struct sockaddr_in))
			return EINVAL;
		address_in = (struct sockaddr_in *) destination;
		dest = &address_in->sin_addr;
		if (!dest->s_addr)
			dest->s_addr = IPV4_LOCALHOST_ADDRESS;
		break;

	case AF_INET6:
	default:
		return EAFNOSUPPORT;
	}

	fibril_rwlock_read_lock(&ip_globals.lock);
	route = ip_find_route(*dest);
	/* If the local host is the destination */
	if (route && (route->address.s_addr == dest->s_addr) &&
	    (dest->s_addr != IPV4_LOCALHOST_ADDRESS)) {
		/* Find the loopback device to deliver */
		dest->s_addr = IPV4_LOCALHOST_ADDRESS;
		route = ip_find_route(*dest);
	}

	if (!route || !route->netif) {
		fibril_rwlock_read_unlock(&ip_globals.lock);
		return ENOENT;
	}

	*device_id = route->netif->device_id;
	src = ip_netif_address(route->netif);
	fibril_rwlock_read_unlock(&ip_globals.lock);

	*headerlen = sizeof(*header_in);
	header_in = (ipv4_pseudo_header_t *) malloc(*headerlen);
	if (!header_in)
		return ENOMEM;

	bzero(header_in, *headerlen);
	header_in->destination_address = dest->s_addr;
	header_in->source_address = src->s_addr;
	header_in->protocol = protocol;
	header_in->data_length = 0;
	*header = header_in;

	return EOK;
}

/** Processes the IP message.
 *
 * @param[in] callid	The message identifier.
 * @param[in] call	The message parameters.
 * @param[out] answer	The message answer parameters.
 * @param[out] answer_count The last parameter for the actual answer in the
 *			answer parameter.
 * @return		EOK on success.
 * @return		ENOTSUP if the message is not known.
 *
 * @see ip_interface.h
 * @see il_remote.h
 * @see IS_NET_IP_MESSAGE()
 */
int il_module_message(ipc_callid_t callid, ipc_call_t *call, ipc_call_t *answer,
    size_t *answer_count)
{
	packet_t *packet;
	struct sockaddr *addr;
	void *header;
	size_t headerlen;
	size_t addrlen;
	size_t prefix;
	size_t suffix;
	size_t content;
	nic_device_id_t device_id;
	int rc;
	
	*answer_count = 0;
	
	if (!IPC_GET_IMETHOD(*call))
		return EOK;
	
	async_sess_t *callback =
	    async_callback_receive_start(EXCHANGE_SERIALIZE, call);
	if (callback)
		return ip_register(IL_GET_PROTO(*call), IL_GET_SERVICE(*call),
		    callback, NULL);
	
	switch (IPC_GET_IMETHOD(*call)) {
	case NET_IP_DEVICE:
		return ip_device_req_local(IPC_GET_DEVICE(*call),
		    IPC_GET_SERVICE(*call));
	
	case NET_IP_RECEIVED_ERROR:
		rc = packet_translate_remote(ip_globals.net_sess, &packet,
		    IPC_GET_PACKET(*call));
		if (rc != EOK)
			return rc;
		return ip_received_error_msg_local(IPC_GET_DEVICE(*call),
		    packet, IPC_GET_TARGET(*call), IPC_GET_ERROR(*call));
	
	case NET_IP_ADD_ROUTE:
		return ip_add_route_req_local(IPC_GET_DEVICE(*call),
		    IP_GET_ADDRESS(*call), IP_GET_NETMASK(*call),
		    IP_GET_GATEWAY(*call));

	case NET_IP_SET_GATEWAY:
		return ip_set_gateway_req_local(IPC_GET_DEVICE(*call),
		    IP_GET_GATEWAY(*call));

	case NET_IP_GET_ROUTE:
		rc = async_data_write_accept((void **) &addr, false, 0, 0, 0,
		    &addrlen);
		if (rc != EOK)
			return rc;
		
		rc = ip_get_route_req_local(IP_GET_PROTOCOL(*call), addr,
		    (socklen_t) addrlen, &device_id, &header, &headerlen);
		if (rc != EOK)
			return rc;
		
		IPC_SET_DEVICE(*answer, device_id);
		IP_SET_HEADERLEN(*answer, headerlen);
		
		*answer_count = 2;
		
		rc = data_reply(&headerlen, sizeof(headerlen));
		if (rc == EOK)
			rc = data_reply(header, headerlen);
			
		free(header);
		return rc;
	
	case NET_IP_PACKET_SPACE:
		rc = ip_packet_size_message(IPC_GET_DEVICE(*call), &addrlen,
		    &prefix, &content, &suffix);
		if (rc != EOK)
			return rc;
		
		IPC_SET_ADDR(*answer, addrlen);
		IPC_SET_PREFIX(*answer, prefix);
		IPC_SET_CONTENT(*answer, content);
		IPC_SET_SUFFIX(*answer, suffix);
		*answer_count = 4;
		return EOK;
	
	case NET_IP_SEND:
		rc = packet_translate_remote(ip_globals.net_sess, &packet,
		    IPC_GET_PACKET(*call));
		if (rc != EOK)
			return rc;
		
		return ip_send_msg_local(IPC_GET_DEVICE(*call), packet, 0,
		    IPC_GET_ERROR(*call));
	}
	
	return ENOTSUP;
}

int main(int argc, char *argv[])
{
	/* Start the module */
	return il_module_start(SERVICE_IP);
}

/** @}
 */
