/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2005 Jakub Vana
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
#include <assert.h>
#include <interrupt.h>
#include <ddi/irq.h>
#include <panic.h>
#include <print.h>
#include <console/console.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <arch/barrier.h>
#include <arch/register.h>
#include <arch.h>
#include <syscall/syscall.h>
#include <print.h>
#include <proc/scheduler.h>
#include <ipc/sysipc.h>
#include <ipc/irq.h>
#include <ipc/ipc.h>
#include <synch/spinlock.h>
#include <mm/tlb.h>
#include <arch/mm/tlb.h>
#include <symtab.h>
#include <putchar.h>

#define VECTORS_64_BUNDLE        20
#define VECTORS_16_BUNDLE        48
#define VECTORS_16_BUNDLE_START  0x50

#define VECTOR_MAX  0x7f

static const char *vector_names_64_bundle[VECTORS_64_BUNDLE] = {
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
	"Data Access-Bit vector",
	"Break Instruction vector",
	"External Interrupt vector",
	"Virtual External Interrupt vector",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved"
};

static const char *vector_names_16_bundle[VECTORS_16_BUNDLE] = {
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
	"Single Step Trap vector",
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

static const char *vector_to_string(unsigned int n)
{
	assert(n <= VECTOR_MAX);

	if (n >= VECTORS_16_BUNDLE_START)
		return vector_names_16_bundle[n - VECTORS_16_BUNDLE_START];
	else
		return vector_names_64_bundle[n / 4];
}

void istate_decode(istate_t *istate)
{
	printf("ar.bsp=%p\tar.bspstore=%p\n",
	    (void *) istate->ar_bsp, (void *) istate->ar_bspstore);
	printf("ar.rnat=%#0" PRIx64 "\tar.rsc=%#0" PRIx64 "\n",
	    istate->ar_rnat, istate->ar_rsc);
	printf("ar.ifs=%#0" PRIx64 "\tar.pfs=%#0" PRIx64 "\n",
	    istate->ar_ifs, istate->ar_pfs);
	printf("cr.isr=%#0" PRIx64 "\tcr.ipsr=%#0" PRIx64 "\n",
	    istate->cr_isr.value, istate->cr_ipsr.value);

	printf("cr.iip=%#0" PRIxPTR ", #%u\t(%s)\n",
	    istate->cr_iip, istate->cr_isr.ei,
	    symtab_fmt_name_lookup(istate->cr_iip));
	printf("cr.iipa=%#0" PRIxPTR "\t(%s)\n", istate->cr_iipa,
	    symtab_fmt_name_lookup(istate->cr_iipa));
	printf("cr.ifa=%#0" PRIxPTR "\t(%s)\n", istate->cr_ifa,
	    symtab_fmt_name_lookup(istate->cr_ifa));
}

void general_exception(unsigned int n, istate_t *istate)
{
	const char *desc;

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

	fault_if_from_uspace(istate, "General Exception (%s).", desc);
	panic_badtrap(istate, n, "General Exception (%s).", desc);
}

void disabled_fp_register(unsigned int n, istate_t *istate)
{
#ifdef CONFIG_FPU_LAZY
	scheduler_fpu_lazy_request();
#else
	fault_if_from_uspace(istate, "Interruption: %#hx (%s).",
	    (uint16_t) n, vector_to_string(n));
	panic_badtrap(istate, vector, "Interruption: %#hx (%s).",
	    (uint16_t) n, vector_to_string(n));
#endif
}

/** Handle syscall. */
sysarg_t break_instruction(unsigned int n, istate_t *istate)
{
	sysarg_t ret;

	/*
	 * Move to next instruction after BREAK.
	 */
	if (istate->cr_ipsr.ri == 2) {
		istate->cr_ipsr.ri = 0;
		istate->cr_iip += 16;
	} else {
		istate->cr_ipsr.ri++;
	}

	interrupts_enable();
	ret = syscall_handler(istate->in0, istate->in1, istate->in2,
	    istate->in3, istate->in4, istate->in5, istate->in6);
	interrupts_disable();

	return ret;
}

void universal_handler(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "Interruption: %#hx (%s).",
	    n, vector_to_string(n));
	panic_badtrap(istate, n, "Interruption: %#hx (%s).",
	    n, vector_to_string(n));
}

