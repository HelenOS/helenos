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

/** @addtogroup socket
 *  @{
 */

/** @file
 *  Socket application program interface (API) implementation.
 *  @see socket.h for more information.
 *  This is a part of the network application library.
 */

#include <assert.h>
#include <async.h>
#include <fibril_synch.h>
#include <limits.h>
#include <stdlib.h>

#include <ipc/services.h>

#include "../err.h"
#include "../modules.h"

#include "../include/in.h"
#include "../include/socket.h"
#include "../include/socket_errno.h"

#include "../structures/dynamic_fifo.h"
#include "../structures/int_map.h"

#include "socket_messages.h"

/** Initial received packet queue size.
 */
#define SOCKET_INITIAL_RECEIVED_SIZE	4

/** Maximum received packet queue size.
 */
#define SOCKET_MAX_RECEIVED_SIZE		0

/** Initial waiting sockets queue size.
 */
#define SOCKET_INITIAL_ACCEPTED_SIZE	1

/** Maximum waiting sockets queue size.
 */
#define SOCKET_MAX_ACCEPTED_SIZE		0

/** Default timeout for connections in microseconds.
 */
#define SOCKET_CONNECT_TIMEOUT	( 1 * 1000 * 1000 )

/** Maximum number of random attempts to find a new socket identifier before switching to the sequence.
 */
#define SOCKET_ID_TRIES					100

/** Type definition of the socket specific data.
 *  @see socket
 */
typedef struct socket	socket_t;

/** Type definition of the socket specific data pointer.
 *  @see socket
 */
typedef socket_t *	socket_ref;

/** Socket specific data.
 *  Each socket lock locks only its structure part and any number of them may be locked simultaneously.
 */
struct socket{
	/** Socket identifier.
	 */
	int					socket_id;
	/** Parent module phone.
	 */
	int					phone;
	/** Parent module service.
	 */
	services_t			service;
	/** Underlying protocol header size.
	 *  Sending and receiving optimalization.
	 */
	size_t				header_size;
	/** Packet data fragment size.
	 *  Sending optimalization.
	 */
	size_t				data_fragment_size;
	/** Sending safety lock.
	 *  Locks the header_size and data_fragment_size attributes.
	 */
	fibril_rwlock_t		sending_lock;
	/** Received packets queue.
	 */
	dyn_fifo_t			received;
	/** Received packets safety lock.
	 *  Used for receiving and receive notifications.
	 *  Locks the received attribute.
	 */
	fibril_mutex_t		receive_lock;
	/** Received packets signaling.
	 *  Signaled upon receive notification.
	 */
	fibril_condvar_t	receive_signal;
	/** Waiting sockets queue.
	 */
	dyn_fifo_t			accepted;
	/** Waiting sockets safety lock.
	 *  Used for accepting and accept notifications.
	 *  Locks the accepted attribute.
	 */
	fibril_mutex_t		accept_lock;
	/** Waiting sockets signaling.
	 *  Signaled upon accept notification.
	 */
	fibril_condvar_t	accept_signal;
	/** The number of blocked functions called.
	 *  Used while waiting for the received packets or accepted sockets.
	 */
	int					blocked;
};

/** Sockets map.
 *  Maps socket identifiers to the socket specific data.
 *  @see int_map.h
 */
INT_MAP_DECLARE( sockets, socket_t );

/** Socket client library global data.
 */
static struct socket_client_globals {
	/** TCP module phone.
	 */
	int	tcp_phone;
	/** UDP module phone.
	 */
	int	udp_phone;
//	/** The last socket identifier.
//	 */
//	int last_id;
	/** Active sockets.
	 */
	sockets_ref	sockets;
	/** Safety lock.
	 *  Write lock is used only for adding or removing sockets.
	 *  When locked for writing, no other socket locks need to be locked.
	 *  When locked for reading, any other socket locks may be locked.
	 *  No socket lock may be locked if this lock is unlocked.
	 */
	fibril_rwlock_t	lock;
} socket_globals = {
	.tcp_phone = -1,
	.udp_phone = -1,
//	.last_id = 0,
	.sockets = NULL,
	.lock = {
		.readers = 0,
		.writers = 0,
		.waiters = {
			.prev = & socket_globals.lock.waiters,
			.next = & socket_globals.lock.waiters
		}
	}
};

