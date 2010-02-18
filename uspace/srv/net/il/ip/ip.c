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
 *  IP module implementation.
 *  @see arp.h
 */

#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <string.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include <sys/types.h>

#include "../../err.h"
#include "../../messages.h"
#include "../../modules.h"

#include "../../include/arp_interface.h"
#include "../../include/byteorder.h"
#include "../../include/checksum.h"
#include "../../include/device.h"
#include "../../include/icmp_client.h"
#include "../../include/icmp_codes.h"
#include "../../include/icmp_interface.h"
#include "../../include/il_interface.h"
#include "../../include/in.h"
#include "../../include/in6.h"
#include "../../include/inet.h"
#include "../../include/ip_client.h"
#include "../../include/ip_interface.h"
#include "../../include/net_interface.h"
#include "../../include/nil_interface.h"
#include "../../include/tl_interface.h"
#include "../../include/socket_codes.h"
#include "../../include/socket_errno.h"
#include "../../structures/measured_strings.h"
#include "../../structures/module_map.h"
#include "../../structures/packet/packet_client.h"

#include "../../nil/nil_messages.h"

#include "../il_messages.h"

#include "ip.h"
#include "ip_header.h"
#include "ip_messages.h"
#include "ip_module.h"

/** IP version 4.
 */
#define IPV4				4

/** Default network interface IP version.
 */
#define NET_DEFAULT_IPV		IPV4

/** Default network interface IP routing.
 */
#define NET_DEFAULT_IP_ROUTING	false

/** Minimum IP packet content.
 */
#define IP_MIN_CONTENT	576

/** ARP module name.
 */
#define ARP_NAME				"arp"

/** ARP module filename.
 */
#define ARP_FILENAME			"/srv/arp"

/** IP packet address length.
 */
#define IP_ADDR							sizeof( struct sockaddr_in6 )

/** IP packet prefix length.
 */
#define IP_PREFIX						sizeof( ip_header_t )

/** IP packet suffix length.
 */
#define IP_SUFFIX						0

/** IP packet maximum content length.
 */
#define IP_MAX_CONTENT					65535

/** The IP localhost address.
 */
#define IPV4_LOCALHOST_ADDRESS	htonl(( 127 << 24 ) + 1 )

/** IP global data.
 */
ip_globals_t	ip_globals;

DEVICE_MAP_IMPLEMENT( ip_netifs, ip_netif_t )

INT_MAP_IMPLEMENT( ip_protos, ip_proto_t )

GENERIC_FIELD_IMPLEMENT( ip_routes, ip_route_t )

/** Updates the device content length according to the new MTU value.
 *  @param[in] device_id The device identifier.
 *  @param[in] mtu The new mtu value.
 *  @returns EOK on success.
 *  @returns ENOENT if device is not found.
 */
int	ip_mtu_changed_message( device_id_t device_id, size_t mtu );

/** Updates the device state.
 *  @param[in] device_id The device identifier.
 *  @param[in] state The new state value.
 *  @returns EOK on success.
 *  @returns ENOENT if device is not found.
 */
int	ip_device_state_message( device_id_t device_id, device_state_t state );

/** Returns the device packet dimensions for sending.
 *  @param[in] phone The service module phone.
 *  @param[in] message The service specific message.
 *  @param[in] device_id The device identifier.
 *  @param[out] addr_len The minimum reserved address length.
 *  @param[out] prefix The minimum reserved prefix size.
 *  @param[out] content The maximum content size.
 *  @param[out] suffix The minimum reserved suffix size.
 *  @returns EOK on success.
 */
int	ip_packet_size_message( device_id_t device_id, size_t * addr_len, size_t * prefix, size_t * content, size_t * suffix );

/** Registers the transport layer protocol.
 *  The traffic of this protocol will be supplied using either the receive function or IPC message.
 *  @param[in] protocol The transport layer module protocol.
 *  @param[in] service The transport layer module service.
 *  @param[in] phone The transport layer module phone.
 *  @param[in] tl_received_msg The receiving function.
 *  @returns EOK on success.
 *  @returns EINVAL if the protocol parameter and/or the service parameter is zero (0).
 *  @returns EINVAL if the phone parameter is not a positive number and the tl_receive_msg is NULL.
 *  @returns ENOMEM if there is not enough memory left.
 */
int	ip_register( int protocol, services_t service, int phone, tl_received_msg_t tl_received_msg );

/** Initializes a new network interface specific data.
 *  Connects to the network interface layer module, reads the netif configuration, starts an ARP module if needed and sets the netif routing table.
 *  The device identifier and the nil service has to be set.
 *  @param[in,out] ip_netif Network interface specific data.
 *  @returns EOK on success.
 *  @returns ENOTSUP if DHCP is configured.
 *  @returns ENOTSUP if IPv6 is configured.
 *  @returns EINVAL if any of the addresses is invalid.
 *  @returns EINVAL if the used ARP module is not known.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the net_get_device_conf_req() function.
 *  @returns Other error codes as defined for the bind_service() function.
 *  @returns Other error codes as defined for the specific arp_device_req() function.
 *  @returns Other error codes as defined for the nil_packet_size_req() function.
 */
int	ip_netif_initialize( ip_netif_ref ip_netif );

/** Sends the packet or the packet queue via the specified route.
 *  The ICMP_HOST_UNREACH error notification may be sent if route hardware destination address is found.
 *  @param[in,out] packet The packet to be sent.
 *  @param[in] netif The target network interface.
 *  @param[in] route The target route.
 *  @param[in] src The source address.
 *  @param[in] dest The destination address.
 *  @param[in] error The error module service.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the arp_translate_req() function.
 *  @returns Other error codes as defined for the ip_prepare_packet() function.
 */
int	ip_send_route( packet_t packet, ip_netif_ref netif, ip_route_ref route, in_addr_t * src, in_addr_t dest, services_t error );

/** Prepares the outgoing packet or the packet queue.
 *  The packet queue is a fragmented packet
 *  Updates the first packet's IP header.
 *  Prefixes the additional packets with fragment headers.
 *  @param[in] source The source address.
 *  @param[in] dest The destination address.
 *  @param[in,out] packet The packet to be sent.
 *  @param[in] destination The destination hardware address.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet is too small to contain the IP header.
 *  @returns EINVAL if the packet is too long than the IP allows.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the packet_set_addr() function.
 */
int	ip_prepare_packet( in_addr_t * source, in_addr_t dest, packet_t packet, measured_string_ref destination );

/** Checks the packet queue lengths and fragments the packets if needed.
 *  The ICMP_FRAG_NEEDED error notification may be sent if the packet needs to be fragmented and the fragmentation is not allowed.
 *  @param[in,out] packet The packet or the packet queue to be checked.
 *  @param[in] prefix The minimum prefix size.
 *  @param[in] content The maximum content size.
 *  @param[in] suffix The minimum suffix size.
 *  @param[in] addr_len The minimum address length.
 *  @param[in] error The error module service.
 *  @returns The packet or the packet queue of the allowed length.
 *  @returns NULL if there are no packets left.
 */
packet_t	ip_split_packet( packet_t packet, size_t prefix, size_t content, size_t suffix, socklen_t addr_len, services_t error );

/** Checks the packet length and fragments it if needed.
 *  The new fragments are queued before the original packet.
 *  @param[in,out] packet The packet to be checked.
 *  @param[in] length The maximum packet length.
 *  @param[in] prefix The minimum prefix size.
 *  @param[in] suffix The minimum suffix size.
 *  @param[in] addr_len The minimum address length.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet_get_addr() function fails.
 *  @returns EINVAL if the packet does not contain the IP header.
 *  @returns EPERM if the packet needs to be fragmented and the fragmentation is not allowed.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns ENOMEM if there is no packet available.
 *  @returns ENOMEM if the packet is too small to contain the IP header.
 *  @returns Other error codes as defined for the packet_trim() function.
 *  @returns Other error codes as defined for the ip_create_middle_header() function.
 *  @returns Other error codes as defined for the ip_fragment_packet_data() function.
 */
int	ip_fragment_packet( packet_t packet, size_t length, size_t prefix, size_t suffix, socklen_t addr_len );

