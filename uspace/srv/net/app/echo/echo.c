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

/** @addtogroup echo
 *  @{
 */

/** @file
 *  Echo application.
 *  Answers received packets.
 */

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <task.h>

#include "../../include/in.h"
#include "../../include/in6.h"
#include "../../include/inet.h"
#include "../../include/socket.h"

#include "../../err.h"

#include "../parse.h"
#include "../print_error.h"

/** Echo module name.
 */
#define NAME	"Echo"

/** Module entry point.
 *  Reads command line parameters and starts listenning.
 *  @param[in] argc The number of command line parameters.
 *  @param[in] argv The command line parameters.
 *  @returns EOK on success.
 */
int		main( int argc, char * argv[] );

/** Prints the application help.
 */
void	echo_print_help( void );

/** Translates the character string to the protocol family number.
 *  @param[in] name The protocol family name.
 *  @returns The corresponding protocol family number.
 *  @returns EPFNOSUPPORTED if the protocol family is not supported.
 */
int		echo_parse_protocol_family( const char * name );

/** Translates the character string to the socket type number.
 *  @param[in] name The socket type name.
 *  @returns The corresponding socket type number.
 *  @returns ESOCKNOSUPPORTED if the socket type is not supported.
 */
int		echo_parse_socket_type( const char * name );

void echo_print_help( void ){
	printf(
		"Network Echo aplication\n" \
		"Usage: echo [options]\n" \
		"Where options are:\n" \
		"-b backlog | --backlog=size\n" \
		"\tThe size of the accepted sockets queue. Only for SOCK_STREAM. The default is 3.\n" \
		"\n" \
		"-c count | --count=count\n" \
		"\tThe number of received messages to handle. A negative number means infinity. The default is infinity.\n" \
		"\n" \
		"-f protocol_family | --family=protocol_family\n" \
		"\tThe listenning socket protocol family. Only the PF_INET and PF_INET6 are supported.\n"
		"\n" \
		"-h | --help\n" \
		"\tShow this application help.\n"
		"\n" \
		"-p port_number | --port=port_number\n" \
		"\tThe port number the application should listen at. The default is 7.\n" \
		"\n" \
		"-r reply_string | --reply=reply_string\n" \
		"\tThe constant reply string. The default is the original data received.\n" \
		"\n" \
		"-s receive_size | --size=receive_size\n" \
		"\tThe maximum receive data size the application should accept. The default is 1024 bytes.\n" \
		"\n" \
		"-t socket_type | --type=socket_type\n" \
		"\tThe listenning socket type. Only the SOCK_DGRAM and the SOCK_STREAM are supported.\n" \
		"\n" \
		"-v | --verbose\n" \
		"\tShow all output messages.\n"
	);
}

int echo_parse_protocol_family( const char * name ){
	if( str_lcmp( name, "PF_INET", 7 ) == 0 ){
		return PF_INET;
	}else if( str_lcmp( name, "PF_INET6", 8 ) == 0 ){
		return PF_INET6;
	}
	return EPFNOSUPPORT;
}

int echo_parse_socket_type( const char * name ){
	if( str_lcmp( name, "SOCK_DGRAM", 11 ) == 0 ){
		return SOCK_DGRAM;
	}else if( str_lcmp( name, "SOCK_STREAM", 12 ) == 0 ){
		return SOCK_STREAM;
	}
	return ESOCKTNOSUPPORT;
}

