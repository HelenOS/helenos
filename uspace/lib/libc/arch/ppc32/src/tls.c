/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup libcppc32	
 * @{
 */
/** @file
 */

#include <tls.h>
#include <align.h>
#include <sys/types.h>

tcb_t * __alloc_tls(void **data, size_t size)
{
	return tls_alloc_variant_1(data, size);
}

void __free_tls_arch(tcb_t *tcb, size_t size)
{
	tls_free_variant_1(tcb, size);
}

static void kputint(unsigned i)
{
	asm volatile (
		"mr %%r3, %0\n"
		"li %%r9, 32\n"
		"sc\n"
		:
		: "r" (i)
		: "%r3","%r9"
	) ;
}

typedef struct {
	unsigned long int ti_module;
	unsigned long int ti_offset;
} tls_index;

void *__tls_get_addr(tls_index *ti);

/* ppc32 uses TLS variant 1 */
void *__tls_get_addr(tls_index *ti)
{
	uint8_t *tls;

	/* The TLS section is just after TCB */
	tls = (uint8_t *)__tcb_get() + sizeof(tcb_t);

	/* Hopefully this is right. No docs found. */
	return tls + ti->ti_offset + 32768;
}

/** @}
 */