INT_MAP_IMPLEMENT( sockets, socket_t );

/** Returns the TCP module phone.
 *  Connects to the TCP module if necessary.
 *  @returns The TCP module phone.
 *  @returns Other error codes as defined for the bind_service_timeout() function.
 */
static int	socket_get_tcp_phone( void );

/** Returns the UDP module phone.
 *  Connects to the UDP module if necessary.
 *  @returns The UDP module phone.
 *  @returns Other error codes as defined for the bind_service_timeout() function.
 */
static int	socket_get_udp_phone( void );

/** Returns the active sockets.
 *  @returns The active sockets.
 */
static sockets_ref	socket_get_sockets( void );

/** Tries to find a new free socket identifier.
 *	@returns The new socket identifier.
 *  @returns ELIMIT if there is no socket identifier available.
 */
static int	socket_generate_new_id( void );

/** Default thread for new connections.
 *  @param[in] iid The initial message identifier.
 *  @param[in] icall The initial message call structure.
 */
void	socket_connection( ipc_callid_t iid, ipc_call_t * icall );

/** Sends message to the socket parent module with specified data.
 *  @param[in] socket_id Socket identifier.
 *  @param[in] message The action message.
 *  @param[in] arg2 The second message parameter.
 *  @param[in] data The data to be sent.
 *  @param[in] datalength The data length.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EBADMEM if the data parameter is NULL.
 *  @returns NO_DATA if the datalength parameter is zero (0).
 *  @returns Other error codes as defined for the spcific message.
 */
int	socket_send_data( int socket_id, ipcarg_t message, ipcarg_t arg2, const void * data, size_t datalength );

/** Initializes a new socket specific data.
 *  @param[in,out] socket The socket to be initialized.
 *  @param[in] socket_id The new socket identifier.
 *  @param[in] phone The parent module phone.
 *  @param[in] service The parent module service.
 */
void	socket_initialize( socket_ref socket, int socket_id, int phone, services_t service );

/** Clears and destroys the socket.
 *  @param[in] socket The socket to be destroyed.
 */
void	socket_destroy( socket_ref socket );

/** Receives data via the socket.
 *  @param[in] message The action message.
 *  @param[in] socket_id Socket identifier.
 *  @param[out] data The data buffer to be filled.
 *  @param[in] datalength The data length.
 *  @param[in] flags Various receive flags.
 *  @param[out] fromaddr The source address. May be NULL for connected sockets.
 *  @param[in,out] addrlen The address length. The maximum address length is read. The actual address length is set. Used only if fromaddr is not NULL.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EBADMEM if the data parameter is NULL.
 *  @returns NO_DATA if the datalength or addrlen parameter is zero (0).
 *  @returns Other error codes as defined for the spcific message.
 */
int	recvfrom_core( ipcarg_t message, int socket_id, void * data, size_t datalength, int flags, struct sockaddr * fromaddr, socklen_t * addrlen );

/** Sends data via the socket to the remote address.
 *  Binds the socket to a free port if not already connected/bound.
 *  @param[in] message The action message.
 *  @param[in] socket_id Socket identifier.
 *  @param[in] data The data to be sent.
 *  @param[in] datalength The data length.
 *  @param[in] flags Various send flags.
 *  @param[in] toaddr The destination address. May be NULL for connected sockets.
 *  @param[in] addrlen The address length. Used only if toaddr is not NULL.
 *  @returns EOK on success.
 *  @returns ENOTSOCK if the socket is not found.
 *  @returns EBADMEM if the data or toaddr parameter is NULL.
 *  @returns NO_DATA if the datalength or the addrlen parameter is zero (0).
 *  @returns Other error codes as defined for the NET_SOCKET_SENDTO message.
 */
