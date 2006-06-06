/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __ia64_INTERRUPT_H__
#define __ia64_INTERRUPT_H__

#include <typedefs.h>
#include <arch/types.h>
#include <arch/register.h>

#define IRQ_COUNT               257 /* 256 NOT suppotred IRQS*//* TODO */
#define IRQ_KBD                 256 /* One simulated interrupt for ski simulator keyboard*/

/** External Interrupt vectors. */
#define INTERRUPT_TIMER		0
#define INTERRUPT_SPURIOUS	15

/** General Exception codes. */
#define GE_ILLEGALOP		0
#define GE_PRIVOP		1
#define GE_PRIVREG		2
#define GE_RESREGFLD		3
#define GE_DISBLDISTRAN		4
#define GE_ILLEGALDEP		8

#define EOI	0		/**< The actual value doesn't matter. */

struct istate {
	__r128 f2;
	__r128 f3;
	__r128 f4;
	__r128 f5;
	__r128 f6;
	__r128 f7;
	__r128 f8;
	__r128 f9;
	__r128 f10;
	__r128 f11;
	__r128 f12;
	__r128 f13;
	__r128 f14;
	__r128 f15;
	__r128 f16;
	__r128 f17;
	__r128 f18;
	__r128 f19;
	__r128 f20;
	__r128 f21;
	__r128 f22;
	__r128 f23;
	__r128 f24;
	__r128 f25;
	__r128 f26;
	__r128 f27;
	__r128 f28;
	__r128 f29;
	__r128 f30;
	__r128 f31;
		
	__address ar_bsp;
	__address ar_bspstore;
	__address ar_bspstore_new;
	__u64 ar_rnat;
	__u64 ar_ifs;
	__u64 ar_pfs;
	__u64 ar_rsc;
	__address cr_ifa;
	cr_isr_t cr_isr;
	__address cr_iipa;
	psr_t cr_ipsr;
	__address cr_iip;
	__u64 pr;
	__address sp;
	
	/*
	 * The following variables are defined only for break_instruction handler. 
	 */
	__u64 in0;
	__u64 in1;
	__u64 in2;
	__u64 in3;
	__u64 in4;
};

static inline void istate_set_retaddr(istate_t *istate, __address retaddr)
{
	istate->cr_iip = retaddr;
	istate->cr_ipsr.ri = 0;		/* return to instruction slot #0 */
}

static inline __native istate_get_pc(istate_t *istate)
{
	return istate->cr_iip;
}
#include <panic.h>
static inline int istate_from_uspace(istate_t *istate)
{
	return (istate->cr_iip)<0xe000000000000000ULL;
}

extern void *ivt;

extern void general_exception(__u64 vector, istate_t *istate);
extern int break_instruction(__u64 vector, istate_t *istate);
extern void universal_handler(__u64 vector, istate_t *istate);
extern void nop_handler(__u64 vector, istate_t *istate);
extern void external_interrupt(__u64 vector, istate_t *istate);
extern void virtual_interrupt(__u64 irq, void *param);
extern void disabled_fp_register(__u64 vector, istate_t *istate);



#endif
