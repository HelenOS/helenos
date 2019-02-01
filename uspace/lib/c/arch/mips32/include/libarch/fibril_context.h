/*
 * Copyright (c) 2014 Jakub Jermar
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

#define __CONTEXT_OFFSET_SP   0x00
#define __CONTEXT_OFFSET_PC   0x04
#define __CONTEXT_OFFSET_S0   0x08
#define __CONTEXT_OFFSET_S1   0x0c
#define __CONTEXT_OFFSET_S2   0x10
#define __CONTEXT_OFFSET_S3   0x14
#define __CONTEXT_OFFSET_S4   0x18
#define __CONTEXT_OFFSET_S5   0x1c
#define __CONTEXT_OFFSET_S6   0x20
#define __CONTEXT_OFFSET_S7   0x24
#define __CONTEXT_OFFSET_S8   0x28
#define __CONTEXT_OFFSET_GP   0x2c
#define __CONTEXT_OFFSET_TLS  0x30
#define __CONTEXT_OFFSET_F20  0x34
#define __CONTEXT_OFFSET_F21  0x38
#define __CONTEXT_OFFSET_F22  0x3c
#define __CONTEXT_OFFSET_F23  0x40
#define __CONTEXT_OFFSET_F24  0x44
#define __CONTEXT_OFFSET_F25  0x48
#define __CONTEXT_OFFSET_F26  0x4c
#define __CONTEXT_OFFSET_F27  0x50
#define __CONTEXT_OFFSET_F28  0x54
#define __CONTEXT_OFFSET_F29  0x58
#define __CONTEXT_OFFSET_F30  0x5c
#define __CONTEXT_SIZE        0x60

#ifndef __ASSEMBLER__

#include <stddef.h>
#include <stdint.h>

typedef struct __context {
	uint32_t sp;
	uint32_t pc;
	uint32_t s0;
	uint32_t s1;
	uint32_t s2;
	uint32_t s3;
	uint32_t s4;
	uint32_t s5;
	uint32_t s6;
	uint32_t s7;
	uint32_t s8;
	uint32_t gp;
	/* Thread local storage (k1) */
	uint32_t tls;
	uint32_t f20;
	uint32_t f21;
	uint32_t f22;
	uint32_t f23;
	uint32_t f24;
	uint32_t f25;
	uint32_t f26;
	uint32_t f27;
	uint32_t f28;
	uint32_t f29;
	uint32_t f30;
} __context_t;

#endif
#endif