static void end_of_local_irq(void)
{
	asm volatile (
	    "mov cr.eoi = r0 ;;"
	);
}

void external_interrupt(unsigned int n, istate_t *istate)
{
	cr_ivr_t ivr;

	ivr.value = ivr_read();
	srlz_d();

	irq_t *irq;

	switch (ivr.vector) {
	case INTERRUPT_SPURIOUS:
#ifdef CONFIG_DEBUG
		printf("cpu%d: spurious interrupt\n", CPU->id);
#endif
		break;

#ifdef CONFIG_SMP
	case VECTOR_TLB_SHOOTDOWN_IPI:
		tlb_shootdown_ipi_recv();
		end_of_local_irq();
		break;
#endif

	case INTERRUPT_TIMER:
		irq = irq_dispatch_and_lock(ivr.vector);
		if (irq) {
			irq->handler(irq);
			irq_spinlock_unlock(&irq->lock, false);
		} else {
			panic("Unhandled Internal Timer Interrupt (%d).",
			    ivr.vector);
		}
		break;
	default:
		irq = irq_dispatch_and_lock(ivr.vector);
		if (irq) {
			/*
			 * The IRQ handler was found.
			 */
			if (irq->preack) {
				/* Send EOI before processing the interrupt */
				end_of_local_irq();
			}
			irq->handler(irq);
			if (!irq->preack)
				end_of_local_irq();
			irq_spinlock_unlock(&irq->lock, false);
		} else {
			/*
			 * Unhandled interrupt.
			 */
			end_of_local_irq();
#ifdef CONFIG_DEBUG
			printf("\nUnhandled External Interrupt Vector %d\n",
			    ivr.vector);
#endif
		}
		break;
	}
}

void trap_virtual_enable_irqs(uint16_t irqmask)
{
}

void exception_init(void)
{
	unsigned int i;

	for (i = 0; i < IVT_ITEMS; i++)
		exc_register(i, "universal_handler", false, universal_handler);

	exc_register(EXC_ALT_ITLB_FAULT,
	    vector_to_string(EXC_ALT_ITLB_FAULT), true,
	    alternate_instruction_tlb_fault);
	exc_register(EXC_ALT_DTLB_FAULT,
	    vector_to_string(EXC_ALT_DTLB_FAULT), true,
	    alternate_data_tlb_fault);
	exc_register(EXC_NESTED_TLB_FAULT,
	    vector_to_string(EXC_NESTED_TLB_FAULT), false,
	    data_nested_tlb_fault);
	exc_register(EXC_DATA_D_BIT_FAULT,
	    vector_to_string(EXC_DATA_D_BIT_FAULT), true,
	    data_dirty_bit_fault);
	exc_register(EXC_INST_A_BIT_FAULT,
	    vector_to_string(EXC_INST_A_BIT_FAULT), true,
	    instruction_access_bit_fault);
	exc_register(EXC_DATA_A_BIT_FAULT,
	    vector_to_string(EXC_DATA_A_BIT_FAULT), true,
	    data_access_bit_fault);
	exc_register(EXC_EXT_INTERRUPT,
	    vector_to_string(EXC_EXT_INTERRUPT), true,
	    external_interrupt);

	exc_register(EXC_PAGE_NOT_PRESENT,
	    vector_to_string(EXC_PAGE_NOT_PRESENT), true,
	    page_not_present);
	exc_register(EXC_DATA_AR_FAULT,
	    vector_to_string(EXC_DATA_AR_FAULT), true,
	    data_access_rights_fault);
	exc_register(EXC_GENERAL_EXCEPTION,
	    vector_to_string(EXC_GENERAL_EXCEPTION), false,
	    general_exception);
	exc_register(EXC_DISABLED_FP_REG,
	    vector_to_string(EXC_DISABLED_FP_REG), true,
	    disabled_fp_register);
}

/** @}
 */
