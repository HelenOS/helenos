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

/** @addtogroup ia32interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_ISTATE_H_
#define KERN_ia32_ISTATE_H_

#include <trace.h>

typedef struct istate {
	/*
	 * The strange order of the GPRs is given by the requirement to use the
	 * istate structure for both regular interrupts and exceptions as well
	 * as for syscall handlers which use this order as an optimization.
	 */
	uint32_t edx;
	uint32_t ecx;
	uint32_t ebx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
	uint32_t eax;
	
	uint32_t ebp_frame;  /* imitation of frame pointer linkage */
	uint32_t eip_frame;  /* imitation of return address linkage */
	
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	
	uint32_t error_word;  /* real or fake error word */
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t esp;         /* only if istate_t is from uspace */
	uint32_t ss;          /* only if istate_t is from uspace */
} istate_t;

/** Return true if exception happened while in userspace */
NO_TRACE static inline int istate_from_uspace(istate_t *istate)
{
	return !(istate->eip & UINT32_C(0x80000000));
}

NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
{
	istate->eip = retaddr;
}

NO_TRACE static inline uintptr_t istate_get_pc(istate_t *istate)
{
	return istate->eip;
}

NO_TRACE static inline uintptr_t istate_get_fp(istate_t *istate)
{
	return istate->ebp;
}

#endif

/** @}
 */
