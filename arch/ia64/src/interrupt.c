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
 */

/** @addtogroup ia64interrupt
 * @{
 */
/** @file
 */

#include <arch/interrupt.h>
#include <panic.h>
#include <print.h>
#include <console/console.h>
#include <arch/types.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <arch/register.h>
#include <arch/drivers/it.h>
#include <arch.h>
#include <symtab.h>
#include <debug.h>
#include <syscall/syscall.h>
#include <print.h>
#include <proc/scheduler.h>
#include <ipc/sysipc.h>
#include <ipc/irq.h>
#include <ipc/ipc.h>
#include <interrupt.h>


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
static void dump_interrupted_context(istate_t *istate);

char *vector_to_string(__u16 vector)
{
	ASSERT(vector <= VECTOR_MAX);
	
	if (vector >= VECTORS_16_BUNDLE_START)
		return vector_names_16_bundle[(vector-VECTORS_16_BUNDLE_START)/(16*BUNDLE_SIZE)];
	else
		return vector_names_64_bundle[vector/(64*BUNDLE_SIZE)];
}

void dump_interrupted_context(istate_t *istate)
{
	char *ifa, *iipa, *iip;

	ifa = get_symtab_entry(istate->cr_ifa);
	iipa = get_symtab_entry(istate->cr_iipa);
	iip = get_symtab_entry(istate->cr_iip);

	putchar('\n');
	printf("Interrupted context dump:\n");
	printf("ar.bsp=%p\tar.bspstore=%p\n", istate->ar_bsp, istate->ar_bspstore);
	printf("ar.rnat=%#018llx\tar.rsc=%#018llx\n", istate->ar_rnat, istate->ar_rsc);
	printf("ar.ifs=%#018llx\tar.pfs=%#018llx\n", istate->ar_ifs, istate->ar_pfs);
	printf("cr.isr=%#018llx\tcr.ipsr=%#018llx\t\n", istate->cr_isr.value, istate->cr_ipsr);
	
	printf("cr.iip=%#018llx, #%d\t(%s)\n", istate->cr_iip, istate->cr_isr.ei, iip);
	printf("cr.iipa=%#018llx\t(%s)\n", istate->cr_iipa, iipa);
	printf("cr.ifa=%#018llx\t(%s)\n", istate->cr_ifa, ifa);
}

void general_exception(__u64 vector, istate_t *istate)
{
	char *desc = "";

	switch (istate->cr_isr.ge_code) {
	    case GE_ILLEGALOP:
		desc = "Illegal Operation fault";
		break;
	    case GE_PRIVOP:
		desc = "Privileged Operation fault";
		break;
	    case GE_PRIVREG:
		desc = "Privileged Register fault";
		break;
	    case GE_RESREGFLD:
		desc = "Reserved Register/Field fault";
		break;
	    case GE_DISBLDISTRAN:
		desc = "Disabled Instruction Set Transition fault";
		break;
	    case GE_ILLEGALDEP:
		desc = "Illegal Dependency fault";
		break;
	    default:
	    	desc = "unknown";
		break;
	}

	fault_if_from_uspace(istate, "General Exception (%s)", desc);

	dump_interrupted_context(istate);
	panic("General Exception (%s)\n", desc);
}

void fpu_enable(void);

void disabled_fp_register(__u64 vector, istate_t *istate)
{
#ifdef CONFIG_FPU_LAZY 
	scheduler_fpu_lazy_request();	
#else
	fault_if_from_uspace(istate, "Interruption: %#hx (%s)", (__u16) vector, vector_to_string(vector));
	dump_interrupted_context(istate);
	panic("Interruption: %#hx (%s)\n", (__u16) vector, vector_to_string(vector));
#endif
}


void nop_handler(__u64 vector, istate_t *istate)
{
}



/** Handle syscall. */
int break_instruction(__u64 vector, istate_t *istate)
{
	/*
	 * Move to next instruction after BREAK.
	 */
	if (istate->cr_ipsr.ri == 2) {
		istate->cr_ipsr.ri = 0;
		istate->cr_iip += 16;
	} else {
		istate->cr_ipsr.ri++;
	}

	if (istate->in4 < SYSCALL_END)
		return syscall_table[istate->in4](istate->in0, istate->in1, istate->in2, istate->in3);
	else
		panic("Undefined syscall %d", istate->in4);
		
	return -1;
}

void universal_handler(__u64 vector, istate_t *istate)
{
	fault_if_from_uspace(istate,"Interruption: %#hx (%s)\n",(__u16) vector, vector_to_string(vector));
	dump_interrupted_context(istate);
	panic("Interruption: %#hx (%s)\n", (__u16) vector, vector_to_string(vector));
}

void external_interrupt(__u64 vector, istate_t *istate)
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

void virtual_interrupt(__u64 irq,void *param)
{
	switch(irq) {
		case IRQ_KBD:
			if(kbd_uspace) ipc_irq_send_notif(irq);
			break;
		default:
			panic("\nUnhandled Virtual Interrupt request %d\n", irq);
		break;
	}
}

/* Reregister irq to be IPC-ready */
void irq_ipc_bind_arch(__native irq)
{
	if(irq==IRQ_KBD) {
		kbd_uspace=1;
		return;
	}
	return;
	panic("not implemented\n");
	/* TODO */
}

/** @}
 */

