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

/** @addtogroup libc
 *  @{
 */

/** @file
 *  Character string to integer map.
 */

#ifndef LIBC_CHAR_MAP_H_
#define LIBC_CHAR_MAP_H_

#include <libarch/types.h>

/** Invalid assigned value used also if an&nbsp;entry does not exist. */
#define CHAR_MAP_NULL  (-1)

/** Type definition of the character string to integer map.
 *  @see char_map
 */
typedef struct char_map char_map_t;

/** Character string to integer map item.
 *
 * This structure recursivelly contains itself as a character by character tree.
 * The actually mapped character string consists of all the parent characters
 * and the actual one.
 */
struct char_map {
	/** Actually mapped character. */
	uint8_t c;
	/** Stored integral value. */
	int value;
	/** Next character array size. */
	int size;
	/** First free position in the next character array. */
	int next;
	/** Next character array. */
	char_map_t **items;
	/** Consistency check magic value. */
	int magic;
};

extern int char_map_initialize(char_map_t *);
extern void char_map_destroy(char_map_t *);
extern int char_map_exclude(char_map_t *, const uint8_t *, size_t);
extern int char_map_add(char_map_t *, const uint8_t *, size_t, const int);
extern int char_map_find(const char_map_t *, const uint8_t *, size_t);
extern int char_map_update(char_map_t *, const uint8_t *, size_t, const int);

#endif

/** @}
 */
