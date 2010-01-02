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

/** @addtogroup nettest
 *  @{
 */

/** @file
 *  Networking test 2 application - transfer.
 */

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <time.h>

#include "../../include/in.h"
#include "../../include/in6.h"
#include "../../include/inet.h"
#include "../../include/socket.h"

#include "../../err.h"

#include "../parse.h"
#include "../print_error.h"

/** Echo module name.
 */
#define NAME	"Nettest2"

/** Packet data pattern.
 */
#define NETTEST2_TEXT	"Networking test 2 - transfer"

/** Module entry point.
 *  Starts testing.
 *  @param[in] argc The number of command line parameters.
 *  @param[in] argv The command line parameters.
 *  @returns EOK on success.
 */
int		main( int argc, char * argv[] );

/** Prints the application help.
 */
void	print_help( void );

/** Translates the character string to the protocol family number.
 *  @param[in] name The protocol family name.
 *  @returns The corresponding protocol family number.
 *  @returns EPFNOSUPPORTED if the protocol family is not supported.
 */
int		parse_protocol_family( const char * name );

/** Translates the character string to the socket type number.
 *  @param[in] name The socket type name.
 *  @returns The corresponding socket type number.
 *  @returns ESOCKNOSUPPORTED if the socket type is not supported.
 */
int		parse_socket_type( const char * name );

/** Refreshes the data.
 *  Fills the data block with the NETTEST1_TEXT pattern.
 *  @param[out] data The data block.
 *  @param[in] size The data block size in bytes.
 */
void	refresh_data( char * data, size_t size );

/** Creates new sockets.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[out] socket_ids A field to store the socket identifiers.
 *  @param[in] sockets The number of sockets to create. Should be at most the size of the field.
 *  @param[in] family The socket address family.
 *  @param[in] type The socket type.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the socket() function.
 */
int	sockets_create( int verbose, int * socket_ids, int sockets, int family, sock_type_t type );

/** Closes sockets.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[in] socket_ids A field of stored socket identifiers.
 *  @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the closesocket() function.
 */
int	sockets_close( int verbose, int * socket_ids, int sockets );

/** Connects sockets.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[in] socket_ids A field of stored socket identifiers.
 *  @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 *  @param[in] address The destination host address to connect to.
 *  @param[in] addrlen The length of the destination address in bytes.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the connect() function.
 */
int	sockets_connect( int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t addrlen );

/** Sends data via sockets.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[in] socket_ids A field of stored socket identifiers.
 *  @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 *  @param[in] address The destination host address to send data to.
 *  @param[in] addrlen The length of the destination address in bytes.
 *  @param[in] data The data to be sent.
 *  @param[in] size The data size in bytes.
 *  @param[in] messages The number of datagrams per socket to be sent.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the sendto() function.
 */
int	sockets_sendto( int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t addrlen, char * data, int size, int messages );

/** Receives data via sockets.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[in] socket_ids A field of stored socket identifiers.
 *  @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 *  @param[in] address The source host address of received datagrams.
 *  @param[in,out] addrlen The maximum length of the source address in bytes. The actual size of the source address is set instead.
 *  @param[out] data The received data.
 *  @param[in] size The maximum data size in bytes.
 *  @param[in] messages The number of datagrams per socket to be received.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the recvfrom() function.
 */
int	sockets_recvfrom( int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t * addrlen, char * data, int size, int messages );

/** Sends and receives data via sockets.
 *  Each datagram is sent and a reply read consequently.
 *  The next datagram is sent after the reply is received.
 *  @param[in] verbose A value indicating whether to print out verbose information.
 *  @param[in] socket_ids A field of stored socket identifiers.
 *  @param[in] sockets The number of sockets in the field. Should be at most the size of the field.
 *  @param[in,out] address The destination host address to send data to. The source host address of received datagrams is set instead.
 *  @param[in] addrlen The length of the destination address in bytes.
 *  @param[in,out] data The data to be sent. The received data are set instead.
 *  @param[in] size The data size in bytes.
 *  @param[in] messages The number of datagrams per socket to be received.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the recvfrom() function.
 */
int	sockets_sendto_recvfrom( int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t * addrlen, char * data, int size, int messages );

/** Prints a mark.
 *  If the index is a multiple of ten, a different mark is printed.
 *  @param[in] index The index of the mark to be printed.
 */
void	print_mark( int index );

