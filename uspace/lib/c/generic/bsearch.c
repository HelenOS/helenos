/*
 * Copyright (c) 2018 Jiri Svoboda
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
