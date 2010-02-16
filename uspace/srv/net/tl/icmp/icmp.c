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
 *  @{
 */

/** @file
 *  ICMP module implementation.
 *  @see icmp.h
 */

#include <async.h>
#include <atomic.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <stdint.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include <sys/time.h>
#include <sys/types.h>

#include "../../err.h"
#include "../../messages.h"
#include "../../modules.h"

#include "../../structures/packet/packet_client.h"

#include "../../include/byteorder.h"
#include "../../include/checksum.h"
#include "../../include/icmp_api.h"
#include "../../include/icmp_client.h"
#include "../../include/icmp_codes.h"
#include "../../include/icmp_common.h"
#include "../../include/icmp_interface.h"
#include "../../include/il_interface.h"
#include "../../include/inet.h"
#include "../../include/ip_client.h"
#include "../../include/ip_interface.h"
#include "../../include/ip_protocols.h"
#include "../../include/net_interface.h"
#include "../../include/socket_codes.h"
#include "../../include/socket_errno.h"

#include "../../tl/tl_messages.h"

#include "icmp.h"
#include "icmp_header.h"
#include "icmp_messages.h"
#include "icmp_module.h"

/** Default ICMP error reporting.
 */
#define NET_DEFAULT_ICMP_ERROR_REPORTING	true

/** Default ICMP echo replying.
 */
#define NET_DEFAULT_ICMP_ECHO_REPLYING		true

/** Original datagram length in bytes transfered to the error notification message.
 */
#define ICMP_KEEP_LENGTH	8

/** Free identifier numbers pool start.
 */
#define ICMP_FREE_IDS_START	1

/** Free identifier numbers pool end.
 */
#define ICMP_FREE_IDS_END	MAX_UINT16

/** Computes the ICMP datagram checksum.
 *  @param[in,out] header The ICMP datagram header.
 *  @param[in] length The total datagram length.
 *  @returns The computed checksum.
 */
#define ICMP_CHECKSUM( header, length )		htons( ip_checksum(( uint8_t * ) ( header ), ( length )))

/** An echo request datagrams pattern.
 */
#define ICMP_ECHO_TEXT					"Hello from HelenOS."

/** Computes an ICMP reply data key.
 *  @param[in] id The message identifier.
 *  @param[in] sequence The message sequence number.
 *  @returns The computed ICMP reply data key.
 */
#define ICMP_GET_REPLY_KEY( id, sequence )	((( id ) << 16 ) | ( sequence & 0xFFFF ))

/** Type definition of the ICMP reply timeout.
 *  @see icmp_reply_timeout
 */
typedef struct icmp_reply_timeout	icmp_reply_timeout_t;

/** Type definition of the ICMP reply timeout pointer.
 *  @see icmp_reply_timeout
 */
typedef icmp_reply_timeout_t *	icmp_reply_timeout_ref;

/** ICMP reply timeout data.
 *  Used as a timeouting fibril argument.
 *  @see icmp_timeout_for_reply()
 */
struct icmp_reply_timeout{
	/** Reply data key.
	 */
	int			reply_key;
	/** Timeout in microseconds.
	 */
	suseconds_t	timeout;
};

/** Processes the received ICMP packet.
 *  Is used as an entry point from the underlying IP module.
 *  Releases the packet on error.
 *  @param device_id The device identifier. Ignored parameter.
 *  @param[in,out] packet The received packet.
 *  @param receiver The target service. Ignored parameter.
 *  @param[in] error The packet error reporting service. Prefixes the received packet.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the icmp_process_packet() function.
 */
int	icmp_received_msg( device_id_t device_id, packet_t packet, services_t receiver, services_t error );

/** Processes the received ICMP packet.
 *  Notifies the destination socket application.
 *  @param[in,out] packet The received packet.
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
int	icmp_process_packet( packet_t packet, services_t error );

/** Processes the client messages.
 *  Remembers the assigned identifier and sequence numbers.
 *  Runs until the client module disconnects.
 *  @param[in] callid The message identifier.
 *  @param[in] call The message parameters.
 *  @returns EOK.
 *  @see icmp_interface.h
 *  @see icmp_api.h
 */
int icmp_process_client_messages( ipc_callid_t callid, ipc_call_t call );

/** Processes the generic client messages.
 *  @param[in] call The message parameters.
 *  @returns EOK on success.
 *  @returns ENOTSUP if the message is not known.
 *  @returns Other error codes as defined for the packet_translate() function.
 *  @returns Other error codes as defined for the icmp_destination_unreachable_msg() function.
 *  @returns Other error codes as defined for the icmp_source_quench_msg() function.
 *  @returns Other error codes as defined for the icmp_time_exceeded_msg() function.
 *  @returns Other error codes as defined for the icmp_parameter_problem_msg() function.
 *  @see icmp_interface.h
 */
int	icmp_process_message( ipc_call_t * call );

/** Releases the packet and returns the result.
 *  @param[in] packet The packet queue to be released.
 *  @param[in] result The result to be returned.
 *  @returns The result parameter.
 */
int	icmp_release_and_return( packet_t packet, int result );

/** Requests an echo message.
 *  Sends a packet with specified parameters to the target host and waits for the reply upto the given timeout.
 *  Blocks the caller until the reply or the timeout occurres.
 *  @param[in] id The message identifier.
 *  @param[in] sequence The message sequence parameter.
 *  @param[in] size The message data length in bytes.
 *  @param[in] timeout The timeout in miliseconds.
 *  @param[in] ttl The time to live.
 *  @param[in] tos The type of service.
 *  @param[in] dont_fragment The value indicating whether the datagram must not be fragmented. Is used as a MTU discovery.
 *  @param[in] addr The target host address.
 *  @param[in] addrlen The torget host address length.
 *  @returns ICMP_ECHO on success.
 *  @returns ETIMEOUT if the reply has not arrived before the timeout.
 *  @returns ICMP type of the received error notification. 
 *  @returns EINVAL if the addrlen parameter is less or equal to zero (<=0).
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns EPARTY if there was an internal error.
 */
int	icmp_echo( icmp_param_t id, icmp_param_t sequence, size_t size, mseconds_t timeout, ip_ttl_t ttl, ip_tos_t tos, int dont_fragment, const struct sockaddr * addr, socklen_t addrlen );

/** Prepares the ICMP error packet.
 *  Truncates the original packet if longer than ICMP_KEEP_LENGTH bytes.
 *  Prefixes and returns the ICMP header.
 *  @param[in,out] packet The original packet.
 *  @returns The prefixed ICMP header.
 *  @returns NULL on errors.
 */
icmp_header_ref	icmp_prepare_packet( packet_t packet );

/** Sends the ICMP message.
 *  Sets the message type and code and computes the checksum.
 *  Error messages are sent only if allowed in the configuration.
 *  Releases the packet on errors.
 *  @param[in] type The message type.
 *  @param[in] code The message code.
 *  @param[in] packet The message packet to be sent.
 *  @param[in] header The ICMP header.
 *  @param[in] error The error service to be announced. Should be SERVICE_ICMP or zero (0).
 *  @param[in] ttl The time to live.
 *  @param[in] tos The type of service.
 *  @param[in] dont_fragment The value indicating whether the datagram must not be fragmented. Is used as a MTU discovery.
 *  @returns EOK on success.
 *  @returns EPERM if the error message is not allowed.
 */
int	icmp_send_packet( icmp_type_t type, icmp_code_t code, packet_t packet, icmp_header_ref header, services_t error, ip_ttl_t ttl, ip_tos_t tos, int dont_fragment );

/** Tries to set the pending reply result as the received message type.
 *  If the reply data are still present, the reply timeouted and the parent fibril is awaken.
 *  The global lock is not released in this case to be reused by the parent fibril.
 *  Releases the packet.
 *  @param[in] packet The received reply message.
 *  @param[in] header The ICMP message header.
 *  @param[in] type The received reply message type.
 *  @param[in] code The received reply message code.
 *  @returns EOK.
 */
int	icmp_process_echo_reply( packet_t packet, icmp_header_ref header, icmp_type_t type, icmp_code_t code );

/** Tries to set the pending reply result as timeouted.
 *  Sleeps the timeout period of time and then tries to obtain and set the pending reply result as timeouted and signals the reply result.
 *  If the reply data are still present, the reply timeouted and the parent fibril is awaken.
 *  The global lock is not released in this case to be reused by the parent fibril.
 *  Should run in a searate fibril.
 *  @param[in] data The icmp_reply_timeout structure.
 *  @returns EOK on success.
 *  @returns EINVAL if the data parameter is NULL.
 */
int	icmp_timeout_for_reply( void * data );

/** Assigns a new identifier for the connection.
 *  Fills the echo data parameter with the assigned values.
 *  @param[in,out] echo_data The echo data to be bound.
 *  @returns Index of the inserted echo data.
 *  @returns EBADMEM if the echo_data parameter is NULL.
 *  @returns ENOTCONN if no free identifier have been found.
 */
int	icmp_bind_free_id( icmp_echo_ref echo_data );

/** ICMP global data.
 */
icmp_globals_t	icmp_globals;

INT_MAP_IMPLEMENT( icmp_replies, icmp_reply_t );

INT_MAP_IMPLEMENT( icmp_echo_data, icmp_echo_t );

int icmp_echo_msg( int icmp_phone, size_t size, mseconds_t timeout, ip_ttl_t ttl, ip_tos_t tos, int dont_fragment, const struct sockaddr * addr, socklen_t addrlen ){
	icmp_echo_ref	echo_data;
	int				res;

	fibril_rwlock_write_lock( & icmp_globals.lock );
	// use the phone as the echo data index
	echo_data = icmp_echo_data_find( & icmp_globals.echo_data, icmp_phone );
	if( ! echo_data ){
		res = ENOENT;
	}else{
		res = icmp_echo( echo_data->identifier, echo_data->sequence_number, size, timeout, ttl, tos, dont_fragment, addr, addrlen );
		if( echo_data->sequence_number < MAX_UINT16 ){
			++ echo_data->sequence_number;
		}else{
			echo_data->sequence_number = 0;
		}
	}
	fibril_rwlock_write_unlock( & icmp_globals.lock );
	return res;
}

int icmp_timeout_for_reply( void * data ){
	icmp_reply_ref			reply;
	icmp_reply_timeout_ref	timeout = data;

	if( ! timeout ){
		return EINVAL;
	}
	// sleep the given timeout
	async_usleep( timeout->timeout );
	// lock the globals
	fibril_rwlock_write_lock( & icmp_globals.lock );
	// find the pending reply
	reply = icmp_replies_find( & icmp_globals.replies, timeout->reply_key );
	if( reply ){
		// set the timeout result
		reply->result = ETIMEOUT;
		// notify the main fibril
		fibril_condvar_signal( & reply->condvar );
	}else{
		// unlock only if no reply
		fibril_rwlock_write_unlock( & icmp_globals.lock );
	}
	// release the timeout structure
	free( timeout );
	return EOK;
}

int icmp_echo( icmp_param_t id, icmp_param_t sequence, size_t size, mseconds_t timeout, ip_ttl_t ttl, ip_tos_t tos, int dont_fragment, const struct sockaddr * addr, socklen_t addrlen ){
	ERROR_DECLARE;

	icmp_header_ref	header;
	packet_t		packet;
	size_t			length;
	uint8_t *		data;
	icmp_reply_ref			reply;
	icmp_reply_timeout_ref	reply_timeout;
	int				result;
	int				index;
	fid_t			fibril;

	if( addrlen <= 0 ){
		return EINVAL;
	}
	length = ( size_t ) addrlen;
	// TODO do not ask all the time
	ERROR_PROPAGATE( ip_packet_size_req( icmp_globals.ip_phone, -1, & icmp_globals.addr_len, & icmp_globals.prefix, & icmp_globals.content, & icmp_globals.suffix ));
	packet = packet_get_4( icmp_globals.net_phone, size, icmp_globals.addr_len, ICMP_HEADER_SIZE + icmp_globals.prefix, icmp_globals.suffix );
	if( ! packet ) return ENOMEM;

	// prepare the requesting packet
	// set the destination address
	if( ERROR_OCCURRED( packet_set_addr( packet, NULL, ( const uint8_t * ) addr, length ))){
		return icmp_release_and_return( packet, ERROR_CODE );
	}
	// allocate space in the packet
	data = ( uint8_t * ) packet_suffix( packet, size );
	if( ! data ){
		return icmp_release_and_return( packet, ENOMEM );
	}
	// fill the data
	length = 0;
	while( size > length + sizeof( ICMP_ECHO_TEXT )){
		memcpy( data + length, ICMP_ECHO_TEXT, sizeof( ICMP_ECHO_TEXT ));
		length += sizeof( ICMP_ECHO_TEXT );
	}
	memcpy( data + length, ICMP_ECHO_TEXT, size - length );
	// prefix the header
	header = PACKET_PREFIX( packet, icmp_header_t );
	if( ! header ){
		return icmp_release_and_return( packet, ENOMEM );
	}
	bzero( header, sizeof( * header ));
	header->un.echo.identifier = id;
	header->un.echo.sequence_number = sequence;

	// prepare the reply and the reply timeout structures
	reply_timeout = malloc( sizeof( * reply_timeout ));
	if( ! reply_timeout ){
		return icmp_release_and_return( packet, ENOMEM );
	}
	reply = malloc( sizeof( * reply ));
	if( ! reply ){
		free( reply_timeout );
		return icmp_release_and_return( packet, ENOMEM );
	}
	// prepare the timeouting thread
	fibril = fibril_create( icmp_timeout_for_reply, reply_timeout );
	if( ! fibril ){
		free( reply );
		free( reply_timeout );
		return icmp_release_and_return( packet, EPARTY );
	}
	reply_timeout->reply_key = ICMP_GET_REPLY_KEY( header->un.echo.identifier, header->un.echo.sequence_number );
	// timeout in microseconds
	reply_timeout->timeout = timeout * 1000;
	fibril_mutex_initialize( & reply->mutex );
	fibril_mutex_lock( & reply->mutex );
	fibril_condvar_initialize( & reply->condvar );
	// start the timeouting fibril
	fibril_add_ready( fibril );
	index = icmp_replies_add( & icmp_globals.replies, reply_timeout->reply_key, reply );
	if( index < 0 ){
		free( reply );
		return icmp_release_and_return( packet, index );
	}

	// unlock the globals and wait for a reply
	fibril_rwlock_write_unlock( & icmp_globals.lock );

	// send the request
	icmp_send_packet( ICMP_ECHO, 0, packet, header, 0, ttl, tos, dont_fragment );

	// wait for a reply
	fibril_condvar_wait( & reply->condvar, & reply->mutex );

	// read the result
	result = reply->result;

	// destroy the reply structure
	fibril_mutex_unlock( & reply->mutex );
	icmp_replies_exclude_index( & icmp_globals.replies, index );
	return result;
}

int icmp_destination_unreachable_msg( int icmp_phone, icmp_code_t code, icmp_param_t mtu, packet_t packet ){
	icmp_header_ref	header;

	header = icmp_prepare_packet( packet );
	if( ! header ){
		return icmp_release_and_return( packet, ENOMEM );
	}
	if( mtu ){
		header->un.frag.mtu = mtu;
	}
	return icmp_send_packet( ICMP_DEST_UNREACH, code, packet, header, SERVICE_ICMP, 0, 0, 0 );
}

int icmp_source_quench_msg( int icmp_phone, packet_t packet ){
	icmp_header_ref	header;

	header = icmp_prepare_packet( packet );
	if( ! header ){
		return icmp_release_and_return( packet, ENOMEM );
	}
	return icmp_send_packet( ICMP_SOURCE_QUENCH, 0, packet, header, SERVICE_ICMP, 0, 0, 0 );
}

int icmp_time_exceeded_msg( int icmp_phone, icmp_code_t code, packet_t packet ){
	icmp_header_ref	header;

	header = icmp_prepare_packet( packet );
	if( ! header ){
		return icmp_release_and_return( packet, ENOMEM );
	}
	return icmp_send_packet( ICMP_TIME_EXCEEDED, code, packet, header, SERVICE_ICMP, 0, 0, 0 );
}

int icmp_parameter_problem_msg( int icmp_phone, icmp_code_t code, icmp_param_t pointer, packet_t packet ){
	icmp_header_ref	header;

	header = icmp_prepare_packet( packet );
	if( ! header ){
		return icmp_release_and_return( packet, ENOMEM );
	}
	header->un.param.pointer = pointer;
	return icmp_send_packet( ICMP_PARAMETERPROB, code, packet, header, SERVICE_ICMP, 0, 0, 0 );
}

icmp_header_ref icmp_prepare_packet( packet_t packet ){
	icmp_header_ref	header;
	size_t			header_length;
	size_t			total_length;

	total_length = packet_get_data_length( packet );
	if( total_length <= 0 ) return NULL;
	header_length = ip_client_header_length( packet );
	if( header_length <= 0 ) return NULL;
	// truncate if longer than 64 bits (without the IP header)
	if(( total_length > header_length + ICMP_KEEP_LENGTH )
	&& ( packet_trim( packet, 0, total_length - header_length - ICMP_KEEP_LENGTH ) != EOK )){
		return NULL;
	}
	header = PACKET_PREFIX( packet, icmp_header_t );
	if( ! header ) return NULL;
	bzero( header, sizeof( * header ));
	return header;
}

int icmp_send_packet( icmp_type_t type, icmp_code_t code, packet_t packet, icmp_header_ref header, services_t error, ip_ttl_t ttl, ip_tos_t tos, int dont_fragment ){
	ERROR_DECLARE;

	// do not send an error if disabled
	if( error && ( ! icmp_globals.error_reporting )){
		return icmp_release_and_return( packet, EPERM );
	}
	header->type = type;
	header->code = code;
	header->checksum = 0;
	header->checksum = ICMP_CHECKSUM( header, packet_get_data_length( packet ));
	if( ERROR_OCCURRED( ip_client_prepare_packet( packet, IPPROTO_ICMP, ttl, tos, dont_fragment, 0 ))){
		return icmp_release_and_return( packet, ERROR_CODE );
	}
	return ip_send_msg( icmp_globals.ip_phone, -1, packet, SERVICE_ICMP, error );
}

int icmp_connect_module( services_t service, suseconds_t timeout ){
	icmp_echo_ref	echo_data;
	icmp_param_t	id;
	int				index;

	echo_data = ( icmp_echo_ref ) malloc( sizeof( * echo_data ));
	if( ! echo_data ) return ENOMEM;
	// assign a new identifier
	fibril_rwlock_write_lock( & icmp_globals.lock );
	index = icmp_bind_free_id( echo_data );
	if( index < 0 ){
		free( echo_data );
		fibril_rwlock_write_unlock( & icmp_globals.lock );
		return index;
	}else{
		id = echo_data->identifier;
		fibril_rwlock_write_unlock( & icmp_globals.lock );
		// return the echo data identifier as the ICMP phone
		return id;
	}
}

int icmp_initialize( async_client_conn_t client_connection ){
	ERROR_DECLARE;

	measured_string_t	names[] = {{ "ICMP_ERROR_REPORTING", 20 }, { "ICMP_ECHO_REPLYING", 18 }};
	measured_string_ref	configuration;
	size_t				count = sizeof( names ) / sizeof( measured_string_t );
	char *				data;

	fibril_rwlock_initialize( & icmp_globals.lock );
	fibril_rwlock_write_lock( & icmp_globals.lock );
	icmp_replies_initialize( & icmp_globals.replies );
	icmp_echo_data_initialize( & icmp_globals.echo_data );
	icmp_globals.ip_phone = ip_bind_service( SERVICE_IP, IPPROTO_ICMP, SERVICE_ICMP, client_connection, icmp_received_msg );
	if( icmp_globals.ip_phone < 0 ){
		return icmp_globals.ip_phone;
	}
	ERROR_PROPAGATE( ip_packet_size_req( icmp_globals.ip_phone, -1, & icmp_globals.addr_len, & icmp_globals.prefix, & icmp_globals.content, & icmp_globals.suffix ));
	icmp_globals.prefix += ICMP_HEADER_SIZE;
	icmp_globals.content -= ICMP_HEADER_SIZE;
	// get configuration
	icmp_globals.error_reporting = NET_DEFAULT_ICMP_ERROR_REPORTING;
	icmp_globals.echo_replying = NET_DEFAULT_ICMP_ECHO_REPLYING;
	configuration = & names[ 0 ];
	ERROR_PROPAGATE( net_get_conf_req( icmp_globals.net_phone, & configuration, count, & data ));
	if( configuration ){
		if( configuration[ 0 ].value ){
			icmp_globals.error_reporting = ( configuration[ 0 ].value[ 0 ] == 'y' );
		}
		if( configuration[ 1 ].value ){
			icmp_globals.echo_replying = ( configuration[ 1 ].value[ 0 ] == 'y' );
		}
		net_free_settings( configuration, data );
	}
	fibril_rwlock_write_unlock( & icmp_globals.lock );
	return EOK;
}

int	icmp_received_msg( device_id_t device_id, packet_t packet, services_t receiver, services_t error ){
	ERROR_DECLARE;

	if( ERROR_OCCURRED( icmp_process_packet( packet, error ))){
		return icmp_release_and_return( packet, ERROR_CODE );
	}

	return EOK;
}

int icmp_process_packet( packet_t packet, services_t error ){
	ERROR_DECLARE;

	size_t			length;
	uint8_t *		src;
	int				addrlen;
	int				result;
	void *			data;
	icmp_header_ref	header;
	icmp_type_t		type;
	icmp_code_t		code;

	if( error ){
		switch( error ){
			case SERVICE_ICMP:
				// process error
				result = icmp_client_process_packet( packet, & type, & code, NULL, NULL );
				if( result < 0 ) return result;
				length = ( size_t ) result;
				// remove the error header
				ERROR_PROPAGATE( packet_trim( packet, length, 0 ));
				break;
			default:
				return ENOTSUP;
		}
	}
	// get rid of the ip header
	length = ip_client_header_length( packet );
	ERROR_PROPAGATE( packet_trim( packet, length, 0 ));

	length = packet_get_data_length( packet );
	if( length <= 0 ) return EINVAL;
	if( length < ICMP_HEADER_SIZE) return EINVAL;
	data = packet_get_data( packet );
	if( ! data ) return EINVAL;
	// get icmp header
	header = ( icmp_header_ref ) data;
	// checksum
	if( header->checksum ){
		while( ICMP_CHECKSUM( header, length ) != IP_CHECKSUM_ZERO ){
			// set the original message type on error notification
			// type swap observed in Qemu
			if( error ){
				switch( header->type ){
					case ICMP_ECHOREPLY:
						header->type = ICMP_ECHO;
						continue;
				}
			}
			return EINVAL;
		}
	}
	switch( header->type ){
		case ICMP_ECHOREPLY:
			if( error ){
				return icmp_process_echo_reply( packet, header, type, code );
			}else{
				return icmp_process_echo_reply( packet, header, ICMP_ECHO, 0 );
			}
		case ICMP_ECHO:
			if( error ){
				return icmp_process_echo_reply( packet, header, type, code );
			// do not send a reply if disabled
			}else if( icmp_globals.echo_replying ){
				addrlen = packet_get_addr( packet, & src, NULL );
				if(( addrlen > 0 )
				// set both addresses to the source one (avoids the source address deletion before setting the destination one)
				&& ( packet_set_addr( packet, src, src, ( size_t ) addrlen ) == EOK )){
					// send the reply
					icmp_send_packet( ICMP_ECHOREPLY, 0, packet, header, 0, 0, 0, 0 );
					return EOK;
				}else{
					return EINVAL;
				}
			}else{
				return EPERM;
			}
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
			ip_received_error_msg( icmp_globals.ip_phone, -1, packet, SERVICE_IP, SERVICE_ICMP );
			return EOK;
		default:
			return ENOTSUP;
	}
}

int icmp_process_echo_reply( packet_t packet, icmp_header_ref header, icmp_type_t type, icmp_code_t code ){
	int				reply_key;
	icmp_reply_ref	reply;

	// compute the reply key
	reply_key = ICMP_GET_REPLY_KEY( header->un.echo.identifier, header->un.echo.sequence_number );
	pq_release( icmp_globals.net_phone, packet_get_id( packet ));
	// lock the globals
	fibril_rwlock_write_lock( & icmp_globals.lock );
	// find the pending reply
	reply = icmp_replies_find( & icmp_globals.replies, reply_key );
	if( reply ){
		// set the result
		reply->result = type;
		// notify the main fibril
		fibril_condvar_signal( & reply->condvar );
	}else{
		// unlock only if no reply
		fibril_rwlock_write_unlock( & icmp_globals.lock );
	}
	return EOK;
}

int icmp_message( ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count ){
	ERROR_DECLARE;

	packet_t			packet;

	* answer_count = 0;
	switch( IPC_GET_METHOD( * call )){
		case NET_TL_RECEIVED:
			if( ! ERROR_OCCURRED( packet_translate( icmp_globals.net_phone, & packet, IPC_GET_PACKET( call )))){
				ERROR_CODE = icmp_received_msg( IPC_GET_DEVICE( call ), packet, SERVICE_ICMP, IPC_GET_ERROR( call ));
			}
			return ERROR_CODE;
		case NET_ICMP_INIT:
			return icmp_process_client_messages( callid, * call );
		default:
			return icmp_process_message( call );
	}
	return ENOTSUP;
}

int icmp_process_client_messages( ipc_callid_t callid, ipc_call_t call ){
	ERROR_DECLARE;

	bool					keep_on_going = true;
//	fibril_rwlock_t			lock;
	ipc_call_t				answer;
	int						answer_count;
	size_t					length;
	struct sockaddr *		addr;
	ipc_callid_t			data_callid;
	icmp_echo_ref			echo_data;

	/*
	 * Accept the connection
	 *  - Answer the first NET_ICMP_INIT call.
	 */
	ipc_answer_0( callid, EOK );

//	fibril_rwlock_initialize( & lock );

	echo_data = ( icmp_echo_ref ) malloc( sizeof( * echo_data ));
	if( ! echo_data ) return ENOMEM;
	// assign a new identifier
	fibril_rwlock_write_lock( & icmp_globals.lock );
	ERROR_CODE = icmp_bind_free_id( echo_data );
	fibril_rwlock_write_unlock( & icmp_globals.lock );
	if( ERROR_CODE < 0 ){
		free( echo_data );
		return ERROR_CODE;
	}

	while( keep_on_going ){
		refresh_answer( & answer, & answer_count );

		callid = async_get_call( & call );

		switch( IPC_GET_METHOD( call )){
			case IPC_M_PHONE_HUNGUP:
				keep_on_going = false;
				ERROR_CODE = EOK;
				break;
			case NET_ICMP_ECHO:
//				fibril_rwlock_write_lock( & lock );
				if( ! async_data_write_receive( & data_callid, & length )){
					ERROR_CODE = EINVAL;
				}else{
					addr = malloc( length );
					if( ! addr ){
						ERROR_CODE = ENOMEM;
					}else{
						if( ! ERROR_OCCURRED( async_data_write_finalize( data_callid, addr, length ))){
							fibril_rwlock_write_lock( & icmp_globals.lock );
							ERROR_CODE = icmp_echo( echo_data->identifier, echo_data->sequence_number, ICMP_GET_SIZE( call ), ICMP_GET_TIMEOUT( call ), ICMP_GET_TTL( call ), ICMP_GET_TOS( call ), ICMP_GET_DONT_FRAGMENT( call ), addr, ( socklen_t ) length );
							fibril_rwlock_write_unlock( & icmp_globals.lock );
							free( addr );
							if( echo_data->sequence_number < MAX_UINT16 ){
								++ echo_data->sequence_number;
							}else{
								echo_data->sequence_number = 0;
							}
						}
					}
				}
//				fibril_rwlock_write_unlock( & lock );
				break;
			default:
				ERROR_CODE = icmp_process_message( & call );
		}

		answer_call( callid, ERROR_CODE, & answer, answer_count );
	}

	// release the identifier
	fibril_rwlock_write_lock( & icmp_globals.lock );
	icmp_echo_data_exclude( & icmp_globals.echo_data, echo_data->identifier );
	fibril_rwlock_write_unlock( & icmp_globals.lock );
	return EOK;
}

int icmp_process_message( ipc_call_t * call ){
	ERROR_DECLARE;

	packet_t	packet;

	switch( IPC_GET_METHOD( * call )){
		case NET_ICMP_DEST_UNREACH:
			if( ! ERROR_OCCURRED( packet_translate( icmp_globals.net_phone, & packet, IPC_GET_PACKET( call )))){
				ERROR_CODE = icmp_destination_unreachable_msg( 0, ICMP_GET_CODE( call ), ICMP_GET_MTU( call ), packet );
			}
			return ERROR_CODE;
		case NET_ICMP_SOURCE_QUENCH:
			if( ! ERROR_OCCURRED( packet_translate( icmp_globals.net_phone, & packet, IPC_GET_PACKET( call )))){
				ERROR_CODE = icmp_source_quench_msg( 0, packet );
			}
			return ERROR_CODE;
		case NET_ICMP_TIME_EXCEEDED:
			if( ! ERROR_OCCURRED( packet_translate( icmp_globals.net_phone, & packet, IPC_GET_PACKET( call )))){
				ERROR_CODE = icmp_time_exceeded_msg( 0, ICMP_GET_CODE( call ), packet );
			}
			return ERROR_CODE;
		case NET_ICMP_PARAMETERPROB:
			if( ! ERROR_OCCURRED( packet_translate( icmp_globals.net_phone, & packet, IPC_GET_PACKET( call )))){
				ERROR_CODE = icmp_parameter_problem_msg( 0, ICMP_GET_CODE( call ), ICMP_GET_POINTER( call ), packet );
			}
			return ERROR_CODE;
		default:
			return ENOTSUP;
	}
}

int	icmp_release_and_return( packet_t packet, int result ){
	pq_release( icmp_globals.net_phone, packet_get_id( packet ));
	return result;
}

int icmp_bind_free_id( icmp_echo_ref echo_data ){
	icmp_param_t	index;

	if( ! echo_data ) return EBADMEM;
	// from the last used one
	index = icmp_globals.last_used_id;
	do{
		++ index;
		// til the range end
		if( index >= ICMP_FREE_IDS_END ){
			// start from the range beginning
			index = ICMP_FREE_IDS_START - 1;
			do{
				++ index;
				// til the last used one
				if( index >= icmp_globals.last_used_id ){
					// none found
					return ENOTCONN;
				}
			}while( icmp_echo_data_find( & icmp_globals.echo_data, index ) != NULL );
			// found, break immediately
			break;
		}
	}while( icmp_echo_data_find( & icmp_globals.echo_data, index ) != NULL );
	echo_data->identifier = index;
	echo_data->sequence_number = 0;
	return icmp_echo_data_add( & icmp_globals.echo_data, index, echo_data );
}

/** @}
 */
