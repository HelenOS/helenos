/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <balloc.h>
#include <stdalign.h>
#include <stddef.h>
#include <align.h>

static ballocs_t *ballocs;
static uintptr_t phys_base;
static size_t max_size;

void balloc_init(ballocs_t *ball, void *base, uintptr_t kernel_base,
    size_t size)
{
	ballocs = ball;
	phys_base = (uintptr_t) base;
	max_size = size;
	ballocs->base = kernel_base;
	ballocs->size = 0;
}

void *balloc(size_t size, size_t alignment)
{
	if (alignment == 0)
		return NULL;

	/* Enforce minimal alignment. */
	alignment = ALIGN_UP(alignment, alignof(max_align_t));

	uintptr_t addr = phys_base + ALIGN_UP(ballocs->size, alignment);

	if (ALIGN_UP(ballocs->size, alignment) + size >= max_size)
		return NULL;

	ballocs->size = ALIGN_UP(ballocs->size, alignment) + size;

	return (void *) addr;
}

void *balloc_rebase(void *ptr)
{
	return (void *) (((uintptr_t) ptr - phys_base) + ballocs->base);
}
