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
 * algorithms (e.g. quick sort and bubble sort).
 *
 */

#include <sort.h>
#include <mem.h>
#include <malloc.h>

/** Bubble sort
 *
 * Apply generic bubble sort algorithm on supplied data,
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
static void _bsort(void *data, size_t cnt, size_t elem_size, sort_cmp_t cmp,
    void *arg, void *slot)
{
	bool done = false;
	void *tmp;
	
	while (!done) {
		done = true;
		
		for (tmp = data; tmp < data + elem_size * (cnt - 1);
		    tmp = tmp + elem_size) {
			if (cmp(tmp, tmp + elem_size, arg) == 1) {
				memcpy(slot, tmp, elem_size);
				memcpy(tmp, tmp + elem_size, elem_size);
				memcpy(tmp + elem_size, slot, elem_size);
				done = false;
			}
		}
	}
}

/** Quicksort
 *
 * Apply generic quicksort algorithm on supplied data,
 * using pre-allocated buffers.
 *
 * @param data      Pointer to data to be sorted.
 * @param cnt       Number of elements to be sorted.
 * @param elem_size Size of one element.
 * @param cmp       Comparator function.
 * @param arg       3rd argument passed to cmp.
 * @param tmp       Pointer to scratch memory buffer
 *                  elem_size bytes long.
 * @param pivot     Pointer to scratch memory buffer
 *                  elem_size bytes long.
 *
 */
static void _qsort(void *data, size_t cnt, size_t elem_size, sort_cmp_t cmp,
    void *arg, void *tmp, void *pivot)
{
	if (cnt > 4) {
		size_t i = 0;
		size_t j = cnt - 1;
		
		memcpy(pivot, data, elem_size);
		
		while (true) {
			while ((cmp(data + i * elem_size, pivot, arg) < 0) && (i < cnt))
				i++;
			
			while ((cmp(data + j * elem_size, pivot, arg) >= 0) && (j > 0))
				j--;
			
			if (i < j) {
				memcpy(tmp, data + i * elem_size, elem_size);
				memcpy(data + i * elem_size, data + j * elem_size,
				    elem_size);
				memcpy(data + j * elem_size, tmp, elem_size);
			} else
				break;
		}
		
		_qsort(data, j + 1, elem_size, cmp, arg, tmp, pivot);
		_qsort(data + (j + 1) * elem_size, cnt - j - 1, elem_size,
		    cmp, arg, tmp, pivot);
	} else
		_bsort(data, cnt, elem_size, cmp, arg, tmp);
}

/** Bubble sort wrapper
 *
 * This is only a wrapper that takes care of memory
 * allocations for storing the slot element for generic
 * bubble sort algorithm.
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
bool bsort(void *data, size_t cnt, size_t elem_size, sort_cmp_t cmp, void *arg)
{
	void *slot = (void *) malloc(elem_size);
	if (!slot)
		return false;
	
	_bsort(data, cnt, elem_size, cmp, arg, slot);
	
	free(slot);
	
	return true;
}

/** Quicksort wrapper
 *
 * This is only a wrapper that takes care of memory
 * allocations for storing the pivot and temporary elements
 * for generic quicksort algorithm.
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
bool qsort(void *data, size_t cnt, size_t elem_size, sort_cmp_t cmp, void *arg)
{
	void *tmp = (void *) malloc(elem_size);
	if (!tmp)
		return false;
	
	void *pivot = (void *) malloc(elem_size);
	if (!pivot) {
		free(tmp);
		return false;
	}
	
	_qsort(data, cnt, elem_size, cmp, arg, tmp, pivot);
	
	free(pivot);
	free(tmp);
	
	return true;
}

/** @}
 */