/** Fragments the packet from the end.
 *  @param[in] packet The packet to be fragmented.
 *  @param[in,out] new_packet The new packet fragment.
 *  @param[in,out] header The original packet header.
 *  @param[in,out] new_header The new packet fragment header.
 *  @param[in] length The new fragment length.
 *  @param[in] src The source address.
 *  @param[in] dest The destiantion address.
 *  @param[in] addrlen The address length.
 *  @returns EOK on success.
 *  @returns ENOMEM if the target packet is too small.
 *  @returns Other error codes as defined for the packet_set_addr() function.
 *  @returns Other error codes as defined for the pq_insert_after() function.
 */
int	ip_fragment_packet_data( packet_t packet, packet_t new_packet, ip_header_ref header, ip_header_ref new_header, size_t length, const struct sockaddr * src, const struct sockaddr * dest, socklen_t addrlen );

/** Prefixes a middle fragment header based on the last fragment header to the packet.
 *  @param[in] packet The packet to be prefixed.
 *  @param[in] last The last header to be copied.
 *  @returns The prefixed middle header.
 *  @returns NULL on error.
 */
ip_header_ref	ip_create_middle_header( packet_t packet, ip_header_ref last );

/** Copies the fragment header.
 *  Copies only the header itself and relevant IP options.
 *  @param[out] last The created header.
 *  @param[in] first The original header to be copied.
 */
void ip_create_last_header( ip_header_ref last, ip_header_ref first );

/** Returns the network interface's IP address.
 *  @param[in] netif The network interface.
 *  @returns The IP address.
 *  @returns NULL if no IP address was found.
 */
in_addr_t *	ip_netif_address( ip_netif_ref netif );

/** Searches all network interfaces if there is a suitable route.
 *  @param[in] destination The destination address.
 *  @returns The found route.
 *  @returns NULL if no route was found.
 */
ip_route_ref	ip_find_route( in_addr_t destination );

/** Searches the network interfaces if there is a suitable route.
 *  @param[in] netif The network interface to be searched for routes. May be NULL.
 *  @param[in] destination The destination address.
 *  @returns The found route.
 *  @returns NULL if no route was found.
 */
ip_route_ref	ip_netif_find_route( ip_netif_ref netif, in_addr_t destination );

/** Processes the received IP packet or the packet queue one by one.
 *  The packet is either passed to another module or released on error.
 *  @param[in] device_id The source device identifier.
 *  @param[in,out] packet The received packet.
 *  @returns EOK on success and the packet is no longer needed.
 *  @returns EINVAL if the packet is too small to carry the IP packet.
 *  @returns EINVAL if the received address lengths differs from the registered values.
 *  @returns ENOENT if the device is not found in the cache.
 *  @returns ENOENT if the protocol for the device is not found in the cache.
 *  @returns ENOMEM if there is not enough memory left.
 */
int	ip_receive_message( device_id_t device_id, packet_t packet );

/** Processes the received packet.
 *  The packet is either passed to another module or released on error.
 *  The ICMP_PARAM_POINTER error notification may be sent if the checksum is invalid.
 *  The ICMP_EXC_TTL error notification may be sent if the TTL is less than two (2).
 *  The ICMP_HOST_UNREACH error notification may be sent if no route was found.
 *  The ICMP_HOST_UNREACH error notification may be sent if the packet is for another host and the routing is disabled.
 *  @param[in] device_id The source device identifier.
 *  @param[in] packet The received packet to be processed.
 *  @returns EOK on success.
 *  @returns EINVAL if the TTL is less than two (2).
 *  @returns EINVAL if the checksum is invalid.
 *  @returns EAFNOSUPPORT if the address family is not supported.
 *  @returns ENOENT if no route was found.
 *  @returns ENOENT if the packet is for another host and the routing is disabled.
 */
int	ip_process_packet( device_id_t device_id, packet_t packet );

/** Returns the packet destination address from the IP header.
 *  @param[in] header The packet IP header to be read.
 *  @returns The packet destination address.
 */
in_addr_t	ip_get_destination( ip_header_ref header );

/** Delivers the packet to the local host.
 *  The packet is either passed to another module or released on error.
 *  The ICMP_PROT_UNREACH error notification may be sent if the protocol is not found.
 *  @param[in] device_id The source device identifier.
 *  @param[in] packet The packet to be delivered.
 *  @param[in] header The first packet IP header. May be NULL.
 *  @param[in] error The packet error service.
 *  @returns EOK on success.
 *  @returns ENOTSUP if the packet is a fragment.
 *  @returns EAFNOSUPPORT if the address family is not supported.
 *  @returns ENOENT if the target protocol is not found.
 *  @returns Other error codes as defined for the packet_set_addr() function.
 *  @returns Other error codes as defined for the packet_trim() function.
 *  @returns Other error codes as defined for the protocol specific tl_received_msg function.
 */
int	ip_deliver_local( device_id_t device_id, packet_t packet, ip_header_ref header, services_t error );

/** Prepares the ICMP notification packet.
 *  Releases additional packets and keeps only the first one.
 *  All packets is released on error.
 *  @param[in] error The packet error service.
 *  @param[in] packet The packet or the packet queue to be reported as faulty.
 *  @param[in] header The first packet IP header. May be NULL.
 *  @returns The found ICMP phone.
 *  @returns EINVAL if the error parameter is set.
 *  @returns EINVAL if the ICMP phone is not found.
 *  @returns EINVAL if the ip_prepare_icmp() fails.
 */
int	ip_prepare_icmp_and_get_phone( services_t error, packet_t packet, ip_header_ref header );

/** Returns the ICMP phone.
 *  Searches the registered protocols.
 *  @returns The found ICMP phone.
 *  @returns ENOENT if the ICMP is not registered.
 */
int	ip_get_icmp_phone( void );

/** Prepares the ICMP notification packet.
 *  Releases additional packets and keeps only the first one.
 *  @param[in] packet The packet or the packet queue to be reported as faulty.
 *  @param[in] header The first packet IP header. May be NULL.
 *  @returns EOK on success.
 *  @returns EINVAL if there are no data in the packet.
 *  @returns EINVAL if the packet is a fragment.
 *  @returns ENOMEM if the packet is too short to contain the IP header.
 *  @returns EAFNOSUPPORT if the address family is not supported.
 *  @returns EPERM if the protocol is not allowed to send ICMP notifications. The ICMP protocol itself.
 *  @returns Other error codes as defined for the packet_set_addr().
 */
int	ip_prepare_icmp( packet_t packet, ip_header_ref header );

/** Releases the packet and returns the result.
 *  @param[in] packet The packet queue to be released.
 *  @param[in] result The result to be returned.
 *  @return The result parameter.
 */
int	ip_release_and_return( packet_t packet, int result );

int ip_initialize( async_client_conn_t client_connection ){
	ERROR_DECLARE;

	fibril_rwlock_initialize( & ip_globals.lock );
	fibril_rwlock_write_lock( & ip_globals.lock );
	fibril_rwlock_initialize( & ip_globals.protos_lock );
	fibril_rwlock_initialize( & ip_globals.netifs_lock );
	ip_globals.packet_counter = 0;
	ip_globals.gateway.address.s_addr = 0;
	ip_globals.gateway.netmask.s_addr = 0;
	ip_globals.gateway.gateway.s_addr = 0;
	ip_globals.gateway.netif = NULL;
	ERROR_PROPAGATE( ip_netifs_initialize( & ip_globals.netifs ));
	ERROR_PROPAGATE( ip_protos_initialize( & ip_globals.protos ));
	ip_globals.client_connection = client_connection;
	ERROR_PROPAGATE( modules_initialize( & ip_globals.modules ));
	ERROR_PROPAGATE( add_module( NULL, & ip_globals.modules, ARP_NAME, ARP_FILENAME, SERVICE_ARP, arp_task_get_id(), arp_connect_module ));
	fibril_rwlock_write_unlock( & ip_globals.lock );
	return EOK;
}

