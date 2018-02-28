/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup genericadt
 * @{
 */
/** @file
 */

#ifndef KERN_BITMAP_H_
#define KERN_BITMAP_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define BITMAP_ELEMENT   8
#define BITMAP_REMAINER  7

typedef struct {
	size_t elements;
	uint8_t *bits;
	size_t next_fit;
} bitmap_t;

static inline void bitmap_set(bitmap_t *bitmap, size_t element,
    unsigned int value)
{
	if (element >= bitmap->elements)
		return;

	size_t byte = element / BITMAP_ELEMENT;
	uint8_t mask = 1 << (element & BITMAP_REMAINER);

	if (value) {
		bitmap->bits[byte] |= mask;
	} else {
		bitmap->bits[byte] &= ~mask;
		bitmap->next_fit = byte;
	}
}

static inline unsigned int bitmap_get(bitmap_t *bitmap, size_t element)
{
	if (element >= bitmap->elements)
		return 0;

	size_t byte = element / BITMAP_ELEMENT;
	uint8_t mask = 1 << (element & BITMAP_REMAINER);

	return !!((bitmap->bits)[byte] & mask);
}

extern size_t bitmap_size(size_t);
extern void bitmap_initialize(bitmap_t *, size_t, void *);

extern void bitmap_set_range(bitmap_t *, size_t, size_t);
extern void bitmap_clear_range(bitmap_t *, size_t, size_t);

extern bool bitmap_allocate_range(bitmap_t *, size_t, size_t, size_t, size_t,
    size_t *);
extern void bitmap_copy(bitmap_t *, bitmap_t *, size_t);

#endif

/** @}
 */