int	sendto_core( ipcarg_t message, int socket_id, const void * data, size_t datalength, int flags, const struct sockaddr * toaddr, socklen_t addrlen );

static int socket_get_tcp_phone( void ){
	if( socket_globals.tcp_phone < 0 ){
		socket_globals.tcp_phone = bind_service_timeout( SERVICE_TCP, 0, 0, SERVICE_TCP, socket_connection, SOCKET_CONNECT_TIMEOUT );
	}
	return socket_globals.tcp_phone;
}

static int socket_get_udp_phone( void ){
	if( socket_globals.udp_phone < 0 ){
		socket_globals.udp_phone = bind_service_timeout( SERVICE_UDP, 0, 0, SERVICE_UDP, socket_connection, SOCKET_CONNECT_TIMEOUT );
	}
	return socket_globals.udp_phone;
}

static sockets_ref socket_get_sockets( void ){
	if( ! socket_globals.sockets ){
		socket_globals.sockets = ( sockets_ref ) malloc( sizeof( sockets_t ));
		if( ! socket_globals.sockets ) return NULL;
		if( sockets_initialize( socket_globals.sockets ) != EOK ){
			free( socket_globals.sockets );
			socket_globals.sockets = NULL;
		}
		srand( task_get_id());
	}
	return socket_globals.sockets;
}

static int socket_generate_new_id( void ){
	sockets_ref	sockets;
	int			socket_id;
	int			count;

	sockets = socket_get_sockets();
	count = 0;
//	socket_id = socket_globals.last_id;
	do{
		if( count < SOCKET_ID_TRIES ){
			socket_id = rand() % INT_MAX;
			++ count;
		}else if( count == SOCKET_ID_TRIES ){
			socket_id = 1;
			++ count;
		// only this branch for last_id
		}else{
			if( socket_id < INT_MAX ){
				++ socket_id;
/*			}else if( socket_globals.last_id ){
*				socket_globals.last_id = 0;
*				socket_id = 1;
*/			}else{
				return ELIMIT;
			}
		}
	}while( sockets_find( sockets, socket_id ));
//	last_id = socket_id
	return socket_id;
}

void socket_initialize( socket_ref socket, int socket_id, int phone, services_t service ){
	socket->socket_id = socket_id;
	socket->phone = phone;
	socket->service = service;
	dyn_fifo_initialize( & socket->received, SOCKET_INITIAL_RECEIVED_SIZE );
	dyn_fifo_initialize( & socket->accepted, SOCKET_INITIAL_ACCEPTED_SIZE );
	fibril_mutex_initialize( & socket->receive_lock );
	fibril_condvar_initialize( & socket->receive_signal );
	fibril_mutex_initialize( & socket->accept_lock );
	fibril_condvar_initialize( & socket->accept_signal );
	fibril_rwlock_initialize( & socket->sending_lock );
}