void print_help( void ){
	printf(
		"Network Networking test 2 aplication - UDP transfer\n" \
		"Usage: echo [options] numeric_address\n" \
		"Where options are:\n" \
		"-f protocol_family | --family=protocol_family\n" \
		"\tThe listenning socket protocol family. Only the PF_INET and PF_INET6 are supported.\n"
		"\n" \
		"-h | --help\n" \
		"\tShow this application help.\n"
		"\n" \
		"-m count | --messages=count\n" \
		"\tThe number of messages to send and receive per socket. The default is 10.\n" \
		"\n" \
		"-n sockets | --sockets=count\n" \
		"\tThe number of sockets to use. The default is 10.\n" \
		"\n" \
		"-p port_number | --port=port_number\n" \
		"\tThe port number the application should send messages to. The default is 7.\n" \
		"\n" \
		"-s packet_size | --size=packet_size\n" \
		"\tThe packet data size the application sends. The default is 29 bytes.\n" \
		"\n" \
		"-v | --verbose\n" \
		"\tShow all output messages.\n"
	);
}

int parse_protocol_family( const char * name ){
	if( str_lcmp( name, "PF_INET", 7 ) == 0 ){
		return PF_INET;
	}else if( str_lcmp( name, "PF_INET6", 8 ) == 0 ){
		return PF_INET6;
	}
	return EPFNOSUPPORT;
}

int parse_socket_type( const char * name ){
	if( str_lcmp( name, "SOCK_DGRAM", 11 ) == 0 ){
		return SOCK_DGRAM;
	}else if( str_lcmp( name, "SOCK_STREAM", 12 ) == 0 ){
		return SOCK_STREAM;
	}
	return ESOCKTNOSUPPORT;
}

void refresh_data( char * data, size_t size ){
	size_t	length;

	// fill the data
	length = 0;
	while( size > length + sizeof( NETTEST2_TEXT ) - 1 ){
		memcpy( data + length, NETTEST2_TEXT, sizeof( NETTEST2_TEXT ) - 1 );
		length += sizeof( NETTEST2_TEXT ) - 1;
	}
	memcpy( data + length, NETTEST2_TEXT, size - length );
	data[ size ] = '\0';
}

int sockets_create( int verbose, int * socket_ids, int sockets, int family, sock_type_t type ){
	int	index;

	if( verbose ) printf( "Create\t" );
	fflush( stdout );
	for( index = 0; index < sockets; ++ index ){
		socket_ids[ index ] = socket( family, type, 0 );
		if( socket_ids[ index ] < 0 ){
			printf( "Socket %d (%d) error:\n", index, socket_ids[ index ] );
			socket_print_error( stderr, socket_ids[ index ], "Socket create: ", "\n" );
			return socket_ids[ index ];
		}
		if( verbose ) print_mark( index );
	}
	return EOK;
}

int sockets_close( int verbose, int * socket_ids, int sockets ){
	ERROR_DECLARE;

	int	index;

	if( verbose ) printf( "\tClose\t" );
	fflush( stdout );
	for( index = 0; index < sockets; ++ index ){
		if( ERROR_OCCURRED( closesocket( socket_ids[ index ] ))){
			printf( "Socket %d (%d) error:\n", index, socket_ids[ index ] );
			socket_print_error( stderr, ERROR_CODE, "Socket close: ", "\n" );
			return ERROR_CODE;
		}
		if( verbose ) print_mark( index );
	}
	return EOK;
}

int sockets_connect( int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t addrlen ){
	ERROR_DECLARE;

	int	index;

	if( verbose ) printf( "\tConnect\t" );
	fflush( stdout );
	for( index = 0; index < sockets; ++ index ){
		if( ERROR_OCCURRED( connect( socket_ids[ index ], address, addrlen ))){
			socket_print_error( stderr, ERROR_CODE, "Socket connect: ", "\n" );
			return ERROR_CODE;
		}
		if( verbose ) print_mark( index );
	}
	return EOK;
}

int sockets_sendto( int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t addrlen, char * data, int size, int messages ){
	ERROR_DECLARE;

	int	index;
	int	message;

	if( verbose ) printf( "\tSendto\t" );
	fflush( stdout );
	for( index = 0; index < sockets; ++ index ){
		for( message = 0; message < messages; ++ message ){
			if( ERROR_OCCURRED( sendto( socket_ids[ index ], data, size, 0, address, addrlen ))){
				printf( "Socket %d (%d), message %d error:\n", index, socket_ids[ index ], message );
				socket_print_error( stderr, ERROR_CODE, "Socket send: ", "\n" );
				return ERROR_CODE;
			}
		}
		if( verbose ) print_mark( index );
	}
	return EOK;
}