int ip_device_req( int il_phone, device_id_t device_id, services_t netif ){
	ERROR_DECLARE;

	ip_netif_ref	ip_netif;
	ip_route_ref	route;
	int				index;
	char *			data;

	ip_netif = ( ip_netif_ref ) malloc( sizeof( ip_netif_t ));
	if( ! ip_netif ) return ENOMEM;
	if( ERROR_OCCURRED( ip_routes_initialize( & ip_netif->routes ))){
		free( ip_netif );
		return ERROR_CODE;
	}
	ip_netif->device_id = device_id;
	ip_netif->service = netif;
	ip_netif->state = NETIF_STOPPED;
	fibril_rwlock_write_lock( & ip_globals.netifs_lock );
	if( ERROR_OCCURRED( ip_netif_initialize( ip_netif ))){
		fibril_rwlock_write_unlock( & ip_globals.netifs_lock );
		ip_routes_destroy( & ip_netif->routes );
		free( ip_netif );
		return ERROR_CODE;
	}
	if( ip_netif->arp ) ++ ip_netif->arp->usage;
	// print the settings
	printf( "New device registered:\n\tid\t= %d\n\tphone\t= %d\n\tIPV\t= %d\n", ip_netif->device_id, ip_netif->phone, ip_netif->ipv );
	printf( "\tconfiguration\t= %s\n", ip_netif->dhcp ? "dhcp" : "static" );
	// TODO ipv6 addresses
	data = ( char * ) malloc( INET_ADDRSTRLEN );
	if( data ){
		for( index = 0; index < ip_routes_count( & ip_netif->routes ); ++ index ){
			route = ip_routes_get_index( & ip_netif->routes, index );
			if( route ){
				printf( "\tRouting %d:\n", index );
				inet_ntop( AF_INET, ( uint8_t * ) & route->address.s_addr, data, INET_ADDRSTRLEN );
				printf( "\t\taddress\t= %s\n", data );
				inet_ntop( AF_INET, ( uint8_t * ) & route->netmask.s_addr, data, INET_ADDRSTRLEN );
				printf( "\t\tnetmask\t= %s\n", data );
				inet_ntop( AF_INET, ( uint8_t * ) & route->gateway.s_addr, data, INET_ADDRSTRLEN );
				printf( "\t\tgateway\t= %s\n", data );
			}
		}
		inet_ntop( AF_INET, ( uint8_t * ) & ip_netif->broadcast.s_addr, data, INET_ADDRSTRLEN );
		printf( "\t\tbroadcast\t= %s\n", data );
		free( data );
	}
	fibril_rwlock_write_unlock( & ip_globals.netifs_lock );
	return EOK;
}

int ip_netif_initialize( ip_netif_ref ip_netif ){
	ERROR_DECLARE;

	measured_string_t	names[] = {{ "IPV", 3 }, { "IP_CONFIG", 9 }, { "IP_ADDR", 7 }, { "IP_NETMASK", 10 }, { "IP_GATEWAY", 10 }, { "IP_BROADCAST", 12 }, { "ARP", 3 }, { "IP_ROUTING", 10 }};
	measured_string_ref	configuration;
	size_t				count = sizeof( names ) / sizeof( measured_string_t );
	char *				data;
	measured_string_t	address;
	int					index;
	ip_route_ref		route;
	in_addr_t			gateway;

	ip_netif->arp = NULL;
	route = NULL;
	ip_netif->ipv = NET_DEFAULT_IPV;
	ip_netif->dhcp = false;
	ip_netif->routing = NET_DEFAULT_IP_ROUTING;
	configuration = & names[ 0 ];
	// get configuration
	ERROR_PROPAGATE( net_get_device_conf_req( ip_globals.net_phone, ip_netif->device_id, & configuration, count, & data ));
	if( configuration ){
		if( configuration[ 0 ].value ){
			ip_netif->ipv = strtol( configuration[ 0 ].value, NULL, 0 );
		}
		ip_netif->dhcp = ! str_lcmp( configuration[ 1 ].value, "dhcp", configuration[ 1 ].length );
		if( ip_netif->dhcp ){
			// TODO dhcp
			net_free_settings( configuration, data );
			return ENOTSUP;
		}else if( ip_netif->ipv == IPV4 ){
			route = ( ip_route_ref ) malloc( sizeof( ip_route_t ));
			if( ! route ){
				net_free_settings( configuration, data );
				return ENOMEM;
			}
			route->address.s_addr = 0;
			route->netmask.s_addr = 0;
			route->gateway.s_addr = 0;
			route->netif = ip_netif;
			index = ip_routes_add( & ip_netif->routes, route );
			if( index < 0 ){
				net_free_settings( configuration, data );
				free( route );
				return index;
			}
			if( ERROR_OCCURRED( inet_pton( AF_INET, configuration[ 2 ].value, ( uint8_t * ) & route->address.s_addr ))
			|| ERROR_OCCURRED( inet_pton( AF_INET, configuration[ 3 ].value, ( uint8_t * ) & route->netmask.s_addr ))
			|| ( inet_pton( AF_INET, configuration[ 4 ].value, ( uint8_t * ) & gateway.s_addr ) == EINVAL )
			|| ( inet_pton( AF_INET, configuration[ 5 ].value, ( uint8_t * ) & ip_netif->broadcast.s_addr ) == EINVAL )){
				net_free_settings( configuration, data );
				return EINVAL;
			}
		}else{
			// TODO ipv6 in separate module
			net_free_settings( configuration, data );
			return ENOTSUP;
		}
		if( configuration[ 6 ].value ){
			ip_netif->arp = get_running_module( & ip_globals.modules, configuration[ 6 ].value );
			if( ! ip_netif->arp ){
				printf( "Failed to start the arp %s\n", configuration[ 6 ].value );
				net_free_settings( configuration, data );
				return EINVAL;
			}
		}
		if( configuration[ 7 ].value ){
			ip_netif->routing = ( configuration[ 7 ].value[ 0 ] == 'y' );
		}
		net_free_settings( configuration, data );
	}
	// binds the netif service which also initializes the device
	ip_netif->phone = nil_bind_service( ip_netif->service, ( ipcarg_t ) ip_netif->device_id, SERVICE_IP, ip_globals.client_connection );
	if( ip_netif->phone < 0 ){
		printf( "Failed to contact the nil service %d\n", ip_netif->service );
		return ip_netif->phone;
	}
	// has to be after the device netif module initialization
	if( ip_netif->arp ){
		if( route ){
			address.value = ( char * ) & route->address.s_addr;
			address.length = CONVERT_SIZE( in_addr_t, char, 1 );
			ERROR_PROPAGATE( arp_device_req( ip_netif->arp->phone, ip_netif->device_id, SERVICE_IP, ip_netif->service, & address ));
		}else{
			ip_netif->arp = 0;
		}
	}
	// get packet dimensions
	ERROR_PROPAGATE( nil_packet_size_req( ip_netif->phone, ip_netif->device_id, & ip_netif->packet_dimension ));
	if( ip_netif->packet_dimension.content < IP_MIN_CONTENT ){
		printf( "Maximum transmission unit %d bytes is too small, at least %d bytes are needed\n", ip_netif->packet_dimension.content, IP_MIN_CONTENT );
		ip_netif->packet_dimension.content = IP_MIN_CONTENT;
	}
	index = ip_netifs_add( & ip_globals.netifs, ip_netif->device_id, ip_netif );
	if( index < 0 ) return index;
	if( gateway.s_addr ){
		// the default gateway
		ip_globals.gateway.address.s_addr = 0;
		ip_globals.gateway.netmask.s_addr = 0;
		ip_globals.gateway.gateway.s_addr = gateway.s_addr;
		ip_globals.gateway.netif = ip_netif;
	}
	return EOK;
}

int ip_mtu_changed_message( device_id_t device_id, size_t mtu ){
	ip_netif_ref	netif;

	fibril_rwlock_write_lock( & ip_globals.netifs_lock );
	netif = ip_netifs_find( & ip_globals.netifs, device_id );
	if( ! netif ){
		fibril_rwlock_write_unlock( & ip_globals.netifs_lock );
		return ENOENT;
	}
	netif->packet_dimension.content = mtu;
	printf( "ip - device %d changed mtu to %d\n\n", device_id, mtu );
	fibril_rwlock_write_unlock( & ip_globals.netifs_lock );
	return EOK;
}

