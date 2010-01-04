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
 *  @{
 */

/** @file
 *  UDP module implementation.
 *  @see udp.h
 */

#include <async.h>
#include <fibril_synch.h>
#include <malloc.h>
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

#include "../../socket/socket_core.h"
#include "../../socket/socket_messages.h"

#include "../tl_common.h"
#include "../tl_messages.h"

#include "udp.h"
#include "udp_header.h"
#include "udp_module.h"

/** Default UDP checksum computing.
 */
#define NET_DEFAULT_UDP_CHECKSUM_COMPUTING	true

/** Default UDP autobind when sending via unbound sockets.
 */
#define NET_DEFAULT_UDP_AUTOBINDING	true

/** Maximum UDP fragment size.
 */
#define MAX_UDP_FRAGMENT_SIZE	65535

/** Free ports pool start.
 */
#define UDP_FREE_PORTS_START	1025

/** Free ports pool end.
 */
#define UDP_FREE_PORTS_END		65535

/** Processes the received UDP packet queue.
 *  Is used as an entry point from the underlying IP module.
 *  Locks the global lock and calls udp_process_packet() function.
 *  @param device_id The device identifier. Ignored parameter.
 *  @param[in,out] packet The received packet queue.
 *  @param receiver The target service. Ignored parameter.
 *  @param[in] error The packet error reporting service. Prefixes the received packet.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the udp_process_packet() function.
 */
int	udp_received_msg( device_id_t device_id, packet_t packet, services_t receiver, services_t error );

/** Processes the received UDP packet queue.
 *  Notifies the destination socket application.
 *  Releases the packet on error or sends an ICMP error notification..
 *  @param[in,out] packet The received packet queue.
 *  @param[in] error The packet error reporting service. Prefixes the received packet.
 *  @returns EOK on success.
 *  @returns EINVAL if the packet is not valid.
 *  @returns EINVAL if the stored packet address is not the an_addr_t.
 *  @returns EINVAL if the packet does not contain any data.
 *  @returns NO_DATA if the packet content is shorter than the user datagram header.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns EADDRNOTAVAIL if the destination socket does not exist.
 *  @returns Other error codes as defined for the ip_client_process_packet() function.
 */
int udp_process_packet( packet_t packet, services_t error );

/** Releases the packet and returns the result.
 *  @param[in] packet The packet queue to be released.
 *  @param[in] result The result to be returned.
 *  @return The result parameter.
 */
int	udp_release_and_return( packet_t packet, int result );

/** @name Socket messages processing functions
 */
/*@{*/

/** Processes the socket client messages.
 *  Runs until the client module disconnects.
 *  @param[in] callid The message identifier.
 *  @param[in] call The message parameters.
 *  @returns EOK on success.
 *  @see socket.h
 */
int udp_process_client_messages( ipc_callid_t callid, ipc_call_t call );

/** Sends data from the socket to the remote address.
 *  Binds the socket to a free port if not already connected/bound.
 *  Handles the NET_SOCKET_SENDTO message.
 *  Supports AF_INET and AF_INET6 address families.
 *  @param[in,out] local_sockets The application local sockets.
 *  @param[in] socket_id Socket identifier.
 *  @param[in] addr The destination address.
 *  @param[in] addrlen The address length.
 *  @param[in] fragments The number of data fragments.
 *  @param[in] data_fragment_size The data fragment size in bytes.
 *  @param[in] flags Various send flags.
 *  @returns EOK on success.
 *  @returns EAFNOTSUPPORT if the address family is not supported.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EINVAL if the address is invalid.
 *  @returns ENOTCONN if the sending socket is not and cannot be bound.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the socket_read_packet_data() function.
 *  @returns Other error codes as defined for the ip_client_prepare_packet() function.
 *  @returns Other error codes as defined for the ip_send_msg() function.
 */
int udp_sendto_message( socket_cores_ref local_sockets, int socket_id, const struct sockaddr * addr, socklen_t addrlen, int fragments, size_t data_fragment_size, int flags );

/** Receives data to the socket.
 *  Handles the NET_SOCKET_RECVFROM message.
 *  Replies the source address as well.
 *  @param[in] local_sockets The application local sockets.
 *  @param[in] socket_id Socket identifier.
 *  @param[in] flags Various receive flags.
 *  @param[out] addrlen The source address length.
 *  @returns The number of bytes received.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns NO_DATA if there are no received packets or data.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns EINVAL if the received address is not an IP address.
 *  @returns Other error codes as defined for the packet_translate() function.
 *  @returns Other error codes as defined for the data_reply() function.
 */
int	udp_recvfrom_message( socket_cores_ref local_sockets, int socket_id, int flags, size_t * addrlen );

/*@}*/

/** UDP global data.
 */
udp_globals_t	udp_globals;

int udp_initialize( async_client_conn_t client_connection ){
	ERROR_DECLARE;

	measured_string_t	names[] = {{ "UDP_CHECKSUM_COMPUTING", 22 }, { "UDP_AUTOBINDING", 15 }};
	measured_string_ref	configuration;
	size_t				count = sizeof( names ) / sizeof( measured_string_t );
	char *				data;

	fibril_rwlock_initialize( & udp_globals.lock );
	fibril_rwlock_write_lock( & udp_globals.lock );
	udp_globals.icmp_phone = icmp_connect_module( SERVICE_ICMP, ICMP_CONNECT_TIMEOUT );
	udp_globals.ip_phone = ip_bind_service( SERVICE_IP, IPPROTO_UDP, SERVICE_UDP, client_connection, udp_received_msg );
	if( udp_globals.ip_phone < 0 ){
		return udp_globals.ip_phone;
	}
	// read default packet dimensions
	ERROR_PROPAGATE( ip_packet_size_req( udp_globals.ip_phone, -1, & udp_globals.packet_dimension.addr_len, & udp_globals.packet_dimension.prefix, & udp_globals.packet_dimension.content, & udp_globals.packet_dimension.suffix ));
	ERROR_PROPAGATE( socket_ports_initialize( & udp_globals.sockets ));
	if( ERROR_OCCURRED( packet_dimensions_initialize( & udp_globals.dimensions ))){
		socket_ports_destroy( & udp_globals.sockets );
		return ERROR_CODE;
	}
	udp_globals.packet_dimension.prefix += sizeof( udp_header_t );
	udp_globals.packet_dimension.content -= sizeof( udp_header_t );
	udp_globals.last_used_port = UDP_FREE_PORTS_START - 1;
	// get configuration
	udp_globals.checksum_computing = NET_DEFAULT_UDP_CHECKSUM_COMPUTING;
	udp_globals.autobinding = NET_DEFAULT_UDP_AUTOBINDING;
	configuration = & names[ 0 ];
	ERROR_PROPAGATE( net_get_conf_req( udp_globals.net_phone, & configuration, count, & data ));
	if( configuration ){
		if( configuration[ 0 ].value ){
			udp_globals.checksum_computing = ( configuration[ 0 ].value[ 0 ] == 'y' );
		}
		if( configuration[ 1 ].value ){
			udp_globals.autobinding = ( configuration[ 1 ].value[ 0 ] == 'y' );
		}
		net_free_settings( configuration, data );
	}
	fibril_rwlock_write_unlock( & udp_globals.lock );
	return EOK;
}

int	udp_received_msg( device_id_t device_id, packet_t packet, services_t receiver, services_t error ){
	int	result;

	fibril_rwlock_write_lock( & udp_globals.lock );
	result = udp_process_packet( packet, error );
	if( result != EOK ){
		fibril_rwlock_write_unlock( & udp_globals.lock );
	}

	return result;
}

int udp_process_packet( packet_t packet, services_t error ){
	ERROR_DECLARE;

	size_t			length;
	size_t			offset;
	int				result;
	udp_header_ref	header;
	socket_core_ref	socket;
	packet_t		next_packet;
	size_t			total_length;
	uint32_t		checksum;
	int				fragments;
	packet_t		tmp_packet;
	icmp_type_t		type;
	icmp_code_t		code;
	ip_pseudo_header_ref	ip_header;
	struct sockaddr *		src;
	struct sockaddr *		dest;

	if( error ){
		switch( error ){
			case SERVICE_ICMP:
				// ignore error
				// length = icmp_client_header_length( packet );
				// process error
				result = icmp_client_process_packet( packet, & type, & code, NULL, NULL );
				if( result < 0 ){
					return udp_release_and_return( packet, result );
				}
				length = ( size_t ) result;
				if( ERROR_OCCURRED( packet_trim( packet, length, 0 ))){
					return udp_release_and_return( packet, ERROR_CODE );
				}
				break;
			default:
				return udp_release_and_return( packet, ENOTSUP );
		}
	}
	// TODO process received ipopts?
	result = ip_client_process_packet( packet, NULL, NULL, NULL, NULL, NULL );
	if( result < 0 ){
		return udp_release_and_return( packet, result );
	}
	offset = ( size_t ) result;

	length = packet_get_data_length( packet );
	if( length <= 0 ){
		return udp_release_and_return( packet, EINVAL );
	}
	if( length < sizeof( udp_header_t ) + offset ){
		return udp_release_and_return( packet, NO_DATA );
	}

	// trim all but UDP header
	if( ERROR_OCCURRED( packet_trim( packet, offset, 0 ))){
		return udp_release_and_return( packet, ERROR_CODE );
	}

	// get udp header
	header = ( udp_header_ref ) packet_get_data( packet );
	if( ! header ){
		return udp_release_and_return( packet, NO_DATA );
	}
	// find the destination socket
	socket = socket_port_find( & udp_globals.sockets, ntohs( header->destination_port ), SOCKET_MAP_KEY_LISTENING, 0 );
	if( ! socket ){
		if( tl_prepare_icmp_packet( udp_globals.net_phone, udp_globals.icmp_phone, packet, error ) == EOK ){
			icmp_destination_unreachable_msg( udp_globals.icmp_phone, ICMP_PORT_UNREACH, 0, packet );
		}
		return EADDRNOTAVAIL;
	}

	// count the received packet fragments
	next_packet = packet;
	fragments = 0;
	total_length = ntohs( header->total_length );
	// compute header checksum if set
	if( header->checksum && ( ! error )){
		result = packet_get_addr( packet, ( uint8_t ** ) & src, ( uint8_t ** ) & dest );
		if( result <= 0 ){
			return udp_release_and_return( packet, result );
		}
		if( ERROR_OCCURRED( ip_client_get_pseudo_header( IPPROTO_UDP, src, result, dest, result, total_length, & ip_header, & length ))){
			return udp_release_and_return( packet, ERROR_CODE );
		}else{
			checksum = compute_checksum( 0, ip_header, length );
			// the udp header checksum will be added with the first fragment later
			free( ip_header );
		}
	}else{
		header->checksum = 0;
		checksum = 0;
	}

	do{
		++ fragments;
		length = packet_get_data_length( next_packet );
		if( length <= 0 ){
			return udp_release_and_return( packet, NO_DATA );
		}
		if( total_length < length ){
			if( ERROR_OCCURRED( packet_trim( next_packet, 0, length - total_length ))){
				return udp_release_and_return( packet, ERROR_CODE );
			}
			// add partial checksum if set
			if( header->checksum ){
				checksum = compute_checksum( checksum, packet_get_data( packet ), packet_get_data_length( packet ));
			}
			// relese the rest of the packet fragments
			tmp_packet = pq_next( next_packet );
			while( tmp_packet ){
				next_packet = pq_detach( tmp_packet );
				pq_release( udp_globals.net_phone, packet_get_id( tmp_packet ));
				tmp_packet = next_packet;
			}
			// exit the loop
			break;
		}
		total_length -= length;
		// add partial checksum if set
		if( header->checksum ){
			checksum = compute_checksum( checksum, packet_get_data( packet ), packet_get_data_length( packet ));
		}
	}while(( next_packet = pq_next( next_packet )) && ( total_length > 0 ));

	// check checksum
	if( header->checksum ){
		if( flip_checksum( compact_checksum( checksum ))){
			if( tl_prepare_icmp_packet( udp_globals.net_phone, udp_globals.icmp_phone, packet, error ) == EOK ){
				// checksum error ICMP
				icmp_parameter_problem_msg( udp_globals.icmp_phone, ICMP_PARAM_POINTER, (( size_t ) (( void * ) & header->checksum )) - (( size_t ) (( void * ) header )), packet );
			}
			return EINVAL;
		}
	}

	// queue the received packet
	if( ERROR_OCCURRED( dyn_fifo_push( & socket->received, packet_get_id( packet ), SOCKET_MAX_RECEIVED_SIZE ))){
		return udp_release_and_return( packet, ERROR_CODE );
	}

	// notify the destination socket
	fibril_rwlock_write_unlock( & udp_globals.lock );
	async_msg_5( socket->phone, NET_SOCKET_RECEIVED, ( ipcarg_t ) socket->socket_id, 0, 0, 0, ( ipcarg_t ) fragments );
	return EOK;
}

int udp_message( ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count ){
	ERROR_DECLARE;

	packet_t	packet;

	* answer_count = 0;
	switch( IPC_GET_METHOD( * call )){
		case NET_TL_RECEIVED:
			if( ! ERROR_OCCURRED( packet_translate( udp_globals.net_phone, & packet, IPC_GET_PACKET( call )))){
				ERROR_CODE = udp_received_msg( IPC_GET_DEVICE( call ), packet, SERVICE_UDP, IPC_GET_ERROR( call ));
			}
			return ERROR_CODE;
		case IPC_M_CONNECT_TO_ME:
			return udp_process_client_messages( callid, * call );
	}
	return ENOTSUP;
}

int udp_process_client_messages( ipc_callid_t callid, ipc_call_t call ){
	int						res;
	bool					keep_on_going = true;
	socket_cores_t			local_sockets;
	int						app_phone = IPC_GET_PHONE( & call );
	struct sockaddr *		addr;
	size_t					addrlen;
	fibril_rwlock_t			lock;
	ipc_call_t				answer;
	int						answer_count;

	/*
	 * Accept the connection
	 *  - Answer the first IPC_M_CONNECT_TO_ME call.
	 */
	ipc_answer_0( callid, EOK );

	// The client connection is only in one fibril and therefore no additional locks are needed.

	socket_cores_initialize( & local_sockets );
	fibril_rwlock_initialize( & lock );

	while( keep_on_going ){
		// refresh data
		refresh_answer( & answer, & answer_count );

		callid = async_get_call( & call );
//		printf( "message %d\n", IPC_GET_METHOD( * call ));

		switch( IPC_GET_METHOD( call )){
			case IPC_M_PHONE_HUNGUP:
				keep_on_going = false;
				res = EOK;
				break;
			case NET_SOCKET:
				fibril_rwlock_write_lock( & lock );
				res = socket_create( & local_sockets, app_phone, NULL, SOCKET_SET_SOCKET_ID( answer ));
				fibril_rwlock_write_unlock( & lock );
				// TODO max fragment size
				* SOCKET_SET_DATA_FRAGMENT_SIZE( answer ) = MAX_UDP_FRAGMENT_SIZE;
				* SOCKET_SET_HEADER_SIZE( answer ) = sizeof( udp_header_t );
				answer_count = 3;
				break;
			case NET_SOCKET_BIND:
				res = data_receive(( void ** ) & addr, & addrlen );
				if( res == EOK ){
					fibril_rwlock_read_lock( & lock );
					fibril_rwlock_write_lock( & udp_globals.lock );
					res = socket_bind( & local_sockets, & udp_globals.sockets, SOCKET_GET_SOCKET_ID( call ), addr, addrlen, UDP_FREE_PORTS_START, UDP_FREE_PORTS_END, udp_globals.last_used_port );
					fibril_rwlock_write_unlock( & udp_globals.lock );
					fibril_rwlock_read_unlock( & lock );
					free( addr );
				}
				break;
			case NET_SOCKET_SENDTO:
				res = data_receive(( void ** ) & addr, & addrlen );
				if( res == EOK ){
					fibril_rwlock_read_lock( & lock );
					fibril_rwlock_write_lock( & udp_globals.lock );
					res = udp_sendto_message( & local_sockets, SOCKET_GET_SOCKET_ID( call ), addr, addrlen, SOCKET_GET_DATA_FRAGMENTS( call ), SOCKET_GET_DATA_FRAGMENT_SIZE( call ), SOCKET_GET_FLAGS( call ));
					if( res != EOK ){
						fibril_rwlock_write_unlock( & udp_globals.lock );
					}
					fibril_rwlock_read_unlock( & lock );
					free( addr );
				}
				break;
			case NET_SOCKET_RECVFROM:
				fibril_rwlock_read_lock( & lock );
				fibril_rwlock_write_lock( & udp_globals.lock );
				res = udp_recvfrom_message( & local_sockets, SOCKET_GET_SOCKET_ID( call ), SOCKET_GET_FLAGS( call ), & addrlen );
				fibril_rwlock_write_unlock( & udp_globals.lock );
				fibril_rwlock_read_unlock( & lock );
				if( res > 0 ){
					* SOCKET_SET_READ_DATA_LENGTH( answer ) = res;
					* SOCKET_SET_ADDRESS_LENGTH( answer ) = addrlen;
					answer_count = 2;
					res = EOK;
				}
				break;
			case NET_SOCKET_CLOSE:
				fibril_rwlock_write_lock( & lock );
				fibril_rwlock_write_lock( & udp_globals.lock );
				res = socket_destroy( udp_globals.net_phone, SOCKET_GET_SOCKET_ID( call ), & local_sockets, & udp_globals.sockets, NULL );
				fibril_rwlock_write_unlock( & udp_globals.lock );
				fibril_rwlock_write_unlock( & lock );
				break;
			case NET_SOCKET_GETSOCKOPT:
			case NET_SOCKET_SETSOCKOPT:
			default:
				res = ENOTSUP;
				break;
		}

//		printf( "res = %d\n", res );

		answer_call( callid, res, & answer, answer_count );
	}

	// release all local sockets
	socket_cores_release( udp_globals.net_phone, & local_sockets, & udp_globals.sockets, NULL );

	return EOK;
}

int udp_sendto_message( socket_cores_ref local_sockets, int socket_id, const struct sockaddr * addr, socklen_t addrlen, int fragments, size_t data_fragment_size, int flags ){
	ERROR_DECLARE;

	socket_core_ref			socket;
	packet_t				packet;
	packet_t				next_packet;
	udp_header_ref			header;
	int						index;
	size_t					total_length;
	int						result;
	uint16_t				dest_port;
	uint32_t				checksum;
	ip_pseudo_header_ref	ip_header;
	size_t					headerlen;
	device_id_t				device_id;
	packet_dimension_ref	packet_dimension;

	ERROR_PROPAGATE( tl_get_address_port( addr, addrlen, & dest_port ));

	socket = socket_cores_find( local_sockets, socket_id );
	if( ! socket ) return ENOTSOCK;

	if(( socket->port <= 0 ) && udp_globals.autobinding ){
		// bind the socket to a random free port if not bound
//		do{
			// try to find a free port
//			fibril_rwlock_read_unlock( & udp_globals.lock );
//			fibril_rwlock_write_lock( & udp_globals.lock );
			// might be changed in the meantime
//			if( socket->port <= 0 ){
				if( ERROR_OCCURRED( socket_bind_free_port( & udp_globals.sockets, socket, UDP_FREE_PORTS_START, UDP_FREE_PORTS_END, udp_globals.last_used_port ))){
//					fibril_rwlock_write_unlock( & udp_globals.lock );
//					fibril_rwlock_read_lock( & udp_globals.lock );
					return ERROR_CODE;
				}
				// set the next port as the search starting port number
				udp_globals.last_used_port = socket->port;
//			}
//			fibril_rwlock_write_unlock( & udp_globals.lock );
//			fibril_rwlock_read_lock( & udp_globals.lock );
			// might be changed in the meantime
//		}while( socket->port <= 0 );
	}

	if( udp_globals.checksum_computing ){
		if( ERROR_OCCURRED( ip_get_route_req( udp_globals.ip_phone, IPPROTO_UDP, addr, addrlen, & device_id, & ip_header, & headerlen ))){
			return udp_release_and_return( packet, ERROR_CODE );
		}
		// get the device packet dimension
//		ERROR_PROPAGATE( tl_get_ip_packet_dimension( udp_globals.ip_phone, & udp_globals.dimensions, device_id, & packet_dimension ));
	}
//	}else{
		// do not ask all the time
		ERROR_PROPAGATE( ip_packet_size_req( udp_globals.ip_phone, -1, & udp_globals.packet_dimension.addr_len, & udp_globals.packet_dimension.prefix, & udp_globals.packet_dimension.content, & udp_globals.packet_dimension.suffix ));
		packet_dimension = & udp_globals.packet_dimension;
//	}

	// read the first packet fragment
	result = tl_socket_read_packet_data( udp_globals.net_phone, & packet, sizeof( udp_header_t ), packet_dimension, addr, addrlen );
	if( result < 0 ) return result;
	total_length = ( size_t ) result;
	if( udp_globals.checksum_computing ){
		checksum = compute_checksum( 0, packet_get_data( packet ), packet_get_data_length( packet ));
	}else{
		checksum = 0;
	}
	// prefix the udp header
	header = PACKET_PREFIX( packet, udp_header_t );
	if( ! header ){
		return udp_release_and_return( packet, ENOMEM );
	}
	bzero( header, sizeof( * header ));
	// read the rest of the packet fragments
	for( index = 1; index < fragments; ++ index ){
		result = tl_socket_read_packet_data( udp_globals.net_phone, & next_packet, 0, packet_dimension, addr, addrlen );
		if( result < 0 ){
			return udp_release_and_return( packet, result );
		}
		packet = pq_add( packet, next_packet, index, 0 );
		total_length += ( size_t ) result;
		if( udp_globals.checksum_computing ){
			checksum = compute_checksum( checksum, packet_get_data( next_packet ), packet_get_data_length( next_packet ));
		}
	}
	// set the udp header
	header->source_port = htons(( socket->port > 0 ) ? socket->port : 0 );
	header->destination_port = htons( dest_port );
	header->total_length = htons( total_length + sizeof( * header ));
	header->checksum = 0;
	if( udp_globals.checksum_computing ){
//		if( ERROR_OCCURRED( ip_get_route_req( udp_globals.ip_phone, IPPROTO_UDP, addr, addrlen, & device_id, & ip_header, & headerlen ))){
//			return udp_release_and_return( packet, ERROR_CODE );
//		}
		if( ERROR_OCCURRED( ip_client_set_pseudo_header_data_length( ip_header, headerlen, total_length + sizeof( udp_header_t )))){
			free( ip_header );
			return udp_release_and_return( packet, ERROR_CODE );
		}
		checksum = compute_checksum( checksum, ip_header, headerlen );
		checksum = compute_checksum( checksum, ( uint8_t * ) header, sizeof( * header ));
		header->checksum = htons( flip_checksum( compact_checksum( checksum )));
		free( ip_header );
	}else{
		device_id = -1;
	}
	// prepare the first packet fragment
	if( ERROR_OCCURRED( ip_client_prepare_packet( packet, IPPROTO_UDP, 0, 0, 0, 0 ))){
		return udp_release_and_return( packet, ERROR_CODE );
	}
	// send the packet
	fibril_rwlock_write_unlock( & udp_globals.lock );
	ip_send_msg( udp_globals.ip_phone, device_id, packet, SERVICE_UDP, 0 );
	return EOK;
}

int udp_recvfrom_message( socket_cores_ref local_sockets, int socket_id, int flags, size_t * addrlen ){
	ERROR_DECLARE;

	socket_core_ref	socket;
	int				packet_id;
	packet_t		packet;
	udp_header_ref	header;
	struct sockaddr *	addr;
	size_t			length;
	uint8_t *		data;
	int				result;

	// find the socket
	socket = socket_cores_find( local_sockets, socket_id );
	if( ! socket ) return ENOTSOCK;
	// get the next received packet
	packet_id = dyn_fifo_value( & socket->received );
	if( packet_id < 0 ) return NO_DATA;
	ERROR_PROPAGATE( packet_translate( udp_globals.net_phone, & packet, packet_id ));
	// get udp header
	data = packet_get_data( packet );
	if( ! data ){
		pq_release( udp_globals.net_phone, packet_id );
		return NO_DATA;
	}
	header = ( udp_header_ref ) data;

	// set the source address port
	result = packet_get_addr( packet, ( uint8_t ** ) & addr, NULL );
	if( ERROR_OCCURRED( tl_set_address_port( addr, result, ntohs( header->source_port )))){
		pq_release( udp_globals.net_phone, packet_id );
		return ERROR_CODE;
	}
	* addrlen = ( size_t ) result;
	// send the source address
	ERROR_PROPAGATE( data_reply( addr, * addrlen ));

	// trim the header
	ERROR_PROPAGATE( packet_trim( packet, sizeof( udp_header_t ), 0 ));

	// reply the packets
	ERROR_PROPAGATE( socket_reply_packets( packet, & length ));

	// release the packet
	dyn_fifo_pop( & socket->received );
	pq_release( udp_globals.net_phone, packet_get_id( packet ));
	// return the total length
	return ( int ) length;
}

int	udp_release_and_return( packet_t packet, int result ){
	pq_release( udp_globals.net_phone, packet_get_id( packet ));
	return result;
}

/** @}
 */