int sockets_recvfrom( int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t * addrlen, char * data, int size, int messages ){
	int	value;
	int	index;
	int	message;

	if( verbose ) printf( "\tRecvfrom\t" );
	fflush( stdout );
	for( index = 0; index < sockets; ++ index ){
		for( message = 0; message < messages; ++ message ){
			value = recvfrom( socket_ids[ index ], data, size, 0, address, addrlen );
			if( value < 0 ){
				printf( "Socket %d (%d), message %d error:\n", index, socket_ids[ index ], message );
				socket_print_error( stderr, value, "Socket receive: ", "\n" );
				return value;
			}
		}
		if( verbose ) print_mark( index );
	}
	return EOK;
}

int sockets_sendto_recvfrom( int verbose, int * socket_ids, int sockets, struct sockaddr * address, socklen_t * addrlen, char * data, int size, int messages ){
	ERROR_DECLARE;

	int	value;
	int	index;
	int	message;

	if( verbose ) printf( "\tSendto and recvfrom\t" );
	fflush( stdout );
	for( index = 0; index < sockets; ++ index ){
		for( message = 0; message < messages; ++ message ){
			if( ERROR_OCCURRED( sendto( socket_ids[ index ], data, size, 0, address, * addrlen ))){
				printf( "Socket %d (%d), message %d error:\n", index, socket_ids[ index ], message );
				socket_print_error( stderr, ERROR_CODE, "Socket send: ", "\n" );
				return ERROR_CODE;
			}
			value = recvfrom( socket_ids[ index ], data, size, 0, address, addrlen );
			if( value < 0 ){
				printf( "Socket %d (%d), message %d error:\n", index, socket_ids[ index ], message );
				socket_print_error( stderr, value, "Socket receive: ", "\n" );
				return value;
			}
		}
		if( verbose ) print_mark( index );
	}
	return EOK;
}

void print_mark( int index ){
	if(( index + 1 ) % 10 ){
		printf( "*" );
	}else{
		printf( "|" );
	}
	fflush( stdout );
}

