/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */

/**
 * @file
 * @brief Binary search.
 */

#include <bsearch.h>
#include <stddef.h>

/** Binary search.
 *
 * @param key Key to search for
 * @param base Array of objects
 * @param nmemb Number of objects in array
 * @param size Size of each object
 * @param compar Comparison function
 */
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
    int (*compar)(const void *, const void *))
{
	size_t pividx;
	const void *pivot;
	int r;

	while (nmemb != 0) {
		pividx = nmemb / 2;
		pivot = base + size * pividx;

		r = compar(key, pivot);
		if (r == 0)
			return (void *)pivot;

		if (r < 0) {
			/* Now only look at members preceding pivot */
			nmemb = pividx;
		} else {
			/* Now only look at members following pivot */
			nmemb = nmemb - pividx - 1;
			base += size * (pividx + 1);
		}
	}

	return NULL;
}

/** @}
 */
