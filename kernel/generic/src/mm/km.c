/*
 * Copyright (c) 2011 Jakub Jermar
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

/** @addtogroup genericmm
 * @{
 */

/**
 * @file
 * @brief Kernel virtual memory setup.
 */

#include <mm/km.h>
#include <arch/mm/km.h>
#include <config.h>
#include <typedefs.h>
#include <lib/ra.h>
#include <debug.h>

static ra_arena_t *km_ni_arena;

/** Architecture dependent setup of identity-mapped kernel memory. */
void km_identity_init(void)
{
	km_identity_arch_init();
	config.identity_configured = true;
}

/** Architecture dependent setup of non-identity-mapped kernel memory. */
void km_non_identity_init(void)
{
	km_ni_arena = ra_arena_create();
	ASSERT(km_ni_arena != NULL);
	km_non_identity_arch_init();
	config.non_identity_configured = true;
}

bool km_is_non_identity(uintptr_t addr)
{
	return km_is_non_identity_arch(addr);
}

void km_non_identity_span_add(uintptr_t base, size_t size)
{
	bool span_added;

	span_added = ra_span_add(km_ni_arena, base, size);
	ASSERT(span_added);
}

uintptr_t km_page_alloc(size_t size, size_t align)
{
	return ra_alloc(km_ni_arena, size, align);
}

void km_page_free(uintptr_t page, size_t size)
{
	ra_free(km_ni_arena, page, size);
}


/** @}
 */