void socket_connection( ipc_callid_t iid, ipc_call_t * icall ){
	ERROR_DECLARE;

	ipc_callid_t	callid;
	ipc_call_t		call;
	socket_ref		socket;

	while( true ){

		callid = async_get_call( & call );
		switch( IPC_GET_METHOD( call )){
			case NET_SOCKET_RECEIVED:
			case NET_SOCKET_ACCEPTED:
			case NET_SOCKET_DATA_FRAGMENT_SIZE:
				fibril_rwlock_read_lock( & socket_globals.lock );
				// find the socket
				socket = sockets_find( socket_get_sockets(), SOCKET_GET_SOCKET_ID( call ));
				if( ! socket ){
					ERROR_CODE = ENOTSOCK;
				}else{
					switch( IPC_GET_METHOD( call )){
						case NET_SOCKET_RECEIVED:
							fibril_mutex_lock( & socket->receive_lock );
							// push the number of received packet fragments
							if( ! ERROR_OCCURRED( dyn_fifo_push( & socket->received, SOCKET_GET_DATA_FRAGMENTS( call ), SOCKET_MAX_RECEIVED_SIZE ))){
								// signal the received packet
								fibril_condvar_signal( & socket->receive_signal );
							}
							fibril_mutex_unlock( & socket->receive_lock );
							break;
						case NET_SOCKET_ACCEPTED:
							// push the new socket identifier
							fibril_mutex_lock( & socket->accept_lock );
							if( ! ERROR_OCCURRED( dyn_fifo_push( & socket->accepted, 1, SOCKET_MAX_ACCEPTED_SIZE ))){
								// signal the accepted socket
								fibril_condvar_signal( & socket->accept_signal );
							}
							fibril_mutex_unlock( & socket->accept_lock );
							break;
						default:
							ERROR_CODE = ENOTSUP;
					}
					if(( SOCKET_GET_DATA_FRAGMENT_SIZE( call ) > 0 )
					&& ( SOCKET_GET_DATA_FRAGMENT_SIZE( call ) != socket->data_fragment_size )){
						fibril_rwlock_write_lock( & socket->sending_lock );
						// set the data fragment size
						socket->data_fragment_size = SOCKET_GET_DATA_FRAGMENT_SIZE( call );
						fibril_rwlock_write_unlock( & socket->sending_lock );
					}
				}
				fibril_rwlock_read_unlock( & socket_globals.lock );
				break;
			default:
				ERROR_CODE = ENOTSUP;
		}
		ipc_answer_0( callid, ( ipcarg_t ) ERROR_CODE );
	}
}

int socket( int domain, int type, int protocol ){
	ERROR_DECLARE;

	socket_ref	socket;
	int			phone;
	int			socket_id;
	services_t	service;

	// find the appropriate service
	switch( domain ){
		case PF_INET:
			switch( type ){
				case SOCK_STREAM:
					if( ! protocol ) protocol = IPPROTO_TCP;
					switch( protocol ){
						case IPPROTO_TCP:
							phone = socket_get_tcp_phone();
							service = SERVICE_TCP;
							break;
						default:
							return EPROTONOSUPPORT;
					}
					break;
				case SOCK_DGRAM:
					if( ! protocol ) protocol = IPPROTO_UDP;
					switch( protocol ){
						case IPPROTO_UDP:
							phone = socket_get_udp_phone();
							service = SERVICE_UDP;
							break;
						default:
							return EPROTONOSUPPORT;
					}
					break;
				case SOCK_RAW:
				default:
					return ESOCKTNOSUPPORT;
			}
			break;
		// TODO IPv6
		default:
			return EPFNOSUPPORT;
	}
	if( phone < 0 ) return phone;
	// create a new socket structure
	socket = ( socket_ref ) malloc( sizeof( socket_t ));
	if( ! socket ) return ENOMEM;
	bzero( socket, sizeof( * socket ));
	fibril_rwlock_write_lock( & socket_globals.lock );
	// request a new socket
	socket_id = socket_generate_new_id();
	if( socket_id <= 0 ){
		fibril_rwlock_write_unlock( & socket_globals.lock );
		free( socket );
		return socket_id;
	}
	if( ERROR_OCCURRED(( int ) async_req_3_3( phone, NET_SOCKET, socket_id, 0, service, NULL, ( ipcarg_t * ) & socket->data_fragment_size, ( ipcarg_t * ) & socket->header_size ))){
		fibril_rwlock_write_unlock( & socket_globals.lock );
		free( socket );
		return ERROR_CODE;
	}
	// finish the new socket initialization
	socket_initialize( socket, socket_id, phone, service );
	// store the new socket
	ERROR_CODE = sockets_add( socket_get_sockets(), socket_id, socket );
	fibril_rwlock_write_unlock( & socket_globals.lock );
	if( ERROR_CODE < 0 ){
		dyn_fifo_destroy( & socket->received );
		dyn_fifo_destroy( & socket->accepted );
		free( socket );
		async_msg_3( phone, NET_SOCKET_CLOSE, ( ipcarg_t ) socket_id, 0, service );
		return ERROR_CODE;
	}

	return socket_id;
}

