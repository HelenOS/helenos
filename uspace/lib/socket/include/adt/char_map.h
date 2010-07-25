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
 *  Character string to integer map.
 */

#ifndef __CHAR_MAP_H__
#define __CHAR_MAP_H__

/** Invalid assigned value used also if an&nbsp;entry does not exist.
 */
#define CHAR_MAP_NULL	(-1)

/** Type definition of the character string to integer map.
 *  @see char_map
 */
typedef struct char_map	char_map_t;

/** Type definition of the character string to integer map pointer.
 *  @see char_map
 */
typedef char_map_t *	char_map_ref;

/** Character string to integer map item.
 *  This structure recursivelly contains itself as a&nbsp;character by character tree.
 *  The actually mapped character string consists o fall the parent characters and the actual one.
 */
struct	char_map{
	/** Actually mapped character.
	 */
	char c;
	/** Stored integral value.
	 */
	int value;
	/** Next character array size.
	 */
	int size;
	/** First free position in the next character array.
	 */
	int next;
	/** Next character array.
	 */
	char_map_ref * items;
	/** Consistency check magic value.
	 */
	int magic;
};

/** Adds the value with the key to the map.
 *  @param[in,out] map The character string to integer map.
 *  @param[in] identifier The key zero ('\\0') terminated character string. The key character string is processed until the first terminating zero ('\\0') character after the given length is found.
 *  @param[in] length The key character string length. The parameter may be zero (0) which means that the string is processed until the terminating zero ('\\0') character is found.
 *  @param[in] value The integral value to be stored for the key character string.
 *  @returns EOK on success.
 *  @returns EINVAL if the map is not valid.
 *  @returns EINVAL if the identifier parameter is NULL.
 *  @returns EINVAL if the length parameter zero (0) and the identifier parameter is an empty character string (the first character is the terminating zero ('\\0') character.
 *  @returns EEXIST if the key character string is already used.
 *  @returns Other error codes as defined for the char_map_add_item() function.
 */
extern int char_map_add(char_map_ref map, const char * identifier, size_t length, const int value);

/** Clears and destroys the map.
 *  @param[in,out] map The character string to integer map.
 */
extern void char_map_destroy(char_map_ref map);

/** Excludes the value assigned to the key from the map.
 *  The entry is cleared from the map.
 *  @param[in,out] map The character string to integer map.
 *  @param[in] identifier The key zero ('\\0') terminated character string. The key character string is processed until the first terminating zero ('\\0') character after the given length is found.
 *  @param[in] length The key character string length. The parameter may be zero (0) which means that the string is processed until the terminating zero ('\\0') character is found.
 *  @returns The integral value assigned to the key character string.
 *  @returns CHAR_MAP_NULL if the key is not assigned a&nbsp;value.
 */
extern int char_map_exclude(char_map_ref map, const char * identifier, size_t length);

/** Returns the value assigned to the key from the map.
 *  @param[in] map The character string to integer map.
 *  @param[in] identifier The key zero ('\\0') terminated character string. The key character string is processed until the first terminating zero ('\\0') character after the given length is found.
 *  @param[in] length The key character string length. The parameter may be zero (0) which means that the string is processed until the terminating zero ('\\0') character is found.
 *  @returns The integral value assigned to the key character string.
 *  @returns CHAR_MAP_NULL if the key is not assigned a&nbsp;value.
 */
extern int char_map_find(const char_map_ref map, const char * identifier, size_t length);

/** Initializes the map.
 *  @param[in,out] map The character string to integer map.
 *  @returns EOK on success.
 *  @returns EINVAL if the map parameter is NULL.
 *  @returns ENOMEM if there is not enough memory left.
 */
extern int char_map_initialize(char_map_ref map);

/** Adds or updates the value with the key to the map.
 *  @param[in,out] map The character string to integer map.
 *  @param[in] identifier The key zero ('\\0') terminated character string. The key character string is processed until the first terminating zero ('\\0') character after the given length is found.
 *  @param[in] length The key character string length. The parameter may be zero (0) which means that the string is processed until the terminating zero ('\\0') character is found.
 *  @param[in] value The integral value to be stored for the key character string.
 *  @returns EOK on success.
 *  @returns EINVAL if the map is not valid.
 *  @returns EINVAL if the identifier parameter is NULL.
 *  @returns EINVAL if the length parameter zero (0) and the identifier parameter is an empty character string (the first character is the terminating zero ('\\0) character.
 *  @returns EEXIST if the key character string is already used.
 *  @returns Other error codes as defined for the char_map_add_item() function.
 */
extern int char_map_update(char_map_ref map, const char * identifier, size_t length, const int value);

#endif

/** @}
 */
