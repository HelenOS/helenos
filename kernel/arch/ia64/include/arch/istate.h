/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup ia64interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_ISTATE_H_
#define KERN_ia64_ISTATE_H_

#include <trace.h>

#ifdef KERNEL

#include <arch/istate_struct.h>
#include <arch/register.h>

#else /* KERNEL */

#include <libarch/istate_struct.h>
#include <libarch/register.h>

#endif /* KERNEL */

NO_TRACE static inline void istate_set_retaddr(istate_t *istate,
    uintptr_t retaddr)
{
	istate->cr_iip = retaddr;
	istate->cr_ipsr.ri = 0;    /* return to instruction slot #0 */
}

NO_TRACE static inline uintptr_t istate_get_pc(istate_t *istate)
{
	return istate->cr_iip;
}

NO_TRACE static inline uintptr_t istate_get_fp(istate_t *istate)
{
	/* FIXME */

	return 0;
}

NO_TRACE static inline int istate_from_uspace(istate_t *istate)
{
	return (istate->cr_iip) < 0xe000000000000000ULL;
}

#endif

/** @}
 */