int socket_send_data( int socket_id, ipcarg_t message, ipcarg_t arg2, const void * data, size_t datalength ){
	socket_ref		socket;
	aid_t			message_id;
	ipcarg_t		result;

	if( ! data ) return EBADMEM;
	if( ! datalength ) return NO_DATA;

	fibril_rwlock_read_lock( & socket_globals.lock );
	// find the socket
	socket = sockets_find( socket_get_sockets(), socket_id );
	if( ! socket ){
		fibril_rwlock_read_unlock( & socket_globals.lock );
		return ENOTSOCK;
	}
	// request the message
	message_id = async_send_3( socket->phone, message, ( ipcarg_t ) socket->socket_id, arg2, socket->service, NULL );
	// send the address
	async_data_write_start( socket->phone, data, datalength );
	fibril_rwlock_read_unlock( & socket_globals.lock );
	async_wait_for( message_id, & result );
	return ( int ) result;
}

int bind( int socket_id, const struct sockaddr * my_addr, socklen_t addrlen ){
	if( addrlen <= 0 ) return EINVAL;
	// send the address
	return socket_send_data( socket_id, NET_SOCKET_BIND, 0, my_addr, ( size_t ) addrlen );
}

int listen( int socket_id, int backlog ){
	socket_ref	socket;
	int			result;

	if( backlog <= 0 ) return EINVAL;
	fibril_rwlock_read_lock( & socket_globals.lock );
	// find the socket
	socket = sockets_find( socket_get_sockets(), socket_id );
	if( ! socket ){
		fibril_rwlock_read_unlock( & socket_globals.lock );
		return ENOTSOCK;
	}
	// request listen backlog change
	result = ( int ) async_req_3_0( socket->phone, NET_SOCKET_LISTEN, ( ipcarg_t ) socket->socket_id, ( ipcarg_t ) backlog, socket->service );
	fibril_rwlock_read_unlock( & socket_globals.lock );
	return result;
}

int accept( int socket_id, struct sockaddr * cliaddr, socklen_t * addrlen ){
	socket_ref		socket;
	socket_ref		new_socket;
	aid_t			message_id;
	ipcarg_t		ipc_result;
	int			result;
	ipc_call_t		answer;

	if(( ! cliaddr ) || ( ! addrlen )) return EBADMEM;

	fibril_rwlock_write_lock( & socket_globals.lock );
	// find the socket
	socket = sockets_find( socket_get_sockets(), socket_id );
	if( ! socket ){
		fibril_rwlock_write_unlock( & socket_globals.lock );
		return ENOTSOCK;
	}
	fibril_mutex_lock( & socket->accept_lock );
	// wait for an accepted socket
	++ socket->blocked;
	while( dyn_fifo_value( & socket->accepted ) <= 0 ){
		fibril_rwlock_write_unlock( & socket_globals.lock );
		fibril_condvar_wait( & socket->accept_signal, & socket->accept_lock );
		fibril_rwlock_write_lock( & socket_globals.lock );
	}
	-- socket->blocked;

	// create a new scoket
	new_socket = ( socket_ref ) malloc( sizeof( socket_t ));
	if( ! new_socket ){
		fibril_mutex_unlock( & socket->accept_lock );
		fibril_rwlock_write_unlock( & socket_globals.lock );
		return ENOMEM;
	}
	bzero( new_socket, sizeof( * new_socket ));
	socket_id = socket_generate_new_id();
	if( socket_id <= 0 ){
		fibril_mutex_unlock( & socket->accept_lock );
		fibril_rwlock_write_unlock( & socket_globals.lock );
		free( new_socket );
		return socket_id;
	}
	socket_initialize( new_socket, socket_id, socket->phone, socket->service );
	result = sockets_add( socket_get_sockets(), new_socket->socket_id, new_socket );
	if( result < 0 ){
		fibril_mutex_unlock( & socket->accept_lock );
		fibril_rwlock_write_unlock( & socket_globals.lock );
		free( new_socket );
		return result;
	}

	// request accept
	message_id = async_send_5( socket->phone, NET_SOCKET_ACCEPT, ( ipcarg_t ) socket->socket_id, 0, socket->service, 0, new_socket->socket_id, & answer );
	// read address
	ipc_data_read_start( socket->phone, cliaddr, * addrlen );
	fibril_rwlock_write_unlock( & socket_globals.lock );
	async_wait_for( message_id, & ipc_result );
	result = (int) ipc_result;
	if( result > 0 ){
		if( result != socket_id ){
			result = EINVAL;
		}
		// dequeue the accepted socket if successful
		dyn_fifo_pop( & socket->accepted );
		// set address length
		* addrlen = SOCKET_GET_ADDRESS_LENGTH( answer );
		new_socket->data_fragment_size = SOCKET_GET_DATA_FRAGMENT_SIZE( answer );
	}else if( result == ENOTSOCK ){
		// empty the queue if no accepted sockets
		while( dyn_fifo_pop( & socket->accepted ) > 0 );
	}
	fibril_mutex_unlock( & socket->accept_lock );
	return result;
}

