/*
 * Copyright (c) 2015 Petr Pavlu
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

#include <abi/asmtool.h>
#include <libarch/fibril_context.h>

.text

/* Static checks for the context_t save/load. */
#if __CONTEXT_OFFSET_SP + 8 != __CONTEXT_OFFSET_PC
#error sp and pc are not successive in context_t
#endif
#if __CONTEXT_OFFSET_X19 + 8 != __CONTEXT_OFFSET_X20
#error x19 and x20 are not successive in context_t
#endif
#if __CONTEXT_OFFSET_X21 + 8 != __CONTEXT_OFFSET_X22
#error x21 and x22 are not successive in context_t
#endif
#if __CONTEXT_OFFSET_X23 + 8 != __CONTEXT_OFFSET_X24
#error x23 and x24 are not successive in context_t
#endif
#if __CONTEXT_OFFSET_X25 + 8 != __CONTEXT_OFFSET_X26
#error x25 and x26 are not successive in context_t
#endif
#if __CONTEXT_OFFSET_X27 + 8 != __CONTEXT_OFFSET_X28
#error x27 and x28 are not successive in context_t
#endif
#if __CONTEXT_OFFSET_X29 + 8 != __CONTEXT_OFFSET_TLS
#error x29 and tls are not successive in context_t
#endif

FUNCTION_BEGIN(__context_save)
	/* Save callee-saved registers into context_t pointed by x0. */
	mov x1, sp
	stp x1, x30, [x0, #__CONTEXT_OFFSET_SP]
	stp x19, x20, [x0, #__CONTEXT_OFFSET_X19]
	stp x21, x22, [x0, #__CONTEXT_OFFSET_X21]
	stp x23, x24, [x0, #__CONTEXT_OFFSET_X23]
	stp x25, x26, [x0, #__CONTEXT_OFFSET_X25]
	stp x27, x28, [x0, #__CONTEXT_OFFSET_X27]
	/* Save the last general-purpose register and TLS. */
	mrs x1, tpidr_el0
	stp x29, x1, [x0, #__CONTEXT_OFFSET_X29]
	/* Save low 64 bits of v8-v15. */
	stp d8, d9, [x0, #__CONTEXT_OFFSET_VREGS + 8 * 0]
	stp d10, d11, [x0, #__CONTEXT_OFFSET_VREGS + 8 * 2]
	stp d12, d13, [x0, #__CONTEXT_OFFSET_VREGS + 8 * 4]
	stp d14, d15, [x0, #__CONTEXT_OFFSET_VREGS + 8 * 6]

	/* Return 0. */
	mov x0, #0
	ret
FUNCTION_END(__context_save)

FUNCTION_BEGIN(__context_restore)
	/* Restore callee-saved registers from context_t pointed by x0. */
	ldp x2, x30, [x0, #__CONTEXT_OFFSET_SP]
	mov sp, x2
	ldp x19, x20, [x0, #__CONTEXT_OFFSET_X19]
	ldp x21, x22, [x0, #__CONTEXT_OFFSET_X21]
	ldp x23, x24, [x0, #__CONTEXT_OFFSET_X23]
	ldp x25, x26, [x0, #__CONTEXT_OFFSET_X25]
	ldp x27, x28, [x0, #__CONTEXT_OFFSET_X27]
	/* Restore the last general-purpose register and TLS. */
	ldp x29, x2, [x0, #__CONTEXT_OFFSET_X29]
	msr tpidr_el0, x2
	/* Restore low 64 bits of v8-v15. */
	ldp d8, d9, [x0, #__CONTEXT_OFFSET_VREGS + 8 * 0]
	ldp d10, d11, [x0, #__CONTEXT_OFFSET_VREGS + 8 * 2]
	ldp d12, d13, [x0, #__CONTEXT_OFFSET_VREGS + 8 * 4]
	ldp d14, d15, [x0, #__CONTEXT_OFFSET_VREGS + 8 * 6]

	/* Return the second argument. */
	mov x0, x1
	ret
FUNCTION_END(__context_restore)
