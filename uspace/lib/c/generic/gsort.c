/*
 * Copyright (c) 2005 Sergey Bondari
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

/**
 * @file
 * @brief Sorting functions.
 *
 * This files contains functions implementing several sorting
 * algorithms (e.g. quick sort and gnome sort).
 *
 */

#include <gsort.h>
#include <inttypes.h>
#include <mem.h>
#include <stdlib.h>

/** Immediate buffer size.
 *
 * For small buffer sizes avoid doing malloc()
 * and use the stack.
 *
 */
#define IBUF_SIZE  32

/** Array accessor.
 *
 */
#define INDEX(buf, i, elem_size)  ((buf) + (i) * (elem_size))

/** Gnome sort
 *
 * Apply generic gnome sort algorithm on supplied data,
 * using pre-allocated buffer.
 *
 * @param data      Pointer to data to be sorted.
 * @param cnt       Number of elements to be sorted.
 * @param elem_size Size of one element.
 * @param cmp       Comparator function.
 * @param arg       3rd argument passed to cmp.
 * @param slot      Pointer to scratch memory buffer
 *                  elem_size bytes long.
 *
 */
static void _gsort(void *data, size_t cnt, size_t elem_size, sort_cmp_t cmp,
    void *arg, void *slot)
{
	size_t i = 0;

	while (i < cnt) {
		if ((i != 0) &&
		    (cmp(INDEX(data, i, elem_size),
		    INDEX(data, i - 1, elem_size), arg) == -1)) {
			memcpy(slot, INDEX(data, i, elem_size), elem_size);
			memcpy(INDEX(data, i, elem_size), INDEX(data, i - 1, elem_size),
			    elem_size);
			memcpy(INDEX(data, i - 1, elem_size), slot, elem_size);
			i--;
		} else
			i++;
	}
}

/** Gnome sort wrapper
 *
 * This is only a wrapper that takes care of memory
 * allocations for storing the slot element for generic
 * gnome sort algorithm.
 *
 * @param data      Pointer to data to be sorted.
 * @param cnt       Number of elements to be sorted.
 * @param elem_size Size of one element.
 * @param cmp       Comparator function.
 * @param arg       3rd argument passed to cmp.
 *
 * @return True if sorting succeeded.
 *
 */
bool gsort(void *data, size_t cnt, size_t elem_size, sort_cmp_t cmp, void *arg)
{
	uint8_t ibuf_slot[IBUF_SIZE];
	void *slot;

	if (elem_size > IBUF_SIZE) {
		slot = (void *) malloc(elem_size);
		if (!slot)
			return false;
	} else
		slot = (void *) ibuf_slot;

	_gsort(data, cnt, elem_size, cmp, arg, slot);

	if (elem_size > IBUF_SIZE)
		free(slot);

	return true;
}

/** @}
 */
