/*
 * Copyright (c) 2015 Michal Koutny
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

#ifndef LIBC_DYN_ARRAY_H_
#define LIBC_DYN_ARRAY_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct {
	/** Data buffer of array */
	uint8_t *_data;
	/** Size on bytes of a single item */
	size_t _item_size;

	/** Allocated space (in items) of the array */
	size_t capacity;
	/** No. of items in the array */
	size_t size;
} dyn_array_t;

/** Initialize dynamic array
 *
 * @param[in]  capacity  initial capacity of array
 *
 * @return EOK on success
 * @return ENOMEM on low memory
 */
#define dyn_array_initialize(dyn_array, type, capacity)                        \
	_dyn_array_initialize((dyn_array), sizeof(type), capacity)


/** Dynamic array accessor
 *
 * @return lvalue for the given item
 */
#define dyn_array_at(dyn_array, type, index)                                   \
	(*((type *) (dyn_array)->_data + index))

/** Insert item at given position, shift rest of array
 *
 * @return EOK on success
 * @return ENOMEM on failure
 */
#define dyn_array_insert(dyn_array, type, index, value)                        \
({                                                                             \
 	size_t _index = (index);                                               \
 	dyn_array_t *_da = (dyn_array);                                        \
	int rc = _dyn_array_reserve(_da, _da->size + 1);                       \
	if (!rc) {                                                             \
		_dyn_array_shift(_da, _index, 1);                              \
	        dyn_array_at(_da, type, _index) = (value);                     \
	}                                                                      \
	rc;                                                                    \
})

/** Insert item at the end of array
 *
 * @return EOK on success
 * @return ENOMEM on failure
 */
#define dyn_array_append(dyn_array, type, value)                               \
	dyn_array_insert(dyn_array, type, (dyn_array)->size, (value))


/** Dynamic array iteration
 *
 * @param[in]  it   name of variable used as iterator, it's pointer to @p type
 */
#define dyn_array_foreach(dyn_array, type, it)                                 \
	for (type *it = (type *)(dyn_array)._data;                             \
	    it != ((type *)(dyn_array)._data + (dyn_array).size); ++it)


extern void dyn_array_destroy(dyn_array_t *);
extern void dyn_array_remove(dyn_array_t *, size_t);

extern int _dyn_array_initialize(dyn_array_t *, size_t, size_t);
extern int  _dyn_array_reserve(dyn_array_t *, size_t);
extern void _dyn_array_shift(dyn_array_t *, size_t, size_t);
extern void _dyn_array_unshift(dyn_array_t *, size_t, size_t);

#endif

/** @}
 */
