/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_adt
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
