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
#define CONTEXT_OFFSET_PC   0x04
#define CONTEXT_OFFSET_EBX  0x08
#define CONTEXT_OFFSET_ESI  0x0c
#define CONTEXT_OFFSET_EDI  0x10
#define CONTEXT_OFFSET_EBP  0x14
#define CONTEXT_OFFSET_TP   0x18
#define CONTEXT_OFFSET_IPL  0x1c
#define CONTEXT_SIZE        0x20

#ifndef __ASSEMBLER__

#include <typedefs.h>

/* Only save registers that must be preserved across function calls. */
typedef struct context {
	uint32_t sp;
	uint32_t pc;
	uint32_t ebx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
	uint32_t tp;
	ipl_t ipl;
} context_t;

#endif
#endif

