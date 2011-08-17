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

#include <arch/register.h>

#else /* KERNEL */

#include <libarch/register.h>

#endif /* KERNEL */

typedef struct istate {
	uint128_t f2;
	uint128_t f3;
	uint128_t f4;
	uint128_t f5;
	uint128_t f6;
	uint128_t f7;
	uint128_t f8;
	uint128_t f9;
	uint128_t f10;
	uint128_t f11;
	uint128_t f12;
	uint128_t f13;
	uint128_t f14;
	uint128_t f15;
	uint128_t f16;
	uint128_t f17;
	uint128_t f18;
	uint128_t f19;
	uint128_t f20;
	uint128_t f21;
	uint128_t f22;
	uint128_t f23;
	uint128_t f24;
	uint128_t f25;
	uint128_t f26;
	uint128_t f27;
	uint128_t f28;
	uint128_t f29;
	uint128_t f30;
	uint128_t f31;
	
	uintptr_t ar_bsp;
	uintptr_t ar_bspstore;
	uintptr_t ar_bspstore_new;
	uint64_t ar_rnat;
	uint64_t ar_ifs;
	uint64_t ar_pfs;
	uint64_t ar_rsc;
	uintptr_t cr_ifa;
	cr_isr_t cr_isr;
	uintptr_t cr_iipa;
	psr_t cr_ipsr;
	uintptr_t cr_iip;
	uint64_t pr;
	uintptr_t sp;
	
	/*
	 * The following variables are defined only for break_instruction
	 * handler.
	 */
	uint64_t in0;
	uint64_t in1;
	uint64_t in2;
	uint64_t in3;
	uint64_t in4;
	uint64_t in5;
	uint64_t in6;
} istate_t;

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
