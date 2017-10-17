/*
 * Copyright (c) 2017 Jiri Svoboda
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
 * @brief Quicksort.
 */

#include <qsort.h>
#include <stdbool.h>
#include <stddef.h>

/** Quicksort spec */
typedef struct {
	void *base;
	size_t nmemb;
	size_t size;
	int (*compar)(const void *, const void *, void *);
	void *arg;
} qs_spec_t;

/** Comparison function wrapper.
 *
 * Performs qsort_r comparison using qsort comparison function
 *
 * @param a First element
 * @param b Second element
 * @param arg qsort comparison function
 */
static int compar_wrap(const void *a, const void *b, void *arg)
{
	int (*compar)(const void *, const void *) =
	    (int (*)(const void *, const void *))arg;

	return compar(a, b);
}

/** Determine if one element is less-than another element.
 *
 * @param qs Quicksort spec
 * @param i First element index
 * @param j Second element index
 */
static bool elem_lt(qs_spec_t *qs, size_t i, size_t j)
{
	const void *a;
	const void *b;
	int r;

	a = qs->base + i * qs->size;
	b = qs->base + j * qs->size;

	r = qs->compar(a, b, qs->arg);

	return r < 0;
}

/** Swap two elements.
 *
 * @param qs Quicksort spec
 * @param i First element index
 * @param j Second element index
 */
static void elem_swap(qs_spec_t *qs, size_t i, size_t j)
{
	char *a;
	char *b;
	char t;
	size_t k;

	a = qs->base + i * qs->size;
	b = qs->base + j * qs->size;

	for (k = 0; k < qs->size; k++) {
		t = a[k];
		a[k] = b[k];
		b[k] = t;
	}
}

/** Partition a range of indices.
 *
 * @param qs Quicksort spec
 * @param lo Lower bound (inclusive)
 * @param hi Upper bound (inclusive)
 * @return Pivot index
 */
static size_t partition(qs_spec_t *qs, size_t lo, size_t hi)
{
	size_t pivot;
	size_t i, j;

	pivot = lo + (hi - lo) / 2;
	i = lo;
	j = hi;
	while (true) {
		while (elem_lt(qs, i, pivot))
			++i;
		while (elem_lt(qs, pivot, j))
			--j;

		if (i >= j)
			return j;

		elem_swap(qs, i, j);

		/* Need to follow pivot movement */
		if (i == pivot)
			pivot = j;
		else if (j == pivot)
			pivot = i;

		i++;
		j--;
	}
}

/** Sort a range of indices.
 *
 * @param qs Quicksort spec
 * @param lo Lower bound (inclusive)
 * @param hi Upper bound (inclusive)
 */
static void quicksort(qs_spec_t *qs, size_t lo, size_t hi)
{
	size_t p;

	if (lo < hi) {
		p = partition(qs, lo, hi);
		quicksort(qs, lo, p);
		quicksort(qs, p + 1, hi);
	}
}

/** Quicksort.
 *
 * @param base Array to sort
 * @param nmemb Number of array members
 * @param size Size of member in bytes
 * @param compar Comparison function
 */
void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *,
    const void *))
{
	qs_spec_t qs;

	if (nmemb == 0)
		return;

	qs.base = base;
	qs.nmemb = nmemb;
	qs.size = size;
	qs.compar = compar_wrap;
	qs.arg = compar;

	quicksort(&qs, 0, nmemb - 1);
}

/** Quicksort with extra argument to comparison function.
 *
 * @param base Array to sort
 * @param nmemb Number of array members
 * @param size Size of member in bytes
 * @param compar Comparison function
 * @param arg Argument to comparison function
 */
void qsort_r(void *base, size_t nmemb, size_t size, int (*compar)(const void *,
    const void *, void *), void *arg)
{
	qs_spec_t qs;

	if (nmemb == 0)
		return;

	qs.base = base;
	qs.nmemb = nmemb;
	qs.size = size;
	qs.compar = compar;
	qs.arg = arg;

	quicksort(&qs, 0, nmemb - 1);
}

/** @}
 */
