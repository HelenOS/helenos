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
 *  Character string to integer map implementation.
 *  @see char_map.h
 */

#include <errno.h>
#include <malloc.h>
#include <mem.h>
#include <unistd.h>

#include "char_map.h"

/** Internal magic value for a&nbsp;consistency check.
 */
#define CHAR_MAP_MAGIC_VALUE	0x12345611

/** Adds the value with the key to the map.
 *  Creates new nodes to map the key.
 *  @param[in,out] map The character string to integer map.
 *  @param[in] identifier The key zero ('\\0') terminated character string. The key character string is processed until the first terminating zero ('\\0') character after the given length is found.
 *  @param[in] length The key character string length. The parameter may be zero (0) which means that the string is processed until the terminating zero ('\\0') character is found.
 *  @param[in] value The integral value to be stored for the key character string.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns EEXIST if the key character string is already used.
 */
int	char_map_add_item( char_map_ref map, const char * identifier, size_t length, const int value );

/** Returns the node assigned to the key from the map.
 *  @param[in] map The character string to integer map.
 *  @param[in] identifier The key zero ('\\0') terminated character string. The key character string is processed until the first terminating zero ('\\0') character after the given length is found.
 *  @param[in] length The key character string length. The parameter may be zero (0) which means that the string is processed until the terminating zero ('\\0') character is found.
 *  @returns The node holding the integral value assigned to the key character string.
 *  @returns NULL if the key is not assigned a&nbsp;node.
 */
char_map_ref	char_map_find_node( const char_map_ref map, const char * identifier, const size_t length );

/** Returns the value assigned to the map.
 *  @param[in] map The character string to integer map.
 *  @returns The integral value assigned to the map.
 *  @returns CHAR_MAP_NULL if the map is not assigned a&nbsp;value.
 */
int	char_map_get_value( const char_map_ref map );

/** Checks if the map is valid.
 *  @param[in] map The character string to integer map.
 *  @returns TRUE if the map is valid.
 *  @returns FALSE otherwise.
 */
int	char_map_is_valid( const char_map_ref map );

int char_map_add( char_map_ref map, const char * identifier, size_t length, const int value ){
	if( char_map_is_valid( map ) && ( identifier ) && (( length ) || ( * identifier ))){
		int	index;

		for( index = 0; index < map->next; ++ index ){
			if( map->items[ index ]->c == * identifier ){
				++ identifier;
				if(( length > 1 ) || (( length == 0 ) && ( * identifier ))){
					return char_map_add( map->items[ index ], identifier, length ? length - 1 : 0, value );
				}else{
					if( map->items[ index ]->value != CHAR_MAP_NULL ) return EEXISTS;
					map->items[ index ]->value = value;
					return EOK;
				}
			}
		}
		return char_map_add_item( map, identifier, length, value );
	}
	return EINVAL;
}

int char_map_add_item( char_map_ref map, const char * identifier, size_t length, const int value ){
	if( map->next == ( map->size - 1 )){
		char_map_ref	* tmp;

		tmp = ( char_map_ref * ) realloc( map->items, sizeof( char_map_ref ) * 2 * map->size );
		if( ! tmp ) return ENOMEM;
		map->size *= 2;
		map->items = tmp;
	}
	map->items[ map->next ] = ( char_map_ref ) malloc( sizeof( char_map_t ));
	if( ! map->items[ map->next ] ) return ENOMEM;
	if( char_map_initialize( map->items[ map->next ] ) != EOK ){
		free( map->items[ map->next ] );
		map->items[ map->next ] = NULL;
		return ENOMEM;
	}
	map->items[ map->next ]->c = * identifier;
	++ identifier;
	++ map->next;
	if(( length > 1 ) || (( length == 0 ) && ( * identifier ))){
		map->items[ map->next - 1 ]->value = CHAR_MAP_NULL;
		return char_map_add_item( map->items[ map->next - 1 ], identifier, length ? length - 1 : 0, value );
	}else{
		map->items[ map->next - 1 ]->value = value;
	}
	return EOK;
}

void char_map_destroy( char_map_ref map ){
	if( char_map_is_valid( map )){
		int	index;

		map->magic = 0;
		for( index = 0; index < map->next; ++ index ){
			char_map_destroy( map->items[ index ] );
		}
		free( map->items );
		map->items = NULL;
	}
}

int char_map_exclude( char_map_ref map, const char * identifier, size_t length ){
	char_map_ref	node;

	node = char_map_find_node( map, identifier, length );
	if( node ){
		int	value;

		value = node->value;
		node->value = CHAR_MAP_NULL;
		return value;
	}
	return CHAR_MAP_NULL;
}

int char_map_find( const char_map_ref map, const char * identifier, size_t length ){
	char_map_ref	node;

	node = char_map_find_node( map, identifier, length );
	return node ? node->value : CHAR_MAP_NULL;
}

char_map_ref char_map_find_node( const char_map_ref map, const char * identifier, size_t length ){
	if( ! char_map_is_valid( map )) return NULL;
	if( length || ( * identifier )){
		int	index;

		for( index = 0; index < map->next; ++ index ){
			if( map->items[ index ]->c == * identifier ){
				++ identifier;
				if( length == 1 ) return map->items[ index ];
				return char_map_find_node( map->items[ index ], identifier, length ? length - 1 : 0 );
			}
		}
		return NULL;
	}
	return map;
}

int char_map_get_value( const char_map_ref map ){
	return char_map_is_valid( map ) ? map->value : CHAR_MAP_NULL;
}

int char_map_initialize( char_map_ref map ){
	if( ! map ) return EINVAL;
	map->c = '\0';
	map->value = CHAR_MAP_NULL;
	map->size = 2;
	map->next = 0;
	map->items = malloc( sizeof( char_map_ref ) * map->size );
	if( ! map->items ){
		map->magic = 0;
		return ENOMEM;
	}
	map->items[ map->next ] = NULL;
	map->magic = CHAR_MAP_MAGIC_VALUE;
	return EOK;
}

int char_map_is_valid( const char_map_ref map ){
	return map && ( map->magic == CHAR_MAP_MAGIC_VALUE );
}

int char_map_update( char_map_ref map, const char * identifier, const size_t length, const int value ){
	char_map_ref	node;

//	if( ! char_map_is_valid( map )) return EINVAL;
	node = char_map_find_node( map, identifier, length );
	if( node ){
		node->value = value;
		return EOK;
	}else{
		return char_map_add( map, identifier, length, value );
	}
}

/** @}
 */
