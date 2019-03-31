/*
 * Copyright (c) 2015 Petr Pavlu
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

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Exception handlers and exception initialization routines.
 */

#include <arch/asm.h>
#include <arch/exception.h>
#include <arch/machine_func.h>
#include <arch/regutils.h>
#include <interrupt.h>
#include <mm/as.h>
#include <stdio.h>
#include <syscall/syscall.h>

static void current_el_sp_sel0_synch_exception(unsigned int exc_no,
    istate_t *istate)
{
	panic_badtrap(istate, exc_no, "Unhandled exception from Current EL, "
	    "SP_SEL0, Synch, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void current_el_sp_sel0_irq_exception(unsigned int exc_no,
    istate_t *istate)
{
	panic_badtrap(istate, exc_no, "Unhandled exception from Current EL, "
	    "SP_SEL0, IRQ, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void current_el_sp_sel0_fiq_exception(unsigned int exc_no,
    istate_t *istate)
{
	panic_badtrap(istate, exc_no, "Unhandled exception from Current EL, "
	    "SP_SEL0, FIQ, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void current_el_sp_sel0_serror_exception(unsigned int exc_no,
    istate_t *istate)
{
	panic_badtrap(istate, exc_no, "Unhandled exception from Current EL, "
	    "SP_SEL0, SError, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64
	    ".", (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void current_el_sp_selx_synch_exception(unsigned int exc_no,
    istate_t *istate)
{
	uint64_t esr_el1 = ESR_EL1_read();
	uint64_t far_el1 = FAR_EL1_read();
	pf_access_t access;

	switch ((esr_el1 & ESR_EC_MASK) >> ESR_EC_SHIFT) {
	case ESR_EC_DA_CURRENT_EL:
		/* Data abort. */
		switch ((esr_el1 & ESR_IDFSC_MASK) >> ESR_IDFSC_SHIFT) {
		case ESR_IDA_IDFSC_TF0:
		case ESR_IDA_IDFSC_TF1:
		case ESR_IDA_IDFSC_TF2:
		case ESR_IDA_IDFSC_TF3:
			/* Translation fault. */
			access = (esr_el1 & ESR_DA_WNR_FLAG) ? PF_ACCESS_WRITE :
			    PF_ACCESS_READ;
			as_page_fault(far_el1, access, istate);
			return;
		}
	}

	panic_badtrap(istate, exc_no, "Unhandled exception from Current EL, "
	    "SP_SELx, Synch, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void current_el_sp_selx_irq_exception(unsigned int exc_no,
    istate_t *istate)
{
	machine_irq_exception(exc_no, istate);
}

static void current_el_sp_selx_fiq_exception(unsigned int exc_no,
    istate_t *istate)
{
	panic_badtrap(istate, exc_no, "Unhandled exception from Current EL, "
	    "SP_SELx, FIQ, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void current_el_sp_selx_serror_exception(unsigned int exc_no,
    istate_t *istate)
{
	panic_badtrap(istate, exc_no, "Unhandled exception from Current EL, "
	    "SP_SELx, SError, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64
	    ".", (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void lower_el_aarch64_synch_exception(unsigned int exc_no,
    istate_t *istate)
{
	uint64_t esr_el1 = ESR_EL1_read();
	uint64_t far_el1 = FAR_EL1_read();
	pf_access_t access;
	bool exec = false;

	switch ((esr_el1 & ESR_EC_MASK) >> ESR_EC_SHIFT) {
	case ESR_EC_FP:
		/* Access to Advanced SIMD or floating-point functionality. */
#ifdef CONFIG_FPU_LAZY
		scheduler_fpu_lazy_request();
#else
		fault_from_uspace(istate, "AdvSIMD/FP fault.");
#endif
		return;
	case ESR_EC_SVC:
		/* SVC instruction. */
		interrupts_enable();
		istate->x0 = syscall_handler(istate->x0, istate->x1, istate->x2,
		    istate->x3, istate->x4, istate->x5, istate->x6);
		interrupts_disable();
		return;
	case ESR_EC_IA_LOWER_EL:
		/* Instruction abort. */
		exec = true;
		/* Fallthrough */
	case ESR_EC_DA_LOWER_EL:
		/* Data abort. */
		switch ((esr_el1 & ESR_IDFSC_MASK) >> ESR_IDFSC_SHIFT) {
		case ESR_IDA_IDFSC_TF0:
		case ESR_IDA_IDFSC_TF1:
		case ESR_IDA_IDFSC_TF2:
		case ESR_IDA_IDFSC_TF3:
			/* Translation fault. */
			if (exec)
				access = PF_ACCESS_EXEC;
			else
				access = (esr_el1 & ESR_DA_WNR_FLAG) ?
				    PF_ACCESS_WRITE : PF_ACCESS_READ;
			as_page_fault(far_el1, access, istate);
			return;
		}
	}

	fault_from_uspace(istate, "Unhandled exception from Lower EL, AArch64, "
	    "Synch, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) esr_el1, far_el1);
}

static void lower_el_aarch64_irq_exception(unsigned int exc_no,
    istate_t *istate)
{
	machine_irq_exception(exc_no, istate);
}

static void lower_el_aarch64_fiq_exception(unsigned int exc_no,
    istate_t *istate)
{
	fault_from_uspace(istate, "Unhandled exception from Lower EL, AArch64, "
	    "FIQ, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void lower_el_aarch64_serror_exception(unsigned int exc_no,
    istate_t *istate)
{
	fault_from_uspace(istate, "Unhandled exception from Lower EL, AArch64, "
	    "SError, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void lower_el_aarch32_synch_exception(unsigned int exc_no,
    istate_t *istate)
{
	fault_from_uspace(istate, "Unhandled exception from Lower EL, AArch32, "
	    "Synch, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void lower_el_aarch32_irq_exception(unsigned int exc_no,
    istate_t *istate)
{
	fault_from_uspace(istate, "Unhandled exception from Lower EL, AArch32, "
	    "IRQ, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void lower_el_aarch32_fiq_exception(unsigned int exc_no,
    istate_t *istate)
{
	fault_from_uspace(istate, "Unhandled exception from Lower EL, AArch32, "
	    "FIQ, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

static void lower_el_aarch32_serror_exception(unsigned int exc_no,
    istate_t *istate)
{
	fault_from_uspace(istate, "Unhandled exception from Lower EL, AArch32, "
	    "SError, ESR_EL1=%0#10" PRIx32 ", FAR_EL1=%0#18" PRIx64 ".",
	    (uint32_t) ESR_EL1_read(), FAR_EL1_read());
}

/** Initializes exception handling.
 *
 * Installs low-level exception handlers and then registers exceptions and their
 * handlers to kernel exception dispatcher.
 */
void exception_init(void)
{
	exc_register(EXC_CURRENT_EL_SP_SEL0_SYNCH,
	    "current EL, SP_SEL0, Synchronous", true,
	    (iroutine_t) current_el_sp_sel0_synch_exception);
	exc_register(EXC_CURRENT_EL_SP_SEL0_IRQ,
	    "current EL, SP_SEL0, IRQ", true,
	    (iroutine_t) current_el_sp_sel0_irq_exception);
	exc_register(EXC_CURRENT_EL_SP_SEL0_FIQ,
	    "current EL, SP_SEL0, FIQ", true,
	    (iroutine_t) current_el_sp_sel0_fiq_exception);
	exc_register(EXC_CURRENT_EL_SP_SEL0_SERROR,
	    "current EL, SP_SEL0, SError", true,
	    (iroutine_t) current_el_sp_sel0_serror_exception);
	exc_register(EXC_CURRENT_EL_SP_SELX_SYNCH,
	    "current EL, SP_SELx, Synchronous", true,
	    (iroutine_t) current_el_sp_selx_synch_exception);
	exc_register(EXC_CURRENT_EL_SP_SELX_IRQ,
	    "current EL, SP_SELx, IRQ", true,
	    (iroutine_t) current_el_sp_selx_irq_exception);
	exc_register(EXC_CURRENT_EL_SP_SELX_FIQ,
	    "current EL, SP_SELx, FIQ", true,
	    (iroutine_t) current_el_sp_selx_fiq_exception);
	exc_register(EXC_CURRENT_EL_SP_SELX_SERROR,
	    "current EL, SP_SELx, SError", true,
	    (iroutine_t) current_el_sp_selx_serror_exception);
	exc_register(EXC_LOWER_EL_AARCH64_SYNCH,
	    "lower EL, AArch64, Synchronous", true,
	    (iroutine_t) lower_el_aarch64_synch_exception);
	exc_register(EXC_LOWER_EL_AARCH64_IRQ,
	    "lower EL, AArch64, IRQ", true,
	    (iroutine_t) lower_el_aarch64_irq_exception);
	exc_register(EXC_LOWER_EL_AARCH64_FIQ,
	    "lower EL, AArch64, FIQ", true,
	    (iroutine_t) lower_el_aarch64_fiq_exception);
	exc_register(EXC_LOWER_EL_AARCH64_SERROR,
	    "lower EL, AArch64, SError", true,
	    (iroutine_t) lower_el_aarch64_serror_exception);
	exc_register(EXC_LOWER_EL_AARCH32_SYNCH,
	    "lower EL, AArch32, Synchronous", true,
	    (iroutine_t) lower_el_aarch32_synch_exception);
	exc_register(EXC_LOWER_EL_AARCH32_IRQ,
	    "lower EL, AArch32, IRQ", true,
	    (iroutine_t) lower_el_aarch32_irq_exception);
	exc_register(EXC_LOWER_EL_AARCH32_FIQ,
	    "lower EL, AArch32, FIQ", true,
	    (iroutine_t) lower_el_aarch32_fiq_exception);
	exc_register(EXC_LOWER_EL_AARCH32_SERROR,
	    "lower EL, AArch32, SError", true,
	    (iroutine_t) lower_el_aarch32_serror_exception);

	VBAR_EL1_write(((uint64_t) &exc_vector));
}

/** Print #istate_t structure content.
 *
 * @param istate Structure to be printed.
 */
void istate_decode(istate_t *istate)
{
	printf("x0 =%0#18" PRIx64 "\tx1 =%0#18" PRIx64 "\t"
	    "x2 =%0#18" PRIx64 "\n", istate->x0, istate->x1, istate->x2);
	printf("x3 =%0#18" PRIx64 "\tx4 =%0#18" PRIx64 "\t"
	    "x5 =%0#18" PRIx64 "\n", istate->x3, istate->x4, istate->x5);
	printf("x6 =%0#18" PRIx64 "\tx7 =%0#18" PRIx64 "\t"
	    "x8 =%0#18" PRIx64 "\n", istate->x6, istate->x7, istate->x8);
	printf("x9 =%0#18" PRIx64 "\tx10=%0#18" PRIx64 "\t"
	    "x11=%0#18" PRIx64 "\n", istate->x9, istate->x10, istate->x11);
	printf("x12=%0#18" PRIx64 "\tx13=%0#18" PRIx64 "\t"
	    "x14=%0#18" PRIx64 "\n", istate->x12, istate->x13, istate->x14);
	printf("x15=%0#18" PRIx64 "\tx16=%0#18" PRIx64 "\t"
	    "x17=%0#18" PRIx64 "\n", istate->x15, istate->x16, istate->x17);
	printf("x18=%0#18" PRIx64 "\tx19=%0#18" PRIx64 "\t"
	    "x20=%0#18" PRIx64 "\n", istate->x18, istate->x19, istate->x20);
	printf("x21=%0#18" PRIx64 "\tx22=%0#18" PRIx64 "\t"
	    "x23=%0#18" PRIx64 "\n", istate->x21, istate->x22, istate->x23);
	printf("x24=%0#18" PRIx64 "\tx25=%0#18" PRIx64 "\t"
	    "x26=%0#18" PRIx64 "\n", istate->x24, istate->x25, istate->x26);
	printf("x27=%0#18" PRIx64 "\tx28=%0#18" PRIx64 "\t"
	    "x29=%0#18" PRIx64 "\n", istate->x27, istate->x28, istate->x29);
	printf("x30=%0#18" PRIx64 "\tsp =%0#18" PRIx64 "\t"
	    "pc =%0#18" PRIx64 "\n", istate->x30, istate->sp, istate->pc);
	printf("spsr=%0#18" PRIx64 "\ttpidr=%0#18" PRIx64 "\n", istate->spsr,
	    istate->tpidr);
}

/** @}
 */