int ip_device_state_message( device_id_t device_id, device_state_t state ){
	ip_netif_ref	netif;

	fibril_rwlock_write_lock( & ip_globals.netifs_lock );
	// find the device
	netif = ip_netifs_find( & ip_globals.netifs, device_id );
	if( ! netif ){
		fibril_rwlock_write_unlock( & ip_globals.netifs_lock );
		return ENOENT;
	}
	netif->state = state;
	printf( "ip - device %d changed state to %d\n\n", device_id, state );
	fibril_rwlock_write_unlock( & ip_globals.netifs_lock );
	return EOK;
}

int ip_connect_module( services_t service ){
	return EOK;
}

int ip_bind_service( services_t service, int protocol, services_t me, async_client_conn_t receiver, tl_received_msg_t received_msg ){
	return ip_register( protocol, me, 0, received_msg );
}

int ip_register( int protocol, services_t service, int phone, tl_received_msg_t received_msg ){
	ip_proto_ref	proto;
	int				index;

	if( !( protocol && service && (( phone > 0 ) || ( received_msg )))) return EINVAL;
	proto = ( ip_proto_ref ) malloc( sizeof( ip_protos_t ));
	if( ! proto ) return ENOMEM;
	proto->protocol = protocol;
	proto->service = service;
	proto->phone = phone;
	proto->received_msg = received_msg;
	fibril_rwlock_write_lock( & ip_globals.protos_lock );
	index = ip_protos_add( & ip_globals.protos, proto->protocol, proto );
	if( index < 0 ){
		fibril_rwlock_write_unlock( & ip_globals.protos_lock );
		free( proto );
		return index;
	}
	printf( "New protocol registered:\n\tprotocol\t= %d\n\tphone\t= %d\n", proto->protocol, proto->phone );
	fibril_rwlock_write_unlock( & ip_globals.protos_lock );
	return EOK;
}

int ip_send_msg( int il_phone, device_id_t device_id, packet_t packet, services_t sender, services_t error ){
	ERROR_DECLARE;

	int					addrlen;
	ip_netif_ref		netif;
	ip_route_ref		route;
	struct sockaddr *		addr;
	struct sockaddr_in *	address_in;
//	struct sockaddr_in6 *	address_in6;
	in_addr_t *			dest;
	in_addr_t *			src;
	int					phone;

	// addresses in the host byte order
	// should be the next hop address or the target destination address
	addrlen = packet_get_addr( packet, NULL, ( uint8_t ** ) & addr );
	if( addrlen < 0 ){
		return ip_release_and_return( packet, addrlen );
	}
	if(( size_t ) addrlen < sizeof( struct sockaddr )){
		return ip_release_and_return( packet, EINVAL );
	}
	switch( addr->sa_family ){
		case AF_INET:
			if( addrlen != sizeof( struct sockaddr_in )){
				return ip_release_and_return( packet, EINVAL );
			}
			address_in = ( struct sockaddr_in * ) addr;
			dest = & address_in->sin_addr;
			if( ! dest->s_addr ){
				dest->s_addr = IPV4_LOCALHOST_ADDRESS;
			}
			break;
		// TODO IPv6
/*		case AF_INET6:
			if( addrlen != sizeof( struct sockaddr_in6 )) return EINVAL;
			address_in6 = ( struct sockaddr_in6 * ) dest;
			address_in6.sin6_addr.s6_addr;
			IPV6_LOCALHOST_ADDRESS;
*/		default:
			return ip_release_and_return( packet, EAFNOSUPPORT );
	}
	fibril_rwlock_read_lock( & ip_globals.netifs_lock );
	// device specified?
	if( device_id > 0 ){
		netif = ip_netifs_find( & ip_globals.netifs, device_id );
		route = ip_netif_find_route( netif, * dest );
		if( netif && ( ! route ) && ( ip_globals.gateway.netif == netif )){
			route = & ip_globals.gateway;
		}
	}else{
		route = ip_find_route( * dest );
		netif = route ? route->netif : NULL;
	}
	if( !( netif && route )){
		fibril_rwlock_read_unlock( & ip_globals.netifs_lock );
		phone = ip_prepare_icmp_and_get_phone( error, packet, NULL );
		if( phone >= 0 ){
			// unreachable ICMP if no routing
			icmp_destination_unreachable_msg( phone, ICMP_NET_UNREACH, 0, packet );
		}
		return ENOENT;
	}
	if( error ){
		// do not send for broadcast, anycast packets or network broadcast
		if(( ! dest->s_addr )
		|| ( !( ~ dest->s_addr ))
		|| ( !( ~(( dest->s_addr & ( ~ route->netmask.s_addr )) | route->netmask.s_addr )))
		|| ( !( dest->s_addr & ( ~ route->netmask.s_addr )))){
			return ip_release_and_return( packet, EINVAL );
		}
	}
	if( route->address.s_addr == dest->s_addr ){
		// find the loopback device to deliver
		dest->s_addr = IPV4_LOCALHOST_ADDRESS;
		route = ip_find_route( * dest );
		netif = route ? route->netif : NULL;
		if( !( netif && route )){
			fibril_rwlock_read_unlock( & ip_globals.netifs_lock );
			phone = ip_prepare_icmp_and_get_phone( error, packet, NULL );
			if( phone >= 0 ){
				// unreachable ICMP if no routing
				icmp_destination_unreachable_msg( phone, ICMP_HOST_UNREACH, 0, packet );
			}
			return ENOENT;
		}
	}
	src = ip_netif_address( netif );
	if( ! src ){
		fibril_rwlock_read_unlock( & ip_globals.netifs_lock );
		return ip_release_and_return( packet, ENOENT );
	}
	ERROR_CODE = ip_send_route( packet, netif, route, src, * dest, error );
	fibril_rwlock_read_unlock( & ip_globals.netifs_lock );
	return ERROR_CODE;
}

in_addr_t * ip_netif_address( ip_netif_ref netif ){
	ip_route_ref	route;

	route = ip_routes_get_index( & netif->routes, 0 );
	return route ? & route->address : NULL;
}

int ip_send_route( packet_t packet, ip_netif_ref netif, ip_route_ref route, in_addr_t * src, in_addr_t dest, services_t error ){
	ERROR_DECLARE;

	measured_string_t	destination;
	measured_string_ref	translation;
	char *				data;
	int					phone;

	// get destination hardware address
	if( netif->arp && ( route->address.s_addr != dest.s_addr )){
		destination.value = route->gateway.s_addr ? ( char * ) & route->gateway.s_addr : ( char * ) & dest.s_addr;
		destination.length = CONVERT_SIZE( dest.s_addr, char, 1 );
		if( ERROR_OCCURRED( arp_translate_req( netif->arp->phone, netif->device_id, SERVICE_IP, & destination, & translation, & data ))){
//			sleep( 1 );
//			ERROR_PROPAGATE( arp_translate_req( netif->arp->phone, netif->device_id, SERVICE_IP, & destination, & translation, & data ));
			pq_release( ip_globals.net_phone, packet_get_id( packet ));
			return ERROR_CODE;
		}
		if( !( translation && translation->value )){
			if( translation ){
				free( translation );
				free( data );
			}
			phone = ip_prepare_icmp_and_get_phone( error, packet, NULL );
			if( phone >= 0 ){
				// unreachable ICMP if no routing
				icmp_destination_unreachable_msg( phone, ICMP_HOST_UNREACH, 0, packet );
			}
			return EINVAL;
		}
	}else translation = NULL;
	if( ERROR_OCCURRED( ip_prepare_packet( src, dest, packet, translation ))){
		pq_release( ip_globals.net_phone, packet_get_id( packet ));
	}else{
		packet = ip_split_packet( packet, netif->packet_dimension.prefix, netif->packet_dimension.content, netif->packet_dimension.suffix, netif->packet_dimension.addr_len, error );
		if( packet ){
			nil_send_msg( netif->phone, netif->device_id, packet, SERVICE_IP );
		}
	}
	if( translation ){
		free( translation );
		free( data );
	}
	return ERROR_CODE;
}

