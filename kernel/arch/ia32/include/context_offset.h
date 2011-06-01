/*
 * Copyright (c) 2008 Josef Cejka
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

/** @addtogroup ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_CONTEXT_OFFSET_H_
#define KERN_ia32_CONTEXT_OFFSET_H_

#define OFFSET_SP   0x00
#define OFFSET_PC   0x04
#define OFFSET_EBX  0x08
#define OFFSET_ESI  0x0C
#define OFFSET_EDI  0x10
#define OFFSET_EBP  0x14

#ifdef KERNEL
	#define OFFSET_IPL  0x18
#else
	#define OFFSET_TLS  0x18
#endif

#ifdef __ASM__

# ctx: address of the structure with saved context
# pc: return address

.macro CONTEXT_SAVE_ARCH_CORE ctx:req pc:req
	movl %esp,OFFSET_SP(\ctx)	# %esp -> ctx->sp
	movl \pc,OFFSET_PC(\ctx)	# %eip -> ctx->pc
	movl %ebx,OFFSET_EBX(\ctx)	# %ebx -> ctx->ebx
	movl %esi,OFFSET_ESI(\ctx)	# %esi -> ctx->esi
	movl %edi,OFFSET_EDI(\ctx)	# %edi -> ctx->edi
	movl %ebp,OFFSET_EBP(\ctx)	# %ebp -> ctx->ebp
.endm

# ctx: address of the structure with saved context

.macro CONTEXT_RESTORE_ARCH_CORE ctx:req pc:req
	movl OFFSET_SP(\ctx),%esp	# ctx->sp -> %esp
	movl OFFSET_PC(\ctx),\pc	# ctx->pc -> \pc
	movl OFFSET_EBX(\ctx),%ebx	# ctx->ebx -> %ebx
	movl OFFSET_ESI(\ctx),%esi	# ctx->esi -> %esi
	movl OFFSET_EDI(\ctx),%edi	# ctx->edi -> %edi
	movl OFFSET_EBP(\ctx),%ebp	# ctx->ebp -> %ebp
.endm

#endif /* __ASM__ */

#endif

/** @}
 */
