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
 *  Character string to generic type map.
 */

#ifndef __GENERIC_CHAR_MAP_H__
#define __GENERIC_CHAR_MAP_H__

#include <errno.h>
#include <unistd.h>

#include "../err.h"

#include "char_map.h"
#include "generic_field.h"

/** Internal magic value for a&nbsp;map consistency check.
 */
#define GENERIC_CHAR_MAP_MAGIC_VALUE	0x12345622

/** Character string to generic type map declaration.
 *  @param[in] name Name of the map.
 *  @param[in] type Inner object type.
 */
#define GENERIC_CHAR_MAP_DECLARE( name, type )									\
																				\
GENERIC_FIELD_DECLARE( name##_items, type )										\
																				\
typedef	struct name		name##_t;												\
typedef	name##_t *		name##_ref;												\
																				\
struct	name{																	\
	char_map_t		names;														\
	name##_items_t	values;														\
	int				magic;														\
};																				\
																				\
int	name##_add( name##_ref map, const char * name, const size_t length, type * value );	\
int	name##_count( name##_ref map );												\
void	name##_destroy( name##_ref map );										\
void	name##_exclude( name##_ref map, const char * name, const size_t length );	\
type *	name##_find( name##_ref map, const char * name, const size_t length );	\
int	name##_initialize( name##_ref map );										\
int	name##_is_valid( name##_ref map );

/** Character string to generic type map implementation.
 *  Should follow declaration with the same parameters.
 *  @param[in] name Name of the map.
 *  @param[in] type Inner object type.
 */
#define GENERIC_CHAR_MAP_IMPLEMENT( name, type )								\
																				\
GENERIC_FIELD_IMPLEMENT( name##_items, type )									\
																				\
int name##_add( name##_ref map, const char * name, const size_t length, type * value ){	\
	ERROR_DECLARE;																\
																				\
	int	index;																	\
																				\
	if( ! name##_is_valid( map )) return EINVAL;								\
	index = name##_items_add( & map->values, value );							\
	if( index < 0 ) return index;												\
	if( ERROR_OCCURRED( char_map_add( & map->names, name, length, index ))){		\
		name##_items_exclude_index( & map->values, index );						\
		return ERROR_CODE;														\
	}																			\
	return EOK;																	\
}																				\
																				\
int name##_count( name##_ref map ){												\
	return name##_is_valid( map ) ? name##_items_count( & map->values ) : -1;	\
}																				\
																				\
void name##_destroy( name##_ref map ){											\
	if( name##_is_valid( map )){												\
		char_map_destroy( & map->names );										\
		name##_items_destroy( & map->values );									\
	}																			\
}																				\
																				\
void name##_exclude( name##_ref map, const char * name, const size_t length ){	\
	if( name##_is_valid( map )){												\
		int	index;																\
																				\
		index = char_map_exclude( & map->names, name, length );					\
		if( index != CHAR_MAP_NULL ){											\
			name##_items_exclude_index( & map->values, index );					\
		}																		\
	}																			\
}																				\
																				\
type * name##_find( name##_ref map, const char * name, const size_t length ){	\
	if( name##_is_valid( map )){												\
		int	index;																\
																				\
		index = char_map_find( & map->names, name, length );					\
		if( index != CHAR_MAP_NULL ){											\
			return name##_items_get_index( & map->values, index );				\
		}																		\
	}																			\
	return NULL;																\
}																				\
																				\
int name##_initialize( name##_ref map ){										\
	ERROR_DECLARE;																\
																				\
	if( ! map ) return EINVAL;													\
	ERROR_PROPAGATE( char_map_initialize( & map->names ));						\
	if( ERROR_OCCURRED( name##_items_initialize( & map->values ))){				\
		char_map_destroy( & map->names );										\
		return ERROR_CODE;														\
	}																			\
	map->magic = GENERIC_CHAR_MAP_MAGIC_VALUE;									\
	return EOK;																	\
}																				\
																				\
int name##_is_valid( name##_ref map ){											\
	return map && ( map->magic == GENERIC_CHAR_MAP_MAGIC_VALUE );				\
}

#endif

/** @}
 */
