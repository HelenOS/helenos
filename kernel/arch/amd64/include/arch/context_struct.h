/* Copyright (c) 2014 Jakub Jermar
 * All rights preserved.
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

#ifndef KERN_ARCH_CONTEXT_STRUCT_H_
#define KERN_ARCH_CONTEXT_STRUCT_H_

#define CONTEXT_OFFSET_SP   0x00
#define CONTEXT_OFFSET_PC   0x08
#define CONTEXT_OFFSET_RBX  0x10
#define CONTEXT_OFFSET_RBP  0x18
#define CONTEXT_OFFSET_R12  0x20
#define CONTEXT_OFFSET_R13  0x28
#define CONTEXT_OFFSET_R14  0x30
#define CONTEXT_OFFSET_R15  0x38
#define CONTEXT_OFFSET_TP   0x40
#define CONTEXT_OFFSET_IPL  0x48
#define CONTEXT_SIZE        0x50

#ifndef __ASSEMBLER__

#include <typedefs.h>

typedef struct context {
	/* We include only registers that must be preserved
	 * during function call.
	 */
	uint64_t sp;
	uint64_t pc;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t tp;
	ipl_t ipl;
} context_t;

#endif
#endif

