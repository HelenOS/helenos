/*
 * Copyright (c) 2006 Jakub Jermar
 * Copyright (c) 2011 Radim Vansa
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
 * @{
 */
/** @file
 */

#ifndef LIBC_HASH_SET_H_
#define LIBC_HASH_SET_H_

#include <adt/list.h>
#include <unistd.h>

#define HASH_SET_MIN_SIZE  8

typedef unsigned long (*hash_set_hash)(const link_t *);
typedef int (*hash_set_equals)(const link_t *, const link_t *);

/** Hash table structure. */
typedef struct {
	list_t *table;
	
	/** Current table size */
	size_t size;
	
	/**
	 * Current number of entries. If count > size,
	 * the table is rehashed into table with double
	 * size. If (4 * count < size) && (size > min_size),
	 * the table is rehashed into table with half the size.
	 */
	size_t count;
	
	/** Hash function */
	hash_set_hash hash;
	
	/** Hash table item equals function */
	hash_set_equals equals;
} hash_set_t;

extern int hash_set_init(hash_set_t *, hash_set_hash, hash_set_equals, size_t);
extern int hash_set_insert(hash_set_t *, link_t *);
extern link_t *hash_set_find(hash_set_t *, const link_t *);
extern int hash_set_contains(const hash_set_t *, const link_t *);
extern size_t hash_set_count(const hash_set_t *);
extern link_t *hash_set_remove(hash_set_t *, const link_t *);
extern void hash_set_remove_selected(hash_set_t *,
    int (*)(link_t *, void *), void *);
extern void hash_set_destroy(hash_set_t *);
extern void hash_set_apply(hash_set_t *, void (*)(link_t *, void *), void *);
extern void hash_set_clear(hash_set_t *, void (*)(link_t *, void *), void *);

#endif

/** @}
 */
