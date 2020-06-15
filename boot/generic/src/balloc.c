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
