/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup amd64interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_ISTATE_H_
#define KERN_amd64_ISTATE_H_

#include <trace.h>

/** This is passed to interrupt handlers */
typedef struct istate {
	uint64_t rax;
	uint64_t rbx;
	uint64_t rcx;
	uint64_t rdx;
	uint64_t rsi;
	uint64_t rdi;
	uint64_t rbp;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t alignment;   /* align rbp_frame on multiple of 16 */
	uint64_t rbp_frame;   /* imitation of frame pointer linkage */
	uint64_t rip_frame;   /* imitation of return address linkage */
	uint64_t error_word;  /* real or fake error word */
	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;         /* only if istate_t is from uspace */
	uint64_t ss;          /* only if istate_t is from uspace */
} istate_t;

/** Return true if exception happened while in userspace */
NO_TRACE static inline int istate_from_uspace(istate_t *istate)
{
	return !(istate->rip & UINT64_C(0x8000000000000000));
}

NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
{
	istate->rip = retaddr;
}

NO_TRACE static inline uintptr_t istate_get_pc(istate_t *istate)
{
	return istate->rip;
}

NO_TRACE static inline uintptr_t istate_get_fp(istate_t *istate)
{
	return istate->rbp;
}

#endif

/** @}
 */
