/*
 * Copyright (C) 2005 Jakub Jermar
 * Copyright (C) 2005 Jakub Vana
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
 *
 */

#include <arch/interrupt.h>
#include <panic.h>
#include <print.h>
#include <arch/types.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <arch/register.h>
#include <arch/drivers/it.h>
#include <arch.h>
#include <symtab.h>
#include <debug.h>

#define VECTORS_64_BUNDLE	20
#define VECTORS_16_BUNDLE	48
#define VECTORS_16_BUNDLE_START	0x5000
#define VECTOR_MAX		0x7f00

#define BUNDLE_SIZE		16

char *vector_names_64_bundle[VECTORS_64_BUNDLE] = {
	"VHPT Translation vector",
	"Instruction TLB vector",
	"Data TLB vector",
	"Alternate Instruction TLB vector",
	"Alternate Data TLB vector",
	"Data Nested TLB vector",
	"Instruction Key Miss vector",
	"Data Key Miss vector",
	"Dirty-Bit vector",
	"Instruction Access-Bit vector",
	"Data Access-Bit vector"
	"Break Instruction vector",
	"External Interrupt vector"
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"
};

char *vector_names_16_bundle[VECTORS_16_BUNDLE] = {
	"Page Not Present vector",
	"Key Permission vector",
	"Instruction Access rights vector",
	"Data Access Rights vector",
	"General Exception vector",
	"Disabled FP-Register vector",
	"NaT Consumption vector",
	"Speculation vector",
	"Reserved",
	"Debug vector",
	"Unaligned Reference vector",
	"Unsupported Data Reference vector",
	"Floating-point Fault vector",
	"Floating-point Trap vector",
	"Lower-Privilege Transfer Trap vector",
	"Taken Branch Trap vector",
	"Single STep Trap vector",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"IA-32 Exception vector",
	"IA-32 Intercept vector",
	"IA-32 Interrupt vector",
	"Reserved",
	"Reserved",
	"Reserved"
};

static char *vector_to_string(__u16 vector);
static void dump_stack(struct exception_regdump *pstate);

char *vector_to_string(__u16 vector)
{
	ASSERT(vector <= VECTOR_MAX);
	
	if (vector >= VECTORS_16_BUNDLE_START)
		return vector_names_16_bundle[(vector-VECTORS_16_BUNDLE_START)/(16*BUNDLE_SIZE)];
	else
		return vector_names_64_bundle[vector/(64*BUNDLE_SIZE)];
}

void dump_stack(struct exception_regdump *pstate)
{
	char *ifa, *iipa, *iip;

	ifa = get_symtab_entry(pstate->cr_ifa);
	iipa = get_symtab_entry(pstate->cr_iipa);
	iip = get_symtab_entry(pstate->cr_iip);

	putchar('\n');
	printf("ar.bsp=%P\tar.bspstore=%P\n", pstate->ar_bsp, pstate->ar_bspstore);
	printf("ar.rnat=%Q\tar.rsc=%Q\n", pstate->ar_rnat, pstate->ar_rsc);
	printf("ar.ifs=%Q\tar.pfs=%Q\n", pstate->ar_ifs, pstate->ar_pfs);
	printf("cr.isr=%Q\tcr.ips=%Q\t\n", pstate->cr_isr, pstate->cr_ips);
	
	printf("cr.iip=%Q (%s)\n", pstate->cr_iip, iip ? iip : "?");
	printf("cr.iipa=%Q (%s)\n", pstate->cr_iipa, iipa ? iipa : "?");
	printf("cr.ifa=%Q (%s)\n", pstate->cr_ifa, ifa ? ifa : "?");
}

void general_exception(__u64 vector, struct exception_regdump *pstate)
{
	dump_stack(pstate);
	panic("General Exception\n");
}

void break_instruction(__u64 vector, struct exception_regdump *pstate)
{
	dump_stack(pstate);
	panic("Break Instruction\n");
}

void universal_handler(__u64 vector, struct exception_regdump *pstate)
{
	dump_stack(pstate);
	panic("Interruption: %W (%s)\n", (__u16) vector, vector_to_string(vector));
}

void external_interrupt(__u64 vector, struct exception_regdump *pstate)
{
	cr_ivr_t ivr;
	
	ivr.value = ivr_read();
	srlz_d();

	switch(ivr.vector) {
	    case INTERRUPT_TIMER:
		it_interrupt();
	    	break;
	    case INTERRUPT_SPURIOUS:
	    	printf("cpu%d: spurious interrupt\n", CPU->id);
		break;
	    default:
		panic("\nUnhandled External Interrupt Vector %d\n", ivr.vector);
		break;
	}
}
