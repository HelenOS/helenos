/*
 * Copyright (c) 2003-2004 Jakub Jermar
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

/** @addtogroup mips32
 * @{
 */
/** @file
 */

#include <arch/exception.h>
#include <arch/interrupt.h>
#include <arch/mm/tlb.h>
#include <panic.h>
#include <arch/cp0.h>
#include <arch.h>
#include <assert.h>
#include <proc/thread.h>
#include <print.h>
#include <interrupt.h>
#include <halt.h>
#include <ddi/irq.h>
#include <arch/debugger.h>
#include <symtab.h>
#include <log.h>

static const char *exctable[] = {
	"Interrupt",
	"TLB Modified",
	"TLB Invalid",
	"TLB Invalid Store",
	"Address Error - load/instr. fetch",
	"Address Error - store",
	"Bus Error - fetch instruction",
	"Bus Error - data reference",
	"Syscall",
	"BreakPoint",
	"Reserved Instruction",
	"Coprocessor Unusable",
	"Arithmetic Overflow",
	"Trap",
	"Virtual Coherency - instruction",
	"Floating Point",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"WatchHi/WatchLo",  /* 23 */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"Virtual Coherency - data",
};

void istate_decode(istate_t *istate)
{
	log_printf("epc=%#010" PRIx32 "\tsta=%#010" PRIx32 "\t"
	    "lo =%#010" PRIx32 "\thi =%#010" PRIx32 "\n",
	    istate->epc, istate->status, istate->lo, istate->hi);

	log_printf("a0 =%#010" PRIx32 "\ta1 =%#010" PRIx32 "\t"
	    "a2 =%#010" PRIx32 "\ta3 =%#010" PRIx32 "\n",
	    istate->a0, istate->a1, istate->a2, istate->a3);

	log_printf("t0 =%#010" PRIx32 "\tt1 =%#010" PRIx32 "\t"
	    "t2 =%#010" PRIx32 "\tt3 =%#010" PRIx32 "\n",
	    istate->t0, istate->t1, istate->t2, istate->t3);

	log_printf("t4 =%#010" PRIx32 "\tt5 =%#010" PRIx32 "\t"
	    "t6 =%#010" PRIx32 "\tt7 =%#010" PRIx32 "\n",
	    istate->t4, istate->t5, istate->t6, istate->t7);

	log_printf("t8 =%#010" PRIx32 "\tt9 =%#010" PRIx32 "\t"
	    "v0 =%#010" PRIx32 "\tv1 =%#010" PRIx32 "\n",
	    istate->t8, istate->t9, istate->v0, istate->v1);

	log_printf("s0 =%#010" PRIx32 "\ts1 =%#010" PRIx32 "\t"
	    "s2 =%#010" PRIx32 "\ts3 =%#010" PRIx32 "\n",
	    istate->s0, istate->s1, istate->s2, istate->s3);

	log_printf("s4 =%#010" PRIx32 "\ts5 =%#010" PRIx32 "\t"
	    "s6 =%#010" PRIx32 "\ts7 =%#010" PRIx32 "\n",
	    istate->s4, istate->s5, istate->s6, istate->s7);

	log_printf("s8 =%#010" PRIx32 "\tat =%#010" PRIx32 "\t"
	    "kt0=%#010" PRIx32 "\tkt1=%#010" PRIx32 "\n",
	    istate->s8, istate->at, istate->kt0, istate->kt1);

	log_printf("sp =%#010" PRIx32 "\tra =%#010" PRIx32 "\t"
	    "gp =%#010" PRIx32 "\n",
	    istate->sp, istate->ra, istate->gp);
}

static void unhandled_exception(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "Unhandled exception %s.", exctable[n]);
	panic_badtrap(istate, n, "Unhandled exception %s.", exctable[n]);
}

static void reserved_instr_exception(unsigned int n, istate_t *istate)
{
	if (*((uint32_t *) istate->epc) == 0x7c03e83b) {
		assert(THREAD);
		istate->epc += 4;
		istate->v1 = istate->kt1;
	} else
		unhandled_exception(n, istate);
}

static void breakpoint_exception(unsigned int n, istate_t *istate)
{
#ifdef CONFIG_DEBUG
	debugger_bpoint(istate);
#else
	/* it is necessary to not re-execute BREAK instruction after
	   returning from Exception handler
	   (see page 138 in R4000 Manual for more information) */
	istate->epc += 4;
#endif
}

static void tlbmod_exception(unsigned int n, istate_t *istate)
{
	tlb_modified(istate);
}

static void tlbinv_exception(unsigned int n, istate_t *istate)
{
	tlb_invalid(istate);
}

#ifdef CONFIG_FPU_LAZY
static void cpuns_exception(unsigned int n, istate_t *istate)
{
	if (cp0_cause_coperr(cp0_cause_read()) == fpu_cop_id)
		scheduler_fpu_lazy_request();
	else {
		fault_if_from_uspace(istate,
		    "Unhandled Coprocessor Unusable Exception.");
		panic_badtrap(istate, n,
		    "Unhandled Coprocessor Unusable Exception.");
	}
}
#endif

static void interrupt_exception(unsigned int n, istate_t *istate)
{
	uint32_t ip;
	uint32_t im;

	/* Decode interrupt number and process the interrupt */
	ip = (cp0_cause_read() & cp0_cause_ip_mask) >> cp0_cause_ip_shift;
	im = (cp0_status_read() & cp0_status_im_mask) >> cp0_status_im_shift;

	unsigned int i;
	for (i = 0; i < 8; i++) {

		/*
		 * The interrupt could only occur if it is unmasked in the
		 * status register. On the other hand, an interrupt can be
		 * apparently pending even if it is masked, so we need to
		 * check both the masked and pending interrupts.
		 */
		if (im & ip & (1 << i)) {
			irq_t *irq = irq_dispatch_and_lock(i);
			if (irq) {
				/*
				 * The IRQ handler was found.
				 */
				irq->handler(irq);
				irq_spinlock_unlock(&irq->lock, false);
			} else {
				/*
				 * Spurious interrupt.
				 */
#ifdef CONFIG_DEBUG
				log(LF_ARCH, LVL_DEBUG,
				    "cpu%u: spurious interrupt (inum=%u)",
				    CPU->id, i);
#endif
			}
		}
	}
}

/** Handle syscall userspace call */
static void syscall_exception(unsigned int n, istate_t *istate)
{
	fault_if_from_uspace(istate, "Syscall is handled through shortcut.");
}

void exception_init(void)
{
	unsigned int i;

	/* Clear exception table */
	for (i = 0; i < IVT_ITEMS; i++)
		exc_register(i, "undef", false,
		    (iroutine_t) unhandled_exception);

	exc_register(EXC_Bp, "bkpoint", true,
	    (iroutine_t) breakpoint_exception);
	exc_register(EXC_RI, "resinstr", true,
	    (iroutine_t) reserved_instr_exception);
	exc_register(EXC_Mod, "tlb_mod", true,
	    (iroutine_t) tlbmod_exception);
	exc_register(EXC_TLBL, "tlbinvl", true,
	    (iroutine_t) tlbinv_exception);
	exc_register(EXC_TLBS, "tlbinvl", true,
	    (iroutine_t) tlbinv_exception);
	exc_register(EXC_Int, "interrupt", true,
	    (iroutine_t) interrupt_exception);

#ifdef CONFIG_FPU_LAZY
	exc_register(EXC_CpU, "cpunus", true,
	    (iroutine_t) cpuns_exception);
#endif

	exc_register(EXC_Sys, "syscall", true,
	    (iroutine_t) syscall_exception);
}

/** @}
 */