int ip_prepare_packet( in_addr_t * source, in_addr_t dest, packet_t packet, measured_string_ref destination ){
	ERROR_DECLARE;

	size_t				length;
	ip_header_ref		header;
	ip_header_ref		last_header;
	ip_header_ref		middle_header;
	packet_t			next;

	length = packet_get_data_length( packet );
	if(( length < sizeof( ip_header_t )) || ( length > IP_MAX_CONTENT )) return EINVAL;
	header = ( ip_header_ref ) packet_get_data( packet );
	if( destination ){
		ERROR_PROPAGATE( packet_set_addr( packet, NULL, ( uint8_t * ) destination->value, CONVERT_SIZE( char, uint8_t, destination->length )));
	}else{
		ERROR_PROPAGATE( packet_set_addr( packet, NULL, NULL, 0 ));
	}
	header->version = IPV4;
	header->fragment_offset_high = 0;
	header->fragment_offset_low = 0;
	header->header_checksum = 0;
	if( source ) header->source_address = source->s_addr;
	header->destination_address = dest.s_addr;
	fibril_rwlock_write_lock( & ip_globals.lock );
	++ ip_globals.packet_counter;
	header->identification = htons( ip_globals.packet_counter );
	fibril_rwlock_write_unlock( & ip_globals.lock );
//	length = packet_get_data_length( packet );
	if( pq_next( packet )){
		last_header = ( ip_header_ref ) malloc( IP_HEADER_LENGTH( header ));
		if( ! last_header ) return ENOMEM;
		ip_create_last_header( last_header, header );
		next = pq_next( packet );
		while( pq_next( next )){
			middle_header = ( ip_header_ref ) packet_prefix( next, IP_HEADER_LENGTH( last_header ));
			if( ! middle_header ) return ENOMEM;
			memcpy( middle_header, last_header, IP_HEADER_LENGTH( last_header ));
			header->flags |= IPFLAG_MORE_FRAGMENTS;
			middle_header->total_length = htons( packet_get_data_length( next ));
			middle_header->fragment_offset_high = IP_COMPUTE_FRAGMENT_OFFSET_HIGH( length );
			middle_header->fragment_offset_low = IP_COMPUTE_FRAGMENT_OFFSET_LOW( length );
			middle_header->header_checksum = IP_HEADER_CHECKSUM( middle_header );
			if( destination ){
				ERROR_PROPAGATE( packet_set_addr( next, NULL, ( uint8_t * ) destination->value, CONVERT_SIZE( char, uint8_t, destination->length )));
			}
			length += packet_get_data_length( next );
			next = pq_next( next );
		}
		middle_header = ( ip_header_ref ) packet_prefix( next, IP_HEADER_LENGTH( last_header ));
		if( ! middle_header ) return ENOMEM;
		memcpy( middle_header, last_header, IP_HEADER_LENGTH( last_header ));
		middle_header->total_length = htons( packet_get_data_length( next ));
		middle_header->fragment_offset_high = IP_COMPUTE_FRAGMENT_OFFSET_HIGH( length );
		middle_header->fragment_offset_low = IP_COMPUTE_FRAGMENT_OFFSET_LOW( length );
		middle_header->header_checksum = IP_HEADER_CHECKSUM( middle_header );
		if( destination ){
			ERROR_PROPAGATE( packet_set_addr( next, NULL, ( uint8_t * ) destination->value, CONVERT_SIZE( char, uint8_t, destination->length )));
		}
		length += packet_get_data_length( next );
		free( last_header );
		header->flags |= IPFLAG_MORE_FRAGMENTS;
	}
	header->total_length = htons( length );
	// unnecessary for all protocols
	header->header_checksum = IP_HEADER_CHECKSUM( header );
	return EOK;
}

int ip_message( ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count ){
	ERROR_DECLARE;

	packet_t				packet;
	struct sockaddr *		addr;
	size_t					addrlen;
	ip_pseudo_header_ref	header;
	size_t					headerlen;

	* answer_count = 0;
	switch( IPC_GET_METHOD( * call )){
		case IPC_M_PHONE_HUNGUP:
			return EOK;
		case NET_IL_DEVICE:
			return ip_device_req( 0, IPC_GET_DEVICE( call ), IPC_GET_SERVICE( call ));
		case IPC_M_CONNECT_TO_ME:
			return ip_register( IL_GET_PROTO( call ), IL_GET_SERVICE( call ), IPC_GET_PHONE( call ), NULL );
		case NET_IL_SEND:
			ERROR_PROPAGATE( packet_translate( ip_globals.net_phone, & packet, IPC_GET_PACKET( call )));
			return ip_send_msg( 0, IPC_GET_DEVICE( call ), packet, 0, IPC_GET_ERROR( call ));
		case NET_IL_DEVICE_STATE:
			return ip_device_state_message( IPC_GET_DEVICE( call ), IPC_GET_STATE( call ));
		case NET_IL_RECEIVED:
			ERROR_PROPAGATE( packet_translate( ip_globals.net_phone, & packet, IPC_GET_PACKET( call )));
			return ip_receive_message( IPC_GET_DEVICE( call ), packet );
		case NET_IP_RECEIVED_ERROR:
			ERROR_PROPAGATE( packet_translate( ip_globals.net_phone, & packet, IPC_GET_PACKET( call )));
			return ip_received_error_msg( 0, IPC_GET_DEVICE( call ), packet, IPC_GET_TARGET( call ), IPC_GET_ERROR( call ));
		case NET_IP_ADD_ROUTE:
			return ip_add_route_req( 0, IPC_GET_DEVICE( call ), IP_GET_ADDRESS( call ), IP_GET_NETMASK( call ), IP_GET_GATEWAY( call ));
		case NET_IP_SET_GATEWAY:
			return ip_set_gateway_req( 0, IPC_GET_DEVICE( call ), IP_GET_GATEWAY( call ));
		case NET_IP_GET_ROUTE:
			ERROR_PROPAGATE( data_receive(( void ** ) & addr, & addrlen ));
			ERROR_PROPAGATE( ip_get_route_req( 0, IP_GET_PROTOCOL( call ), addr, ( socklen_t ) addrlen, IPC_SET_DEVICE( answer ), & header, & headerlen ));
			* IP_SET_HEADERLEN( answer ) = headerlen;
			* answer_count = 2;
			if( ! ERROR_OCCURRED( data_reply( & headerlen, sizeof( headerlen )))){
				ERROR_CODE = data_reply( header, headerlen );
			}
			free( header );
			return ERROR_CODE;
		case NET_IL_PACKET_SPACE:
			ERROR_PROPAGATE( ip_packet_size_message( IPC_GET_DEVICE( call ), IPC_SET_ADDR( answer ), IPC_SET_PREFIX( answer ), IPC_SET_CONTENT( answer ), IPC_SET_SUFFIX( answer )));
			* answer_count = 3;
			return EOK;
		case NET_IL_MTU_CHANGED:
			return ip_mtu_changed_message( IPC_GET_DEVICE( call ), IPC_GET_MTU( call ));
	}
	return ENOTSUP;
}

int ip_packet_size_req( int ip_phone, device_id_t device_id, packet_dimension_ref packet_dimension ){
	if( ! packet_dimension ) return EBADMEM;
	return ip_packet_size_message( device_id, & packet_dimension->addr_len, & packet_dimension->prefix, & packet_dimension->content, & packet_dimension->suffix );
}

int ip_packet_size_message( device_id_t device_id, size_t * addr_len, size_t * prefix, size_t * content, size_t * suffix ){
	ip_netif_ref	netif;
	int				index;

	if( !( addr_len && prefix && content && suffix )) return EBADMEM;
	* content = IP_MAX_CONTENT - IP_PREFIX;
	fibril_rwlock_read_lock( & ip_globals.netifs_lock );
	if( device_id < 0 ){
		* addr_len = IP_ADDR;
		* prefix = 0;
		* suffix = 0;
		for( index = ip_netifs_count( & ip_globals.netifs ) - 1; index >= 0; -- index ){
			netif = ip_netifs_get_index( & ip_globals.netifs, index );
			if( netif ){
				if( netif->packet_dimension.addr_len > * addr_len ) * addr_len = netif->packet_dimension.addr_len;
				if( netif->packet_dimension.prefix > * prefix ) * prefix = netif->packet_dimension.prefix;
				if( netif->packet_dimension.suffix > * suffix ) * suffix = netif->packet_dimension.suffix;
			}
		}
		* prefix = * prefix + IP_PREFIX;
		* suffix = * suffix + IP_SUFFIX;
	}else{
		netif = ip_netifs_find( & ip_globals.netifs, device_id );
		if( ! netif ){
			fibril_rwlock_read_unlock( & ip_globals.netifs_lock );
			return ENOENT;
		}
		* addr_len = ( netif->packet_dimension.addr_len > IP_ADDR ) ? netif->packet_dimension.addr_len : IP_ADDR;
		* prefix = netif->packet_dimension.prefix + IP_PREFIX;
		* suffix = netif->packet_dimension.suffix + IP_SUFFIX;
	}
	fibril_rwlock_read_unlock( & ip_globals.netifs_lock );
	return EOK;
}

