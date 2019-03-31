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

#ifndef _LIBC_ARCH_FIBRIL_CONTEXT_H_
#define _LIBC_ARCH_FIBRIL_CONTEXT_H_

#define __CONTEXT_OFFSET_SP     0x00
#define __CONTEXT_OFFSET_PC     0x08
#define __CONTEXT_OFFSET_X19    0x10
#define __CONTEXT_OFFSET_X20    0x18
#define __CONTEXT_OFFSET_X21    0x20
#define __CONTEXT_OFFSET_X22    0x28
#define __CONTEXT_OFFSET_X23    0x30
#define __CONTEXT_OFFSET_X24    0x38
#define __CONTEXT_OFFSET_X25    0x40
#define __CONTEXT_OFFSET_X26    0x48
#define __CONTEXT_OFFSET_X27    0x50
#define __CONTEXT_OFFSET_X28    0x58
#define __CONTEXT_OFFSET_X29    0x60
#define __CONTEXT_OFFSET_TLS    0x68
#define __CONTEXT_OFFSET_VREGS  0x70
#define __CONTEXT_SIZE          0xb0

#ifndef __ASSEMBLER__

#include <stdint.h>

typedef struct __context {
	uint64_t sp;
	uint64_t pc;
	uint64_t x19;
	uint64_t x20;
	uint64_t x21;
	uint64_t x22;
	uint64_t x23;
	uint64_t x24;
	uint64_t x25;
	uint64_t x26;
	uint64_t x27;
	uint64_t x28;
	uint64_t x29;
	uint64_t tls;
	/* v8-v15, low 64 bits only. */
	uint64_t vregs[8];
} __context_t;

#endif
#endif