int main( int argc, char * argv[] ){
	ERROR_DECLARE;

	size_t				size			= 1024;
	int					verbose			= 0;
	char *				reply			= NULL;
	sock_type_t			type			= SOCK_DGRAM;
	int					count			= -1;
	int					family			= PF_INET;
	uint16_t			port			= 7;
	int					backlog			= 3;

	socklen_t			max_length		= sizeof( struct sockaddr_in6 );
	uint8_t				address_data[ max_length ];
	struct sockaddr *		address		= ( struct sockaddr * ) address_data;
	struct sockaddr_in *	address_in		= ( struct sockaddr_in * ) address;
	struct sockaddr_in6 *	address_in6	= ( struct sockaddr_in6 * ) address;
	socklen_t			addrlen;
	char				address_string[ INET6_ADDRSTRLEN ];
	uint8_t *			address_start;
	int					socket_id;
	int					listening_id;
	char * 				data;
	size_t				length;
	int					index;
	size_t				reply_length;
	int					value;

	printf( "Task %d - ", task_get_id());
	printf( "%s\n", NAME );

	for( index = 1; index < argc; ++ index ){
		if( argv[ index ][ 0 ] == '-' ){
			switch( argv[ index ][ 1 ] ){
				case 'b':	ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & backlog, "accepted sockets queue size", 0 ));
							break;
				case 'c':	ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & count, "message count", 0 ));
							break;
				case 'f':	ERROR_PROPAGATE( parse_parameter_name_int( argc, argv, & index, & family, "protocol family", 0, echo_parse_protocol_family ));
							break;
				case 'h':	echo_print_help();
							return EOK;
							break;
				case 'p':	ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & value, "port number", 0 ));
							port = ( uint16_t ) value;
							break;
				case 'r':	ERROR_PROPAGATE( parse_parameter_string( argc, argv, & index, & reply, "reply string", 0 ));
							break;
				case 's':	ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & value, "receive size", 0 ));
							size = (value >= 0 ) ? ( size_t ) value : 0;
							break;
				case 't':	ERROR_PROPAGATE( parse_parameter_name_int( argc, argv, & index, & value, "socket type", 0, echo_parse_socket_type ));
							type = ( sock_type_t ) value;
							break;
				case 'v':	verbose = 1;
							break;
				case '-':	if( str_lcmp( argv[ index ] + 2, "backlog=", 6 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & backlog, "accepted sockets queue size", 8 ));
							}else if( str_lcmp( argv[ index ] + 2, "count=", 6 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & count, "message count", 8 ));
							}else if( str_lcmp( argv[ index ] + 2, "family=", 7 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_name_int( argc, argv, & index, & family, "protocol family", 9, echo_parse_protocol_family ));
							}else if( str_lcmp( argv[ index ] + 2, "help", 5 ) == 0 ){
								echo_print_help();
								return EOK;
							}else if( str_lcmp( argv[ index ] + 2, "port=", 5 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & value, "port number", 7 ));
								port = ( uint16_t ) value;
							}else if( str_lcmp( argv[ index ] + 2, "reply=", 6 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_string( argc, argv, & index, & reply, "reply string", 8 ));
							}else if( str_lcmp( argv[ index ] + 2, "size=", 5 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_int( argc, argv, & index, & value, "receive size", 7 ));
								size = (value >= 0 ) ? ( size_t ) value : 0;
							}else if( str_lcmp( argv[ index ] + 2, "type=", 5 ) == 0 ){
								ERROR_PROPAGATE( parse_parameter_name_int( argc, argv, & index, & value, "socket type", 7, echo_parse_socket_type ));
								type = ( sock_type_t ) value;
							}else if( str_lcmp( argv[ index ] + 2, "verbose", 8 ) == 0 ){
								verbose = 1;
							}else{
								print_unrecognized( index, argv[ index ] + 2 );
								echo_print_help();
								return EINVAL;
							}
							break;
				default:
					print_unrecognized( index, argv[ index ] + 1 );
					echo_print_help();
					return EINVAL;
			}
		}else{
			print_unrecognized( index, argv[ index ] );
			echo_print_help();
			return EINVAL;
		}
	}

	if( size <= 0 ){
		fprintf( stderr, "Receive size too small (%d). Using 1024 bytes instead.\n", size );
		size = 1024;
	}
	// size plus terminating null (\0)
	data = ( char * ) malloc( size + 1 );
	if( ! data ){
		fprintf( stderr, "Failed to allocate receive buffer.\n" );
		return ENOMEM;
	}

	reply_length = reply ? str_length( reply ) : 0;

	listening_id = socket( family, type, 0 );
	if( listening_id < 0 ){
		socket_print_error( stderr, listening_id, "Socket create: ", "\n" );
		return listening_id;
	}

	bzero( address_data, max_length );
	switch( family ){
		case PF_INET:
			address_in->sin_family = AF_INET;
			address_in->sin_port = htons( port );
			addrlen = sizeof( struct sockaddr_in );
			break;
		case PF_INET6:
			address_in6->sin6_family = AF_INET6;
			address_in6->sin6_port = htons( port );
			addrlen = sizeof( struct sockaddr_in6 );
			break;
		default:
			fprintf( stderr, "Protocol family is not supported\n" );
			return EAFNOSUPPORT;
	}

	listening_id = socket( family, type, 0 );
	if( listening_id < 0 ){
		socket_print_error( stderr, listening_id, "Socket create: ", "\n" );
		return listening_id;
	}

	if( type == SOCK_STREAM ){
		if( backlog <= 0 ){
			fprintf( stderr, "Accepted sockets queue size too small (%d). Using 3 instead.\n", size );
			backlog = 3;
		}
		if( ERROR_OCCURRED( listen( listening_id, backlog ))){
			socket_print_error( stderr, ERROR_CODE, "Socket listen: ", "\n" );
			return ERROR_CODE;
		}
	}

	if( ERROR_OCCURRED( bind( listening_id, address, addrlen ))){
		socket_print_error( stderr, ERROR_CODE, "Socket bind: ", "\n" );
		return ERROR_CODE;
	}

	if( verbose ) printf( "Socket %d listenning at %d\n", listening_id, port );

	socket_id = listening_id;

	while( count ){
		addrlen = max_length;
		if( type == SOCK_STREAM ){
			socket_id = accept( listening_id, address, & addrlen );
			if( socket_id <= 0 ){
				socket_print_error( stderr, socket_id, "Socket accept: ", "\n" );
			}else{
				if( verbose ) printf( "Socket %d accepted\n", socket_id );
			}
		}
		if( socket_id > 0 ){
			value = recvfrom( socket_id, data, size, 0, address, & addrlen );
			if( value < 0 ){
				socket_print_error( stderr, value, "Socket receive: ", "\n" );
			}else{
				length = ( size_t ) value;
				if( verbose ){
					address_start = NULL;
					switch( address->sa_family ){
						case AF_INET:
							port = ntohs( address_in->sin_port );
							address_start = ( uint8_t * ) & address_in->sin_addr.s_addr;
							break;
						case AF_INET6:
							port = ntohs( address_in6->sin6_port );
							address_start = ( uint8_t * ) & address_in6->sin6_addr.s6_addr;
							break;
						default:
							fprintf( stderr, "Address family %d (0x%X) is not supported.\n", address->sa_family );
					}
					if( address_start ){
						if( ERROR_OCCURRED( inet_ntop( address->sa_family, address_start, address_string, sizeof( address_string )))){
							fprintf( stderr, "Received address error %d\n", ERROR_CODE );
						}else{
							data[ length ] = '\0';
							printf( "Socket %d received %d bytes from %s:%d\n%s\n", socket_id, length, address_string, port, data );
						}
					}
				}
				if( ERROR_OCCURRED( sendto( socket_id, reply ? reply : data, reply ? reply_length : length, 0, address, addrlen ))){
					socket_print_error( stderr, ERROR_CODE, "Socket send: ", "\n" );
				}
			}
			if( type == SOCK_STREAM ){
				if( ERROR_OCCURRED( closesocket( socket_id ))){
					socket_print_error( stderr, ERROR_CODE, "Close socket: ", "\n" );
				}
			}
		}
		if( count > 0 ){
			-- count;
			if( verbose ) printf( "Waiting for next %d packet(s)\n", count );
		}
	}

	if( verbose ) printf( "Closing the socket\n" );

	if( ERROR_OCCURRED( closesocket( listening_id ))){
		socket_print_error( stderr, ERROR_CODE, "Close socket: ", "\n" );
		return ERROR_CODE;
	}

	if( verbose ) printf( "Exiting\n" );

	return EOK;
}

/** @}
 */