int ip_add_route_req( int ip_phone, device_id_t device_id, in_addr_t address, in_addr_t netmask, in_addr_t gateway ){
	ip_route_ref	route;
	ip_netif_ref	netif;
	int				index;

	fibril_rwlock_write_lock( & ip_globals.netifs_lock );
	netif = ip_netifs_find( & ip_globals.netifs, device_id );
	if( ! netif ){
		fibril_rwlock_write_unlock( & ip_globals.netifs_lock );
		return ENOENT;
	}
	route = ( ip_route_ref ) malloc( sizeof( ip_route_t ));
	if( ! route ){
		fibril_rwlock_write_unlock( & ip_globals.netifs_lock );
		return ENOMEM;
	}
	route->address.s_addr = address.s_addr;
	route->netmask.s_addr = netmask.s_addr;
	route->gateway.s_addr = gateway.s_addr;
	route->netif = netif;
	index = ip_routes_add( & netif->routes, route );
	if( index < 0 ) free( route );
	fibril_rwlock_write_unlock( & ip_globals.netifs_lock );
	return index;
}

ip_route_ref ip_find_route( in_addr_t destination ){
	int				index;
	ip_route_ref	route;
	ip_netif_ref	netif;

	// start with the last netif - the newest one
	index = ip_netifs_count( & ip_globals.netifs ) - 1;
	while( index >= 0 ){
		netif = ip_netifs_get_index( & ip_globals.netifs, index );
		if( netif && ( netif->state == NETIF_ACTIVE )){
			route = ip_netif_find_route( netif, destination );
			if( route ) return route;
		}
		-- index;
	}
	return & ip_globals.gateway;
}

ip_route_ref ip_netif_find_route( ip_netif_ref netif, in_addr_t destination ){
	int				index;
	ip_route_ref	route;

	if( netif ){
		// start with the first one - the direct route
		for( index = 0; index < ip_routes_count( & netif->routes ); ++ index ){
			route = ip_routes_get_index( & netif->routes, index );
			if( route && (( route->address.s_addr & route->netmask.s_addr ) == ( destination.s_addr & route->netmask.s_addr ))){
				return route;
			}
		}
	}
	return NULL;
}

int ip_set_gateway_req( int ip_phone, device_id_t device_id, in_addr_t gateway ){
	ip_netif_ref	netif;

	fibril_rwlock_write_lock( & ip_globals.netifs_lock );
	netif = ip_netifs_find( & ip_globals.netifs, device_id );
	if( ! netif ){
		fibril_rwlock_write_unlock( & ip_globals.netifs_lock );
		return ENOENT;
	}
	ip_globals.gateway.address.s_addr = 0;
	ip_globals.gateway.netmask.s_addr = 0;
	ip_globals.gateway.gateway.s_addr = gateway.s_addr;
	ip_globals.gateway.netif = netif;
	fibril_rwlock_write_unlock( & ip_globals.netifs_lock );
	return EOK;
}

packet_t ip_split_packet( packet_t packet, size_t prefix, size_t content, size_t suffix, socklen_t addr_len, services_t error ){
	size_t			length;
	packet_t		next;
	packet_t		new_packet;
	int				result;
	int				phone;

	next = packet;
	// check all packets
	while( next ){
		length = packet_get_data_length( next );
		// too long?
		if( length > content ){
			result = ip_fragment_packet( next, content, prefix, suffix, addr_len );
			if( result != EOK ){
				new_packet = pq_detach( next );
				if( next == packet ){
					// the new first packet of the queue
					packet = new_packet;
				}
				// fragmentation needed?
				if( result == EPERM ){
					phone = ip_prepare_icmp_and_get_phone( error, next, NULL );
					if( phone >= 0 ){
						// fragmentation necessary ICMP
						icmp_destination_unreachable_msg( phone, ICMP_FRAG_NEEDED, content, next );
					}
				}else{
					pq_release( ip_globals.net_phone, packet_get_id( next ));
				}
				next = new_packet;
				continue;
			}
		}
		next = pq_next( next );
	}
	return packet;
}

int ip_fragment_packet( packet_t packet, size_t length, size_t prefix, size_t suffix, socklen_t addr_len ){
	ERROR_DECLARE;

	packet_t		new_packet;
	ip_header_ref	header;
	ip_header_ref	middle_header;
	ip_header_ref	last_header;
	struct sockaddr *		src;
	struct sockaddr *		dest;
	socklen_t		addrlen;
	int				result;

	result = packet_get_addr( packet, ( uint8_t ** ) & src, ( uint8_t ** ) & dest );
	if( result <= 0 ) return EINVAL;
	addrlen = ( socklen_t ) result;
	if( packet_get_data_length( packet ) <= sizeof( ip_header_t )) return ENOMEM;
	// get header
	header = ( ip_header_ref ) packet_get_data( packet );
	if( ! header ) return EINVAL;
	// fragmentation forbidden?
	if( header->flags & IPFLAG_DONT_FRAGMENT ){
		return EPERM;
	}
	// create the last fragment
	new_packet = packet_get_4( ip_globals.net_phone, prefix, length, suffix, (( addrlen > addr_len ) ? addrlen : addr_len ));
	if( ! new_packet ) return ENOMEM;
	// allocate as much as originally
	last_header = ( ip_header_ref ) packet_suffix( new_packet, IP_HEADER_LENGTH( header ));
	if( ! last_header ){
		return ip_release_and_return( packet, ENOMEM );
	}
	ip_create_last_header( last_header, header );
	// trim the unused space
	if( ERROR_OCCURRED( packet_trim( new_packet, 0, IP_HEADER_LENGTH( header ) - IP_HEADER_LENGTH( last_header )))){
		return ip_release_and_return( packet, ERROR_CODE );
	}
	// biggest multiple of 8 lower than content
	// TODO even fragmentation?
	length = length & ( ~ 0x7 );// ( content / 8 ) * 8
	if( ERROR_OCCURRED( ip_fragment_packet_data( packet, new_packet, header, last_header, (( IP_HEADER_DATA_LENGTH( header ) - (( length - IP_HEADER_LENGTH( header )) & ( ~ 0x7 ))) % (( length - IP_HEADER_LENGTH( last_header )) & ( ~ 0x7 ))), src, dest, addrlen ))){
		return ip_release_and_return( packet, ERROR_CODE );
	}
	// mark the first as fragmented
	header->flags |= IPFLAG_MORE_FRAGMENTS;
	// create middle framgents
	while( IP_TOTAL_LENGTH( header ) > length ){
		new_packet = packet_get_4( ip_globals.net_phone, prefix, length, suffix, (( addrlen >= addr_len ) ? addrlen : addr_len ));
		if( ! new_packet ) return ENOMEM;
		middle_header = ip_create_middle_header( new_packet, last_header );
		if( ! middle_header ){
			return ip_release_and_return( packet, ENOMEM );
		}
		if( ERROR_OCCURRED( ip_fragment_packet_data( packet, new_packet, header, middle_header, ( length - IP_HEADER_LENGTH( middle_header )) & ( ~ 0x7 ), src, dest, addrlen ))){
			return ip_release_and_return( packet, ERROR_CODE );
		}
	}
	// finish the first fragment
	header->header_checksum = IP_HEADER_CHECKSUM( header );
	return EOK;
}

