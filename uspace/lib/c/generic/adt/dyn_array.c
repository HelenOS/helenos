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
 *
 * Implementation of dynamic array that grows or shrinks based upon no. of
 * items it contains. Non-negligible part of implementation is in @ref
 * dyn_array.h because of type genericity.
 */

#include <assert.h>
#include <errno.h>
#include <adt/dyn_array.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>




static int dyn_array_realloc(dyn_array_t *da, size_t capacity)
{
	if (capacity == da->capacity) {
		return EOK;
	}

	void *new_data = realloc(da->_data, da->_item_size * capacity);
	if (new_data) {
		da->_data = new_data;
		da->capacity = capacity;
	}
	return (new_data == NULL) ? ENOMEM : EOK;
}

void dyn_array_destroy(dyn_array_t *da)
{
	dyn_array_clear(da);
	free(da->_data);
	da->capacity = 0;
}

/** Remove item at given position, shift rest of array */
void dyn_array_remove(dyn_array_t *da, size_t index)
{
	assert(index < da->size);
	_dyn_array_unshift(da, index, 1);
	int rc = dyn_array_reserve(da, da->size);
        assert(rc == EOK);
}

/** Clear dynamic array (empty) */
void dyn_array_clear(dyn_array_t *da)
{
	da->size = 0;
}

/** Clear subsequence of array
 *
 * @param[in/out]  da
 * @param[in]      begin  index of first item to remove
 * @param[in]      end    index behind last item to remove
 */
void dyn_array_clear_range(dyn_array_t *da, size_t begin, size_t end)
{
	assert(begin < da->size);
	assert(end <= da->size);

	_dyn_array_unshift(da, begin, end - begin);
	int rc = dyn_array_reserve(da, da->size);
        assert(rc == EOK);
}

/** Concatenate two arrays
 *
 * @param[in/out]  da1  first array and concatenated output
 * @param[in]      da2  second array, it is untouched
 *
 * @return EOK on success
 * @return ENOMEM when allocation fails
 */
int dyn_array_concat(dyn_array_t *da1, dyn_array_t *da2)
{
	assert(da1->_item_size == da2->_item_size);

	int rc = dyn_array_reserve(da1, da1->size + da2->size);
	if (rc != EOK) {
		return rc;
	}
	void *dst = da1->_data + da1->size * da1->_item_size;
	void *src = da2->_data;
	size_t size = da1->_item_size * da2->size;
	memcpy(dst, src, size);
	da1->size += da2->size;

	return EOK;
}

/** Grows/shrinks array so that it effeciently stores desired capacity
 *
 * @param      da
 * @param[in]  desired capacity of array (items)
 *
 * @return EOK
 * @return ENOMEM
 */
int dyn_array_reserve(dyn_array_t *da, size_t capacity)
{
	const size_t factor = 2;
	size_t new_capacity;
	if (capacity > da->capacity) {
		new_capacity = max(da->capacity * factor, capacity);
	} else if (capacity < da->capacity / factor) {
		new_capacity = min(da->capacity / factor, capacity);
	} else {
		new_capacity = capacity;
	}

	return dyn_array_realloc(da, new_capacity);
}

void _dyn_array_initialize(dyn_array_t *da, size_t item_size)
{
	da->_item_size = item_size;
	da->_data = NULL;

	da->capacity = 0;
	da->size = 0;
}

/** Shift block of array
 *
 * Extends size of dynamic array, assumes sufficient capacity.
 *
 * @param      da
 * @param[in]  index   first item shifted
 * @param[in]  offset  shift in no. of items
 */
void _dyn_array_shift(dyn_array_t *da, size_t index, size_t offset)
{
	assert(da->capacity >= da->size + offset);

	void *src = da->_data + index * da->_item_size;
	void *dst = da->_data + (index + offset) * da->_item_size;
	size_t size = (da->size - index) * da->_item_size;
	memmove(dst, src, size);
	da->size += offset;
}

/** Unshift block of array
 *
 * Reduce size of dynamic array
 *
 * @param      da
 * @param[in]  index   first item unshifted (removed)
 * @param[in]  offset  shift in no. of items
 */
void _dyn_array_unshift(dyn_array_t *da, size_t index, size_t offset)
{
	void *src = da->_data + (index + offset) * da->_item_size;
	void *dst = da->_data + index * da->_item_size;
	size_t size = (da->size - index - offset) * da->_item_size;
	memmove(dst, src, size);
	da->size -= offset;
}


/** @}
 */