int connect( int socket_id, const struct sockaddr * serv_addr, socklen_t addrlen ){
	if( ! serv_addr ) return EDESTADDRREQ;
	if( ! addrlen ) return EDESTADDRREQ;
	// send the address
	return socket_send_data( socket_id, NET_SOCKET_CONNECT, 0, serv_addr, addrlen );
}

int closesocket( int socket_id ){
	ERROR_DECLARE;

	socket_ref		socket;

	fibril_rwlock_write_lock( & socket_globals.lock );
	socket = sockets_find( socket_get_sockets(), socket_id );
	if( ! socket ){
		fibril_rwlock_write_unlock( & socket_globals.lock );
		return ENOTSOCK;
	}
	if( socket->blocked ){
		fibril_rwlock_write_unlock( & socket_globals.lock );
		return EINPROGRESS;
	}
	// request close
	ERROR_PROPAGATE(( int ) async_req_3_0( socket->phone, NET_SOCKET_CLOSE, ( ipcarg_t ) socket->socket_id, 0, socket->service ));
	// free the socket structure
	socket_destroy( socket );
	fibril_rwlock_write_unlock( & socket_globals.lock );
	return EOK;
}

void socket_destroy( socket_ref socket ){
	int	accepted_id;

	// destroy all accepted sockets
	while(( accepted_id = dyn_fifo_pop( & socket->accepted )) >= 0 ){
		socket_destroy( sockets_find( socket_get_sockets(), accepted_id ));
	}
	dyn_fifo_destroy( & socket->received );
	dyn_fifo_destroy( & socket->accepted );
	sockets_exclude( socket_get_sockets(), socket->socket_id );
}

int send( int socket_id, void * data, size_t datalength, int flags ){
	// without the address
	return sendto_core( NET_SOCKET_SEND, socket_id, data, datalength, flags, NULL, 0 );
}

int sendto( int socket_id, const void * data, size_t datalength, int flags, const struct sockaddr * toaddr, socklen_t addrlen ){
	if( ! toaddr ) return EDESTADDRREQ;
	if( ! addrlen ) return EDESTADDRREQ;
	// with the address
	return sendto_core( NET_SOCKET_SENDTO, socket_id, data, datalength, flags, toaddr, addrlen );
}