int ip_fragment_packet_data( packet_t packet, packet_t new_packet, ip_header_ref header, ip_header_ref new_header, size_t length, const struct sockaddr * src, const struct sockaddr * dest, socklen_t addrlen ){
	ERROR_DECLARE;

	void *			data;
	size_t			offset;

	data = packet_suffix( new_packet, length );
	if( ! data ) return ENOMEM;
	memcpy( data, (( void * ) header ) + IP_TOTAL_LENGTH( header ) - length, length );
	ERROR_PROPAGATE( packet_trim( packet, 0, length ));
	header->total_length = htons( IP_TOTAL_LENGTH( header ) - length );
	new_header->total_length = htons( IP_HEADER_LENGTH( new_header ) + length );
	offset = IP_FRAGMENT_OFFSET( header ) + IP_HEADER_DATA_LENGTH( header );
	new_header->fragment_offset_high = IP_COMPUTE_FRAGMENT_OFFSET_HIGH( offset );
	new_header->fragment_offset_low = IP_COMPUTE_FRAGMENT_OFFSET_LOW( offset );
	new_header->header_checksum = IP_HEADER_CHECKSUM( new_header );
	ERROR_PROPAGATE( packet_set_addr( new_packet, ( const uint8_t * ) src, ( const uint8_t * ) dest, addrlen ));
	return pq_insert_after( packet, new_packet );
}

ip_header_ref ip_create_middle_header( packet_t packet, ip_header_ref last ){
	ip_header_ref	middle;

	middle = ( ip_header_ref ) packet_suffix( packet, IP_HEADER_LENGTH( last ));
	if( ! middle ) return NULL;
	memcpy( middle, last, IP_HEADER_LENGTH( last ));
	middle->flags |= IPFLAG_MORE_FRAGMENTS;
	return middle;
}

void ip_create_last_header( ip_header_ref last, ip_header_ref first ){
	ip_option_ref	option;
	size_t			next;
	size_t			length;

	// copy first itself
	memcpy( last, first, sizeof( ip_header_t ));
	length = sizeof( ip_header_t );
	next = sizeof( ip_header_t );
	// process all ip options
	while( next < first->header_length ){
		option = ( ip_option_ref ) ((( uint8_t * ) first ) + next );
		// skip end or noop
		if(( option->type == IPOPT_END ) || ( option->type == IPOPT_NOOP )){
			++ next;
		}else{
			// copy if said so or skip
			if( IPOPT_COPIED( option->type )){
				memcpy((( uint8_t * ) last ) + length, (( uint8_t * ) first ) + next, option->length );
				length += option->length;
			}
			// next option
			next += option->length;
		}
	}
	// align 4 byte boundary
	if( length % 4 ){
		bzero((( uint8_t * ) last ) + length, 4 - ( length % 4 ));
		last->header_length = length / 4 + 1;
	}else{
		last->header_length = length / 4;
	}
	last->header_checksum = 0;
}

int ip_receive_message( device_id_t device_id, packet_t packet ){
	packet_t		next;

	do{
		next = pq_detach( packet );
		ip_process_packet( device_id, packet );
		packet = next;
	}while( packet );
	return EOK;
}

int ip_process_packet( device_id_t device_id, packet_t packet ){
	ERROR_DECLARE;

	ip_header_ref	header;
	in_addr_t		dest;
	ip_route_ref	route;
	int				phone;
	struct sockaddr *	addr;
	struct sockaddr_in	addr_in;
//	struct sockaddr_in	addr_in6;
	socklen_t		addrlen;

	header = ( ip_header_ref ) packet_get_data( packet );
	if( ! header ){
		return ip_release_and_return( packet, ENOMEM );
	}
	// checksum
	if(( header->header_checksum ) && ( IP_HEADER_CHECKSUM( header ) != IP_CHECKSUM_ZERO )){
		phone = ip_prepare_icmp_and_get_phone( 0, packet, header );
		if( phone >= 0 ){
			// checksum error ICMP
			icmp_parameter_problem_msg( phone, ICMP_PARAM_POINTER, (( size_t ) (( void * ) & header->header_checksum )) - (( size_t ) (( void * ) header )), packet );
		}
		return EINVAL;
	}
	if( header->ttl <= 1 ){
		phone = ip_prepare_icmp_and_get_phone( 0, packet, header );
		if( phone >= 0 ){
			// ttl oxceeded ICMP
			icmp_time_exceeded_msg( phone, ICMP_EXC_TTL, packet );
		}
		return EINVAL;
	}
	// process ipopt and get destination
	dest = ip_get_destination( header );
	// set the addrination address
	switch( header->version ){
		case IPVERSION:
			addrlen = sizeof( addr_in );
			bzero( & addr_in, addrlen );
			addr_in.sin_family = AF_INET;
			memcpy( & addr_in.sin_addr.s_addr, & dest, sizeof( dest ));
			addr = ( struct sockaddr * ) & addr_in;
			break;
/*		case IPv6VERSION:
			addrlen = sizeof( dest_in6 );
			bzero( & dest_in6, addrlen );
			dest_in6.sin6_family = AF_INET6;
			memcpy( & dest_in6.sin6_addr.s6_addr, );
			dest = ( struct sockaddr * ) & dest_in;
			break;
*/		default:
			return ip_release_and_return( packet, EAFNOSUPPORT );
	}
	ERROR_PROPAGATE( packet_set_addr( packet, NULL, ( uint8_t * ) & addr, addrlen ));
	route = ip_find_route( dest );
	if( ! route ){
		phone = ip_prepare_icmp_and_get_phone( 0, packet, header );
		if( phone >= 0 ){
			// unreachable ICMP
			icmp_destination_unreachable_msg( phone, ICMP_HOST_UNREACH, 0, packet );
		}
		return ENOENT;
	}
	if( route->address.s_addr == dest.s_addr ){
		// local delivery
		return ip_deliver_local( device_id, packet, header, 0 );
	}else{
		// only if routing enabled
		if( route->netif->routing ){
			-- header->ttl;
			return ip_send_route( packet, route->netif, route, NULL, dest, 0 );
		}else{
			phone = ip_prepare_icmp_and_get_phone( 0, packet, header );
			if( phone >= 0 ){
				// unreachable ICMP if no routing
				icmp_destination_unreachable_msg( phone, ICMP_HOST_UNREACH, 0, packet );
			}
			return ENOENT;
		}
	}
}

int ip_received_error_msg( int ip_phone, device_id_t device_id, packet_t packet, services_t target, services_t error ){
	uint8_t *			data;
	int					offset;
	icmp_type_t			type;
	icmp_code_t			code;
	ip_netif_ref		netif;
	measured_string_t	address;
	ip_route_ref		route;
	ip_header_ref		header;

	switch( error ){
		case SERVICE_ICMP:
			offset = icmp_client_process_packet( packet, & type, & code, NULL, NULL );
			if( offset < 0 ){
				return ip_release_and_return( packet, ENOMEM );
			}
			data = packet_get_data( packet );
			header = ( ip_header_ref )( data + offset );
			// destination host unreachable?
			if(( type == ICMP_DEST_UNREACH ) && ( code == ICMP_HOST_UNREACH )){
				fibril_rwlock_read_lock( & ip_globals.netifs_lock );
				netif = ip_netifs_find( & ip_globals.netifs, device_id );
				if( netif && netif->arp ){
					route = ip_routes_get_index( & netif->routes, 0 );
					// from the same network?
					if( route && (( route->address.s_addr & route->netmask.s_addr ) == ( header->destination_address & route->netmask.s_addr ))){
						// clear the ARP mapping if any
						address.value = ( char * ) & header->destination_address;
						address.length = CONVERT_SIZE( uint8_t, char, sizeof( header->destination_address ));
						arp_clear_address_req( netif->arp->phone, netif->device_id, SERVICE_IP, & address );
					}
				}
				fibril_rwlock_read_unlock( & ip_globals.netifs_lock );
			}
			break;
		default:
			return ip_release_and_return( packet, ENOTSUP );
	}
	return ip_deliver_local( device_id, packet, header, error );
}

