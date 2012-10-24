/*
 * Copyright (c) 2005 Ondrej Palkovsky
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

#ifndef KERN_amd64_CONTEXT_OFFSET_H_
#define KERN_amd64_CONTEXT_OFFSET_H_

#define OFFSET_SP   0x00
#define OFFSET_PC   0x08
#define OFFSET_RBX  0x10
#define OFFSET_RBP  0x18
#define OFFSET_R12  0x20
#define OFFSET_R13  0x28
#define OFFSET_R14  0x30
#define OFFSET_R15  0x38

#ifdef KERNEL
	#define OFFSET_IPL  0x40
#else
	#define OFFSET_TLS  0x40
#endif

#ifdef __ASM__

# ctx: address of the structure with saved context
# pc: return address
.macro CONTEXT_SAVE_ARCH_CORE ctx:req pc:req
	movq \pc, OFFSET_PC(\ctx)
	movq %rsp, OFFSET_SP(\ctx)
	
	movq %rbx, OFFSET_RBX(\ctx)
	movq %rbp, OFFSET_RBP(\ctx)
	movq %r12, OFFSET_R12(\ctx)
	movq %r13, OFFSET_R13(\ctx)
	movq %r14, OFFSET_R14(\ctx)
	movq %r15, OFFSET_R15(\ctx)
.endm

# ctx: address of the structure with saved context
.macro CONTEXT_RESTORE_ARCH_CORE ctx:req pc:req
	movq OFFSET_R15(\ctx), %r15
	movq OFFSET_R14(\ctx), %r14
	movq OFFSET_R13(\ctx), %r13
	movq OFFSET_R12(\ctx), %r12
	movq OFFSET_RBP(\ctx), %rbp
	movq OFFSET_RBX(\ctx), %rbx
	
	movq OFFSET_SP(\ctx), %rsp   # ctx->sp -> %rsp
	
	movq OFFSET_PC(\ctx), \pc
.endm

#endif

#endif
