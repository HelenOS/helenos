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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Character string with measured length implementation.
 *  @see measured_strings.h
 */

#include <errno.h>
#include <malloc.h>
#include <mem.h>
#include <unistd.h>

#include <ipc/ipc.h>

#include "../err.h"
#include "../modules.h"

#include "measured_strings.h"

/** Computes the lengths of the measured strings in the given array.
 *  @param[in] strings The measured strings array to be processed.
 *  @param[in] count The measured strings array size.
 *  @returns The computed sizes array.
 *  @returns NULL if there is not enough memory left.
 */
size_t *	prepare_lengths( const measured_string_ref strings, size_t count );

measured_string_ref measured_string_create_bulk( const char * string, size_t length ){
	measured_string_ref	new;

	if( length == 0 ){
		while( string[ length ] ) ++ length;
	}
	new = ( measured_string_ref ) malloc( sizeof( measured_string_t ) + ( sizeof( char ) * ( length + 1 )));
	if( ! new ) return NULL;
	new->length = length;
	new->value = (( char * ) new ) + sizeof( measured_string_t );
	// append terminating zero explicitly - to be safe
	memcpy( new->value, string, new->length );
	new->value[ new->length ] = '\0';
	return new;
}

measured_string_ref measured_string_copy( measured_string_ref source ){
	measured_string_ref	new;

	if( ! source ) return NULL;
	new = ( measured_string_ref ) malloc( sizeof( measured_string_t ));
	if( new ){
		new->value = ( char * ) malloc( source->length + 1 );
		if( new->value ){
			new->length = source->length;
			memcpy( new->value, source->value, new->length );
			new->value[ new->length ] = '\0';
			return new;
		}else{
			free( new );
		}
	}
	return NULL;
}

int measured_strings_receive( measured_string_ref * strings, char ** data, size_t count ){
	ERROR_DECLARE;

	size_t *		lengths;
	size_t			index;
	size_t			length;
	char *			next;
	ipc_callid_t	callid;

	if(( ! strings ) || ( ! data ) || ( count <= 0 )){
		return EINVAL;
	}
	lengths = ( size_t * ) malloc( sizeof( size_t ) * ( count + 1 ));
	if( ! lengths ) return ENOMEM;
	if(( ! async_data_write_receive( & callid, & length ))
	|| ( length != sizeof( size_t ) * ( count + 1 ))){
		free( lengths );
		return EINVAL;
	}
	if( ERROR_OCCURRED( async_data_write_finalize( callid, lengths, sizeof( size_t ) * ( count + 1 )))){
		free( lengths );
		return ERROR_CODE;
	}
	* data = malloc( lengths[ count ] );
	if( !( * data )) return ENOMEM;
	( * data )[ lengths[ count ] - 1 ] = '\0';
	* strings = ( measured_string_ref ) malloc( sizeof( measured_string_t ) * count );
	if( !( * strings )){
		free( lengths );
		free( * data );
		return ENOMEM;
	}
	next = * data;
	for( index = 0; index < count; ++ index ){
		( * strings)[ index ].length = lengths[ index ];
		if( lengths[ index ] > 0 ){
			if(( ! async_data_write_receive( & callid, & length ))
			|| ( length != lengths[ index ] )){
				free( * data );
				free( * strings );
				free( lengths );
				return EINVAL;
			}
			ERROR_PROPAGATE( async_data_write_finalize( callid, next, lengths[ index ] ));
			( * strings)[ index ].value = next;
			next += lengths[ index ];
			* next = '\0';
			++ next;
		}else{
			( * strings )[ index ].value = NULL;
		}
	}
	free( lengths );
	return EOK;
}

int measured_strings_reply( const measured_string_ref strings, size_t count ){
	ERROR_DECLARE;

	size_t *		lengths;
	size_t			index;
	size_t			length;
	ipc_callid_t	callid;

	if(( ! strings ) || ( count <= 0 )){
		return EINVAL;
	}
	lengths = prepare_lengths( strings, count );
	if( ! lengths ) return ENOMEM;
	if(( ! async_data_read_receive( & callid, & length ))
	|| ( length != sizeof( size_t ) * ( count + 1 ))){
		free( lengths );
		return EINVAL;
	}
	if( ERROR_OCCURRED( async_data_read_finalize( callid, lengths, sizeof( size_t ) * ( count + 1 )))){
		free( lengths );
		return ERROR_CODE;
	}
	free( lengths );
	for( index = 0; index < count; ++ index ){
		if( strings[ index ].length > 0 ){
			if(( ! async_data_read_receive( & callid, & length ))
			|| ( length != strings[ index ].length )){
				return EINVAL;
			}
			ERROR_PROPAGATE( async_data_read_finalize( callid, strings[ index ].value, strings[ index ].length ));
		}
	}
	return EOK;
}

int measured_strings_return( int phone, measured_string_ref * strings, char ** data, size_t count ){
	ERROR_DECLARE;

	size_t *	lengths;
	size_t		index;
	char *		next;

	if(( phone <= 0 ) || ( ! strings ) || ( ! data ) || ( count <= 0 )){
		return EINVAL;
	}
	lengths = ( size_t * ) malloc( sizeof( size_t ) * ( count + 1 ));
	if( ! lengths ) return ENOMEM;
	if( ERROR_OCCURRED( async_data_read_start( phone, lengths, sizeof( size_t ) * ( count + 1 )))){
		free( lengths );
		return ERROR_CODE;
	}
	* data = malloc( lengths[ count ] );
	if( !( * data )) return ENOMEM;
	* strings = ( measured_string_ref ) malloc( sizeof( measured_string_t ) * count );
	if( !( * strings )){
		free( lengths );
		free( * data );
		return ENOMEM;
	}
	next = * data;
	for( index = 0; index < count; ++ index ){
		( * strings )[ index ].length = lengths[ index ];
		if( lengths[ index ] > 0 ){
			ERROR_PROPAGATE( async_data_read_start( phone, next, lengths[ index ] ));
			( * strings )[ index ].value = next;
			next += lengths[ index ];
			* next = '\0';
			++ next;
		}else{
			( * strings )[ index ].value = NULL;
		}
	}
	free( lengths );
	return EOK;
}

int measured_strings_send( int phone, const measured_string_ref strings, size_t count ){
	ERROR_DECLARE;

	size_t *	lengths;
	size_t		index;

	if(( phone <= 0 ) || ( ! strings ) || ( count <= 0 )){
		return EINVAL;
	}
	lengths = prepare_lengths( strings, count );
	if( ! lengths ) return ENOMEM;
	if( ERROR_OCCURRED( async_data_write_start( phone, lengths, sizeof( size_t ) * ( count + 1 )))){
		free( lengths );
		return ERROR_CODE;
	}
	free( lengths );
	for( index = 0; index < count; ++ index ){
		if( strings[ index ].length > 0 ){
			ERROR_PROPAGATE( async_data_write_start( phone, strings[ index ].value, strings[ index ].length ));
		}
	}
	return EOK;
}

size_t * prepare_lengths( const measured_string_ref strings, size_t count ){
	size_t *	lengths;
	size_t		index;
	size_t		length;

	lengths = ( size_t * ) malloc( sizeof( size_t ) * ( count + 1 ));
	if( ! lengths ) return NULL;
	length = 0;
	for( index = 0; index < count; ++ index ){
		lengths[ index ] = strings[ index ].length;
		length += lengths[ index ] + 1;
	}
	lengths[ count ] = length;
	return lengths;
}

/** @}
 */

