/*
 * Copyright (C) 2005 Jakub Vana
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
 *
 */

#include <fpu_context.h>
#include <arch.h>
#include <cpu.h>

void fpu_context_save(fpu_context_t *fctx)
{
	/* Align on 16-byte boundary */
	if (((__u64)fctx) & 0xf)
		fctx = (fpu_context_t *)((((__u64)fctx) | 0xf) + 1);

	__asm__ volatile (
		"fxsave %0"
		: "=m"(*fctx)
		);
}

void fpu_context_restore(fpu_context_t *fctx)
{
	/* TODO: We need malloc that allocates on 16-byte boundary !! */
	if (((__u64)fctx) & 0xf)
		fctx = (fpu_context_t *)((((__u64)fctx) | 0xf) + 1);
	__asm__ volatile (
		"fxrstor %0"
		: "=m"(*fctx)
		);
}

void fpu_init(void)
{
	__asm__ volatile (
		"fninit;"
	);
}