int ip_deliver_local( device_id_t device_id, packet_t packet, ip_header_ref header, services_t error ){
	ERROR_DECLARE;

	ip_proto_ref	proto;
	int				phone;
	services_t		service;
	tl_received_msg_t	received_msg;
	struct sockaddr *	src;
	struct sockaddr *	dest;
	struct sockaddr_in	src_in;
	struct sockaddr_in	dest_in;
//	struct sockaddr_in	src_in6;
//	struct sockaddr_in	dest_in6;
	socklen_t		addrlen;

	if(( header->flags & IPFLAG_MORE_FRAGMENTS ) || IP_FRAGMENT_OFFSET( header )){
		// TODO fragmented
		return ENOTSUP;
	}else{
		switch( header->version ){
			case IPVERSION:
				addrlen = sizeof( src_in );
				bzero( & src_in, addrlen );
				src_in.sin_family = AF_INET;
				memcpy( & dest_in, & src_in, addrlen );
				memcpy( & src_in.sin_addr.s_addr, & header->source_address, sizeof( header->source_address ));
				memcpy( & dest_in.sin_addr.s_addr, & header->destination_address, sizeof( header->destination_address ));
				src = ( struct sockaddr * ) & src_in;
				dest = ( struct sockaddr * ) & dest_in;
				break;
/*			case IPv6VERSION:
				addrlen = sizeof( src_in6 );
				bzero( & src_in6, addrlen );
				src_in6.sin6_family = AF_INET6;
				memcpy( & dest_in6, & src_in6, addrlen );
				memcpy( & src_in6.sin6_addr.s6_addr, );
				memcpy( & dest_in6.sin6_addr.s6_addr, );
				src = ( struct sockaddr * ) & src_in;
				dest = ( struct sockaddr * ) & dest_in;
				break;
*/			default:
				return ip_release_and_return( packet, EAFNOSUPPORT );
		}
		if( ERROR_OCCURRED( packet_set_addr( packet, ( uint8_t * ) src, ( uint8_t * ) dest, addrlen ))){
			return ip_release_and_return( packet, ERROR_CODE );
		}
		// trim padding if present
		if(( ! error ) && ( IP_TOTAL_LENGTH( header ) < packet_get_data_length( packet ))){
			if( ERROR_OCCURRED( packet_trim( packet, 0, packet_get_data_length( packet ) - IP_TOTAL_LENGTH( header )))){
				return ip_release_and_return( packet, ERROR_CODE );
			}
		}
		fibril_rwlock_read_lock( & ip_globals.protos_lock );
		proto = ip_protos_find( & ip_globals.protos, header->protocol );
		if( ! proto ){
			fibril_rwlock_read_unlock( & ip_globals.protos_lock );
			phone = ip_prepare_icmp_and_get_phone( error, packet, header );
			if( phone >= 0 ){
				// unreachable ICMP
				icmp_destination_unreachable_msg( phone, ICMP_PROT_UNREACH, 0, packet );
			}
			return ENOENT;
		}
		if( proto->received_msg ){
			service = proto->service;
			received_msg = proto->received_msg;
			fibril_rwlock_read_unlock( & ip_globals.protos_lock );
			ERROR_CODE = received_msg( device_id, packet, service, error );
		}else{
			ERROR_CODE = tl_received_msg( proto->phone, device_id, packet, proto->service, error );
			fibril_rwlock_read_unlock( & ip_globals.protos_lock );
		}
		return ERROR_CODE;
	}
}

in_addr_t ip_get_destination( ip_header_ref header ){
	in_addr_t	destination;

	// TODO search set ipopt route?
	destination.s_addr = header->destination_address;
	return destination;
}

int ip_prepare_icmp( packet_t packet, ip_header_ref header ){
	packet_t	next;
	struct sockaddr *	dest;
	struct sockaddr_in	dest_in;
//	struct sockaddr_in	dest_in6;
	socklen_t		addrlen;

	// detach the first packet and release the others
	next = pq_detach( packet );
	if( next ){
		pq_release( ip_globals.net_phone, packet_get_id( next ));
	}
	if( ! header ){
		if( packet_get_data_length( packet ) <= sizeof( ip_header_t )) return ENOMEM;
		// get header
		header = ( ip_header_ref ) packet_get_data( packet );
		if( ! header ) return EINVAL;
	}
	// only for the first fragment
	if( IP_FRAGMENT_OFFSET( header )) return EINVAL;
	// not for the ICMP protocol
	if( header->protocol == IPPROTO_ICMP ){
		return EPERM;
	}
	// set the destination address
	switch( header->version ){
		case IPVERSION:
			addrlen = sizeof( dest_in );
			bzero( & dest_in, addrlen );
			dest_in.sin_family = AF_INET;
			memcpy( & dest_in.sin_addr.s_addr, & header->source_address, sizeof( header->source_address ));
			dest = ( struct sockaddr * ) & dest_in;
			break;
/*		case IPv6VERSION:
			addrlen = sizeof( dest_in6 );
			bzero( & dest_in6, addrlen );
			dest_in6.sin6_family = AF_INET6;
			memcpy( & dest_in6.sin6_addr.s6_addr, );
			dest = ( struct sockaddr * ) & dest_in;
			break;
*/		default:
			return EAFNOSUPPORT;
	}
	return packet_set_addr( packet, NULL, ( uint8_t * ) dest, addrlen );
}

int ip_get_icmp_phone( void ){
	ip_proto_ref	proto;
	int				phone;

	fibril_rwlock_read_lock( & ip_globals.protos_lock );
	proto = ip_protos_find( & ip_globals.protos, IPPROTO_ICMP );
	phone = proto ? proto->phone : ENOENT;
	fibril_rwlock_read_unlock( & ip_globals.protos_lock );
	return phone;
}

int ip_prepare_icmp_and_get_phone( services_t error, packet_t packet, ip_header_ref header ){
	int	phone;

	phone = ip_get_icmp_phone();
	if( error || ( phone < 0 ) || ip_prepare_icmp( packet, header )){
		return ip_release_and_return( packet, EINVAL );
	}
	return phone;
}

int	ip_release_and_return( packet_t packet, int result ){
	pq_release( ip_globals.net_phone, packet_get_id( packet ));
	return result;
}

int ip_get_route_req( int ip_phone, ip_protocol_t protocol, const struct sockaddr * destination, socklen_t addrlen, device_id_t * device_id, ip_pseudo_header_ref * header, size_t * headerlen ){
	struct sockaddr_in *	address_in;
//	struct sockaddr_in6 *	address_in6;
	in_addr_t *				dest;
	in_addr_t *				src;
	ip_route_ref			route;
	ipv4_pseudo_header_ref	header_in;

	if( !( destination && ( addrlen > 0 ))) return EINVAL;
	if( !( device_id && header && headerlen )) return EBADMEM;
	if(( size_t ) addrlen < sizeof( struct sockaddr )){
		return EINVAL;
	}
	switch( destination->sa_family ){
		case AF_INET:
			if( addrlen != sizeof( struct sockaddr_in )){
				return EINVAL;
			}
			address_in = ( struct sockaddr_in * ) destination;
			dest = & address_in->sin_addr;
			if( ! dest->s_addr ){
				dest->s_addr = IPV4_LOCALHOST_ADDRESS;
			}
			break;
		// TODO IPv6
/*		case AF_INET6:
			if( addrlen != sizeof( struct sockaddr_in6 )) return EINVAL;
			address_in6 = ( struct sockaddr_in6 * ) dest;
			address_in6.sin6_addr.s6_addr;
*/		default:
			return EAFNOSUPPORT;
	}
	fibril_rwlock_read_lock( & ip_globals.lock );
	route = ip_find_route( * dest );
	if( !( route && route->netif )){
		fibril_rwlock_read_unlock( & ip_globals.lock );
		return ENOENT;
	}
	* device_id = route->netif->device_id;
	src = ip_netif_address( route->netif );
	fibril_rwlock_read_unlock( & ip_globals.lock );
	* headerlen = sizeof( * header_in );
	header_in = ( ipv4_pseudo_header_ref ) malloc( * headerlen );
	if( ! header_in ) return ENOMEM;
	bzero( header_in, * headerlen );
	header_in->destination_address = dest->s_addr;
	header_in->source_address = src->s_addr;
	header_in->protocol = protocol;
	header_in->data_length = 0;
	* header = ( ip_pseudo_header_ref ) header_in;
	return EOK;
}

/** @}
 */