int main( int argc, char * argv[] ){
	ERROR_DECLARE;

	size_t				size			= 29;
	int					verbose			= 0;
	sock_type_t			type			= SOCK_DGRAM;
	int					sockets			= 10;
	int					messages		= 10;
	int					family			= PF_INET;
	uint16_t			port			= 7;

	socklen_t			max_length		= sizeof( struct sockaddr_in6 );
	uint8_t				address_data[ max_length ];
	struct sockaddr *		address		= ( struct sockaddr * ) address_data;
	struct sockaddr_in *	address_in		= ( struct sockaddr_in * ) address;
	struct sockaddr_in6 *	address_in6	= ( struct sockaddr_in6 * ) address;
	socklen_t			addrlen;
//	char				address_string[ INET6_ADDRSTRLEN ];
	uint8_t *			address_start;

	int *				socket_ids;
	char * 				data;
	int					value;
	int					index;
	struct timeval		time_before;
	struct timeval		time_after;

	printf( "Task %d - ", task_get_id());
	printf( "%s\n", NAME );

	if( argc <= 1 ){
		print_help();
		return EINVAL;
	}

	for( index = 1; ( index < argc - 1 ) || (( index == argc ) && ( argv[ index ][ 0 ] == '-' )); ++ index ){
		if( argv[ index ][ 0 ] == '-' ){
			switch( argv[ index ][ 1 ] ){
				case 'f':	ERROR_PROPAGATE( parse_parameter_name_int( argc, argv, & index, & family, "protocol family", 0, parse_protocol_family ));
							break;
				case 'h':	print_help();
							return EOK;
							break;
				case 'm':	ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & messages, "message count", 0 ));
							break;
				case 'n':	ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & sockets, "socket count", 0 ));
							break;
				case 'p':	ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & value, "port number", 0 ));
							port = ( uint16_t ) value;
							break;
				case 's':	ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & value, "packet size", 0 ));
							size = (value >= 0 ) ? ( size_t ) value : 0;
							break;
				case 't':	ERROR_PROPAGATE( parse_parameter_name_int( argc, argv, & index, & value, "socket type", 0, parse_socket_type ));
							type = ( sock_type_t ) value;
							break;
				case 'v':	verbose = 1;
							break;
				case '-':	if( str_lcmp( argv[ index ] + 2, "family=", 7 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_name_int( argc, argv, & index, & family, "protocol family", 9, parse_protocol_family ));
							}else if( str_lcmp( argv[ index ] + 2, "help", 5 ) == 0 ){
								print_help();
								return EOK;
							}else if( str_lcmp( argv[ index ] + 2, "messages=", 6 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & messages, "message count", 8 ));
							}else if( str_lcmp( argv[ index ] + 2, "sockets=", 6 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & sockets, "socket count", 8 ));
							}else if( str_lcmp( argv[ index ] + 2, "port=", 5 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & value, "port number", 7 ));
								port = ( uint16_t ) value;
							}else if( str_lcmp( argv[ index ] + 2, "type=", 5 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_name_int( argc, argv, & index, & value, "socket type", 7, parse_socket_type ));
								type = ( sock_type_t ) value;
							}else if( str_lcmp( argv[ index ] + 2, "verbose", 8 ) == 0 ){
								verbose = 1;
							}else{
								print_unrecognized( index, argv[ index ] + 2 );
								print_help();
								return EINVAL;
							}
							break;
				default:
					print_unrecognized( index, argv[ index ] + 1 );
					print_help();
					return EINVAL;
			}
		}else{
			print_unrecognized( index, argv[ index ] );
			print_help();
			return EINVAL;
		}
	}

	bzero( address_data, max_length );
	switch( family ){
		case PF_INET:
			address_in->sin_family = AF_INET;
			address_in->sin_port = htons( port );
			address_start = ( uint8_t * ) & address_in->sin_addr.s_addr;
			addrlen = sizeof( struct sockaddr_in );
			break;
		case PF_INET6:
			address_in6->sin6_family = AF_INET6;
			address_in6->sin6_port = htons( port );
			address_start = ( uint8_t * ) & address_in6->sin6_addr.s6_addr;
			addrlen = sizeof( struct sockaddr_in6 );
			break;
		default:
			fprintf( stderr, "Address family is not supported\n" );
			return EAFNOSUPPORT;
	}

	if( ERROR_OCCURRED( inet_pton( family, argv[ argc - 1 ], address_start ))){
		fprintf( stderr, "Address parse error %d\n", ERROR_CODE );
		return ERROR_CODE;
	}

	if( size <= 0 ){
		fprintf( stderr, "Data buffer size too small (%d). Using 1024 bytes instead.\n", size );
		size = 1024;
	}
	// size plus terminating null (\0)
	data = ( char * ) malloc( size + 1 );
	if( ! data ){
		fprintf( stderr, "Failed to allocate data buffer.\n" );
		return ENOMEM;
	}
	refresh_data( data, size );

	if( sockets <= 0 ){
		fprintf( stderr, "Socket count too small (%d). Using 2 instead.\n", sockets );
		sockets = 2;
	}
	// count plus terminating null (\0)
	socket_ids = ( int * ) malloc( sizeof( int ) * ( sockets + 1 ));
	if( ! socket_ids ){
		fprintf( stderr, "Failed to allocate receive buffer.\n" );
		return ENOMEM;
	}
	socket_ids[ sockets ] = NULL;

	if( verbose ) printf( "Starting tests\n" );

	ERROR_PROPAGATE( sockets_create( verbose, socket_ids, sockets, family, type ));
	if( type == SOCK_STREAM ){
		ERROR_PROPAGATE( sockets_connect( verbose, socket_ids, sockets, address, addrlen ));
	}

	if( ERROR_OCCURRED( gettimeofday( & time_before, NULL ))){
		fprintf( stderr, "Get time of day error %d\n", ERROR_CODE );
		return ERROR_CODE;
	}

	ERROR_PROPAGATE( sockets_sendto_recvfrom( verbose, socket_ids, sockets, address, & addrlen, data, size, messages ));

	if( ERROR_OCCURRED( gettimeofday( & time_after, NULL ))){
		fprintf( stderr, "Get time of day error %d\n", ERROR_CODE );
		return ERROR_CODE;
	}

	if( verbose ) printf( "\tOK\n" );

	printf( "sendto + recvfrom tested in %d microseconds\n", tv_sub( & time_after, & time_before ));

	if( ERROR_OCCURRED( gettimeofday( & time_before, NULL ))){
		fprintf( stderr, "Get time of day error %d\n", ERROR_CODE );
		return ERROR_CODE;
	}

	ERROR_PROPAGATE( sockets_sendto( verbose, socket_ids, sockets, address, addrlen, data, size, messages ));
	ERROR_PROPAGATE( sockets_recvfrom( verbose, socket_ids, sockets, address, & addrlen, data, size, messages ));

	if( ERROR_OCCURRED( gettimeofday( & time_after, NULL ))){
		fprintf( stderr, "Get time of day error %d\n", ERROR_CODE );
		return ERROR_CODE;
	}

	if( verbose ) printf( "\tOK\n" );

	printf( "sendto, recvfrom tested in %d microseconds\n", tv_sub( & time_after, & time_before ));

	ERROR_PROPAGATE( sockets_close( verbose, socket_ids, sockets ));

	if( verbose ) printf( "Exiting\n" );

	return EOK;
}

/** @}
 */