int sendto_core( ipcarg_t message, int socket_id, const void * data, size_t datalength, int flags, const struct sockaddr * toaddr, socklen_t addrlen ){
	socket_ref		socket;
	aid_t			message_id;
	ipcarg_t		result;
	size_t			fragments;
	ipc_call_t		answer;

	if( ! data ) return EBADMEM;
	if( ! datalength ) return NO_DATA;
	fibril_rwlock_read_lock( & socket_globals.lock );
	// find socket
	socket = sockets_find( socket_get_sockets(), socket_id );
	if( ! socket ){
		fibril_rwlock_read_unlock( & socket_globals.lock );
		return ENOTSOCK;
	}
	fibril_rwlock_read_lock( & socket->sending_lock );
	// compute data fragment count
	if( socket->data_fragment_size > 0 ){
		fragments = ( datalength + socket->header_size ) / socket->data_fragment_size;
		if(( datalength + socket->header_size ) % socket->data_fragment_size ) ++ fragments;
	}else{
		fragments = 1;
	}
	// request send
	message_id = async_send_5( socket->phone, message, ( ipcarg_t ) socket->socket_id, ( fragments == 1 ? datalength : socket->data_fragment_size ), socket->service, ( ipcarg_t ) flags, fragments, & answer );
	// send the address if given
	if(( ! toaddr ) || ( async_data_write_start( socket->phone, toaddr, addrlen ) == EOK )){
		if( fragments == 1 ){
			// send all if only one fragment
			async_data_write_start( socket->phone, data, datalength );
		}else{
			// send the first fragment
			async_data_write_start( socket->phone, data, socket->data_fragment_size - socket->header_size );
			data = (( const uint8_t * ) data ) + socket->data_fragment_size - socket->header_size;
			// send the middle fragments
			while(( -- fragments ) > 1 ){
				async_data_write_start( socket->phone, data, socket->data_fragment_size );
				data = (( const uint8_t * ) data ) + socket->data_fragment_size;
			}
			// send the last fragment
			async_data_write_start( socket->phone, data, ( datalength + socket->header_size ) % socket->data_fragment_size );
		}
	}
	async_wait_for( message_id, & result );
	if(( SOCKET_GET_DATA_FRAGMENT_SIZE( answer ) > 0 )
	&& ( SOCKET_GET_DATA_FRAGMENT_SIZE( answer ) != socket->data_fragment_size )){
		// set the data fragment size
		socket->data_fragment_size = SOCKET_GET_DATA_FRAGMENT_SIZE( answer );
	}
	fibril_rwlock_read_unlock( & socket->sending_lock );
	fibril_rwlock_read_unlock( & socket_globals.lock );
	return ( int ) result;
}

int recv( int socket_id, void * data, size_t datalength, int flags ){
	// without the address
	return recvfrom_core( NET_SOCKET_RECV, socket_id, data, datalength, flags, NULL, NULL );
}

int recvfrom( int socket_id, void * data, size_t datalength, int flags, struct sockaddr * fromaddr, socklen_t * addrlen ){
	if( ! fromaddr ) return EBADMEM;
	if( ! addrlen ) return NO_DATA;
	// with the address
	return recvfrom_core( NET_SOCKET_RECVFROM, socket_id, data, datalength, flags, fromaddr, addrlen );
}

