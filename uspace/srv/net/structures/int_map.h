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
 *  Integer to generic type map.
 */

#ifndef __NET_INT_MAP_H__
#define __NET_INT_MAP_H__

#include <errno.h>
#include <malloc.h>
#include <mem.h>
#include <unistd.h>

/** Internal magic value for a&nbsp;map consistency check.
 */
#define INT_MAP_MAGIC_VALUE			0x11223344

/** Internal magic value for an item consistency check.
 */
#define INT_MAP_ITEM_MAGIC_VALUE	0x55667788

/** Integer to generic type map declaration.
 *  @param[in] name Name of the map.
 *  @param[in] type Inner object type.
 */
#define INT_MAP_DECLARE( name, type )											\
																				\
typedef	struct name			name##_t;											\
typedef	name##_t *			name##_ref;											\
typedef	struct name##_item	name##_item_t;										\
typedef	name##_item_t *		name##_item_ref;									\
																				\
struct	name##_item{															\
	int		key;																\
	type *	value;																\
	int		magic;																\
};																				\
																				\
struct	name{																	\
	int				size;														\
	int				next;														\
	name##_item_ref	items;														\
	int				magic;														\
};																				\
																				\
int		name##_add( name##_ref map, int key, type * value );					\
void	name##_clear( name##_ref map );											\
int		name##_count( name##_ref map );											\
void	name##_destroy( name##_ref map );										\
void	name##_exclude( name##_ref map, int key );								\
void	name##_exclude_index( name##_ref map, int index );						\
type *	name##_find( name##_ref map, int key );									\
type *	name##_get_index( name##_ref map, int index );							\
int		name##_initialize( name##_ref map );									\
int		name##_is_valid( name##_ref map );										\
void	name##_item_destroy( name##_item_ref item );							\
int		name##_item_is_valid( name##_item_ref item );

/** Integer to generic type map implementation.
 *  Should follow declaration with the same parameters.
 *  @param[in] name Name of the map.
 *  @param[in] type Inner object type.
 */
#define INT_MAP_IMPLEMENT( name, type )											\
																				\
int name##_add( name##_ref map, int key, type * value ){						\
	if( name##_is_valid( map )){												\
		if( map->next == ( map->size - 1 )){									\
			name##_item_ref	tmp;												\
																				\
			tmp = ( name##_item_ref ) realloc( map->items, sizeof( name##_item_t ) * 2 * map->size );	\
			if( ! tmp ) return ENOMEM;											\
			map->size *= 2;														\
			map->items = tmp;													\
		}																		\
		map->items[ map->next ].key = key;										\
		map->items[ map->next ].value = value;									\
		map->items[ map->next ].magic = INT_MAP_ITEM_MAGIC_VALUE;				\
		++ map->next;															\
		map->items[ map->next ].magic = 0;										\
		return map->next - 1;													\
	}																			\
	return EINVAL;																\
}																				\
																				\
void name##_clear( name##_ref map ){											\
	if( name##_is_valid( map )){												\
		int	index;																\
																				\
/*		map->magic = 0;*/														\
		for( index = 0; index < map->next; ++ index ){							\
			if( name##_item_is_valid( &( map->items[ index ] ))){				\
				name##_item_destroy( &( map->items[ index ] ));					\
			}																	\
		}																		\
		map->next = 0;															\
		map->items[ map->next ].magic = 0;										\
/*		map->magic = INT_MAP_MAGIC_VALUE;*/										\
	}																			\
}																				\
																				\
int name##_count( name##_ref map ){												\
	return name##_is_valid( map ) ? map->next : -1;								\
}																				\
																				\
void name##_destroy( name##_ref map ){											\
	if( name##_is_valid( map )){												\
		int	index;																\
																				\
		map->magic = 0;															\
		for( index = 0; index < map->next; ++ index ){							\
			if( name##_item_is_valid( &( map->items[ index ] ))){				\
				name##_item_destroy( &( map->items[ index ] ));					\
			}																	\
		}																		\
		free( map->items );														\
	}																			\
}																				\
																				\
void name##_exclude( name##_ref map, int key ){									\
	if( name##_is_valid( map )){												\
		int	index;																\
																				\
		for( index = 0; index < map->next; ++ index ){							\
			if( name##_item_is_valid( &( map->items[ index ] )) && ( map->items[ index ].key == key )){	\
				name##_item_destroy( &( map->items[ index ] ));					\
			}																	\
		}																		\
	}																			\
}																				\
																				\
void name##_exclude_index( name##_ref map, int index ){							\
	if( name##_is_valid( map ) && ( index >= 0 ) && ( index < map->next ) && name##_item_is_valid( &( map->items[ index ] ))){	\
		name##_item_destroy( &( map->items[ index ] ));							\
	}																			\
}																				\
																				\
type * name##_find( name##_ref map, int key ){									\
	if( name##_is_valid( map )){												\
		int	index;																\
																				\
		for( index = 0; index < map->next; ++ index ){							\
			if( name##_item_is_valid( &( map->items[ index ] )) && ( map->items[ index ].key == key )){	\
				return map->items[ index ].value;								\
			}																	\
		}																		\
	}																			\
	return NULL;																\
}																				\
																				\
type * name##_get_index( name##_ref map, int index ){							\
	if( name##_is_valid( map ) && ( index >= 0 ) && ( index < map->next ) && name##_item_is_valid( &( map->items[ index ] ))){	\
		return map->items[ index ].value;										\
	}																			\
	return NULL;																\
}																				\
																				\
int name##_initialize( name##_ref map ){										\
	if( ! map ) return EINVAL;													\
	map->size = 2;																\
	map->next = 0;																\
	map->items = ( name##_item_ref ) malloc( sizeof( name##_item_t ) * map->size );	\
	if( ! map->items ) return ENOMEM;											\
	map->items[ map->next ].magic = 0;											\
	map->magic = INT_MAP_MAGIC_VALUE;											\
	return EOK;																	\
}																				\
																				\
int name##_is_valid( name##_ref map ){											\
	return map && ( map->magic == INT_MAP_MAGIC_VALUE );						\
}																				\
																				\
void name##_item_destroy( name##_item_ref item ){								\
	if( name##_item_is_valid( item )){											\
		item->magic = 0;														\
		if( item->value ){														\
			free( item->value );												\
			item->value = NULL;													\
		}																		\
	}																			\
}																				\
																				\
int name##_item_is_valid( name##_item_ref item ){								\
	return item && ( item->magic == INT_MAP_ITEM_MAGIC_VALUE );					\
}

#endif

/** @}
 */

