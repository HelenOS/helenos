/*
 * Copyright (c) 2010 Martin Decky
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

/** @addtogroup abs32leinterrupt
 * @{
 */
/** @file
 */

#ifndef KERN_abs32le_ISTATE_H_
#define KERN_abs32le_ISTATE_H_

#include <trace.h>

#ifdef KERNEL

#include <typedefs.h>
#include <verify.h>

#else /* KERNEL */

#include <stdint.h>

#define REQUIRES_EXTENT_MUTABLE(arg)
#define WRITES(arg)

#endif /* KERNEL */

/*
 * On real hardware this stores the registers which
 * need to be preserved during interupts.
 */
typedef struct istate {
	uintptr_t ip;
	uintptr_t fp;
	uint32_t stack[];
} istate_t;

NO_TRACE static inline int istate_from_uspace(istate_t *istate)
    REQUIRES_EXTENT_MUTABLE(istate)
{
	/* On real hardware this checks whether the interrupted
	   context originated from user space. */

	return !(istate->ip & UINT32_C(0x80000000));
}

NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
    WRITES(&istate->ip)
{
	/* On real hardware this sets the instruction pointer. */

	istate->ip = retaddr;
}

NO_TRACE static inline uintptr_t istate_get_pc(istate_t *istate)
    REQUIRES_EXTENT_MUTABLE(istate)
{
	/* On real hardware this returns the instruction pointer. */

	return istate->ip;
}

NO_TRACE static inline uintptr_t istate_get_fp(istate_t *istate)
    REQUIRES_EXTENT_MUTABLE(istate)
{
	/* On real hardware this returns the frame pointer. */

	return istate->fp;
}

#endif

/** @}
 */
