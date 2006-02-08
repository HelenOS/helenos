/*
 * Copyright (C) 2005 Sergey Bondari
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

#include <mm/slab.h>
#include <memstr.h>
#include <sort.h>
#include <panic.h>

#define EBUFSIZE	32

void _qsort(void * data, count_t n, size_t e_size, int (* cmp) (void * a, void * b), void *tmp, void *pivot);
void _bubblesort(void * data, count_t n, size_t e_size, int (* cmp) (void * a, void * b), void *slot);

/** Quicksort wrapper
 *
 * This is only a wrapper that takes care of memory allocations for storing
 * the pivot and temporary elements for generic quicksort algorithm.
 * 
 * @param data Pointer to data to be sorted.
 * @param n Number of elements to be sorted.
 * @param e_size Size of one element.
 * @param cmp Comparator function.
 * 
 */
void qsort(void * data, count_t n, size_t e_size, int (* cmp) (void * a, void * b))
{
	__u8 buf_tmp[EBUFSIZE];
	__u8 buf_pivot[EBUFSIZE];
	void * tmp = buf_tmp;
	void * pivot = buf_pivot;

	if (e_size > EBUFSIZE) {
		pivot = (void *) malloc(e_size);
		tmp = (void *) malloc(e_size);
	
		if (!tmp || !pivot) {
			panic("Cannot allocate memory\n");
		}
	}

	_qsort(data, n, e_size, cmp, tmp, pivot);
	
	if (e_size > EBUFSIZE) {
		free(tmp);
		free(pivot);
	}
}

/** Quicksort
 *
 * Apply generic quicksort algorithm on supplied data, using pre-allocated buffers.
 * 
 * @param data Pointer to data to be sorted.
 * @param n Number of elements to be sorted.
 * @param e_size Size of one element.
 * @param cmp Comparator function.
 * @param tmp Pointer to scratch memory buffer e_size bytes long.
 * @param pivot Pointer to scratch memory buffer e_size bytes long.
 * 
 */
void _qsort(void * data, count_t n, size_t e_size, int (* cmp) (void * a, void * b), void *tmp, void *pivot)
{
	if (n > 4) {
		int i = 0, j = n - 1;

		memcpy(pivot, data, e_size);

		while (1) {
			while ((cmp(data + i * e_size, pivot) < 0) && i < n) i++;
			while ((cmp(data + j * e_size, pivot) >=0) && j > 0) j--;
			if (i<j) {
				memcpy(tmp, data + i * e_size, e_size);
				memcpy(data + i * e_size, data + j * e_size, e_size);
				memcpy(data + j * e_size, tmp, e_size);
			} else {
				break;
			}
		}

		_qsort(data, j + 1, e_size, cmp, tmp, pivot);
		_qsort(data + (j + 1) * e_size, n - j - 1, e_size, cmp, tmp, pivot);
	} else {
		_bubblesort(data, n, e_size, cmp, tmp);
	}
}

/** Bubblesort wrapper
 *
 * This is only a wrapper that takes care of memory allocation for storing
 * the slot element for generic bubblesort algorithm.
 * 
 * @param data Pointer to data to be sorted.
 * @param n Number of elements to be sorted.
 * @param e_size Size of one element.
 * @param cmp Comparator function.
 * 
 */
void bubblesort(void * data, count_t n, size_t e_size, int (* cmp) (void * a, void * b))
{
	__u8 buf_slot[EBUFSIZE];
	void * slot = buf_slot;
	
	if (e_size > EBUFSIZE) {
		slot = (void *) malloc(e_size);
		
		if (!slot) {
			panic("Cannot allocate memory\n");
		}
	}

	_bubblesort(data, n, e_size, cmp, slot);
	
	if (e_size > EBUFSIZE) {
		free(slot);
	}
}

/** Bubblesort
 *
 * Apply generic bubblesort algorithm on supplied data, using pre-allocated buffer.
 * 
 * @param data Pointer to data to be sorted.
 * @param n Number of elements to be sorted.
 * @param e_size Size of one element.
 * @param cmp Comparator function.
 * @param slot Pointer to scratch memory buffer e_size bytes long.
 * 
 */
void _bubblesort(void * data, count_t n, size_t e_size, int (* cmp) (void * a, void * b), void *slot)
{
	bool done = false;
	void * p;

	while (!done) {
		done = true;
		for (p = data; p < data + e_size * (n - 1); p = p + e_size) {
			if (cmp(p, p + e_size) == 1) {
				memcpy(slot, p, e_size);
				memcpy(p, p + e_size, e_size);
				memcpy(p + e_size, slot, e_size);
				done = false;
			}
		}
	}

}

/*
 * Comparator returns 1 if a > b, 0 if a == b, -1 if a < b
 */
int int_cmp(void * a, void * b)
{
	return (* (int *) a > * (int*)b) ? 1 : (*(int *)a < * (int *)b) ? -1 : 0;
}

int __u8_cmp(void * a, void * b)
{
	return (* (__u8 *) a > * (__u8 *)b) ? 1 : (*(__u8 *)a < * (__u8 *)b) ? -1 : 0;
}

int __u16_cmp(void * a, void * b)
{
	return (* (__u16 *) a > * (__u16 *)b) ? 1 : (*(__u16 *)a < * (__u16 *)b) ? -1 : 0;
}

int __u32_cmp(void * a, void * b)
{
	return (* (__u32 *) a > * (__u32 *)b) ? 1 : (*(__u32 *)a < * (__u32 *)b) ? -1 : 0;
}