int recvfrom_core( ipcarg_t message, int socket_id, void * data, size_t datalength, int flags, struct sockaddr * fromaddr, socklen_t * addrlen ){
	socket_ref		socket;
	aid_t			message_id;
	ipcarg_t		ipc_result;
	int			result;
	size_t			fragments;
	size_t *		lengths;
	size_t			index;
	ipc_call_t		answer;

	if( ! data ) return EBADMEM;
	if( ! datalength ) return NO_DATA;
	if( fromaddr && ( ! addrlen )) return EINVAL;
	fibril_rwlock_read_lock( & socket_globals.lock );
	// find the socket
	socket = sockets_find( socket_get_sockets(), socket_id );
	if( ! socket ){
		fibril_rwlock_read_unlock( & socket_globals.lock );
		return ENOTSOCK;
	}
	fibril_mutex_lock( & socket->receive_lock );
	// wait for a received packet
	++ socket->blocked;
	while(( result = dyn_fifo_value( & socket->received )) <= 0 ){
		fibril_rwlock_read_unlock( & socket_globals.lock );
		fibril_condvar_wait( & socket->receive_signal, & socket->receive_lock );
		fibril_rwlock_read_lock( & socket_globals.lock );
	}
	-- socket->blocked;
	fragments = ( size_t ) result;
	// prepare lengths if more fragments
	if( fragments > 1 ){
		lengths = ( size_t * ) malloc( sizeof( size_t ) * fragments + sizeof( size_t ));
		if( ! lengths ){
			fibril_mutex_unlock( & socket->receive_lock );
			fibril_rwlock_read_unlock( & socket_globals.lock );
			return ENOMEM;
		}
		// request packet data
		message_id = async_send_4( socket->phone, message, ( ipcarg_t ) socket->socket_id, 0, socket->service, ( ipcarg_t ) flags, & answer );
		// read the address if desired
		if(( ! fromaddr ) || ( async_data_read_start( socket->phone, fromaddr, * addrlen ) == EOK )){
		// read the fragment lengths
			if( async_data_read_start( socket->phone, lengths, sizeof( int ) * ( fragments + 1 )) == EOK ){
				if( lengths[ fragments ] <= datalength ){
					// read all fragments if long enough
					for( index = 0; index < fragments; ++ index ){
						async_data_read_start( socket->phone, data, lengths[ index ] );
						data = (( uint8_t * ) data ) + lengths[ index ];
					}
				}
			}
		}
		free( lengths );
	}else{
		// request packet data
		message_id = async_send_4( socket->phone, message, ( ipcarg_t ) socket->socket_id, 0, socket->service, ( ipcarg_t ) flags, & answer );
		// read the address if desired
		if(( ! fromaddr ) || ( async_data_read_start( socket->phone, fromaddr, * addrlen ) == EOK )){
			// read all if only one fragment
			async_data_read_start( socket->phone, data, datalength );
		}
	}
	async_wait_for( message_id, & ipc_result );
	result = (int) ipc_result;
	// if successful
	if( result == EOK ){
		// dequeue the received packet
		dyn_fifo_pop( & socket->received );
		// return read data length
		result = SOCKET_GET_READ_DATA_LENGTH( answer );
		// set address length
		if( fromaddr && addrlen ) * addrlen = SOCKET_GET_ADDRESS_LENGTH( answer );
	}
	fibril_mutex_unlock( & socket->receive_lock );
	fibril_rwlock_read_unlock( & socket_globals.lock );
	return result;
}

int getsockopt( int socket_id, int level, int optname, void * value, size_t * optlen ){
	socket_ref		socket;
	aid_t			message_id;
	ipcarg_t		result;

	if( !( value && optlen )) return EBADMEM;
	if( !( * optlen )) return NO_DATA;
	fibril_rwlock_read_lock( & socket_globals.lock );
	// find the socket
	socket = sockets_find( socket_get_sockets(), socket_id );
	if( ! socket ){
		fibril_rwlock_read_unlock( & socket_globals.lock );
		return ENOTSOCK;
	}
	// request option value
	message_id = async_send_3( socket->phone, NET_SOCKET_GETSOCKOPT, ( ipcarg_t ) socket->socket_id, ( ipcarg_t ) optname, socket->service, NULL );
	// read the length
	if( async_data_read_start( socket->phone, optlen, sizeof( * optlen )) == EOK ){
		// read the value
		async_data_read_start( socket->phone, value, * optlen );
	}
	fibril_rwlock_read_unlock( & socket_globals.lock );
	async_wait_for( message_id, & result );
	return ( int ) result;
}

int setsockopt( int socket_id, int level, int optname, const void * value, size_t optlen ){
	// send the value
	return socket_send_data( socket_id, NET_SOCKET_SETSOCKOPT, ( ipcarg_t ) optname, value, optlen );

}

/** @}
 */
