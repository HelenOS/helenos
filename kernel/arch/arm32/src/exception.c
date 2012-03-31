/*
 * Copyright (c) 2007 Petr Stepan
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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief Exception handlers and exception initialization routines.
 */

#include <arch/exception.h>
#include <arch/regutils.h>
#include <arch/machine_func.h>
#include <interrupt.h>
#include <arch/mm/page_fault.h>
#include <arch/barrier.h>
#include <print.h>
#include <syscall/syscall.h>
#include <stacktrace.h>

/** Offset used in calculation of exception handler's relative address.
 *
 * @see install_handler()
 */
#define PREFETCH_OFFSET      0x8

/** LDR instruction's code */
#define LDR_OPCODE           0xe59ff000

/** Number of exception vectors. */
#define EXC_VECTORS          8

/** Size of memory block occupied by exception vectors. */
#define EXC_VECTORS_SIZE     (EXC_VECTORS * 4)

/** Updates specified exception vector to jump to given handler.
 *
 *  Addresses of handlers are stored in memory following exception vectors.
 */
static void install_handler(unsigned handler_addr, unsigned *vector)
{
	/* relative address (related to exc. vector) of the word
	 * where handler's address is stored
	*/
	volatile uint32_t handler_address_ptr = EXC_VECTORS_SIZE -
	    PREFETCH_OFFSET;
	
	/* make it LDR instruction and store at exception vector */
	*vector = handler_address_ptr | LDR_OPCODE;
	smc_coherence(*vector);
	
	/* store handler's address */
	*(vector + EXC_VECTORS) = handler_addr;

}

/** Software Interrupt handler.
 *
 * Dispatches the syscall.
 *
 */
static void swi_exception(unsigned int exc_no, istate_t *istate)
{
	istate->r0 = syscall_handler(istate->r0, istate->r1, istate->r2,
	    istate->r3, istate->r4, istate->r5, istate->r6);
}

/** Fills exception vectors with appropriate exception handlers. */
void install_exception_handlers(void)
{
	install_handler((unsigned) reset_exception_entry,
	    (unsigned *) EXC_RESET_VEC);
	
	install_handler((unsigned) undef_instr_exception_entry,
	    (unsigned *) EXC_UNDEF_INSTR_VEC);
	
	install_handler((unsigned) swi_exception_entry,
	    (unsigned *) EXC_SWI_VEC);
	
	install_handler((unsigned) prefetch_abort_exception_entry,
	    (unsigned *) EXC_PREFETCH_ABORT_VEC);
	
	install_handler((unsigned) data_abort_exception_entry,
	    (unsigned *) EXC_DATA_ABORT_VEC);
	
	install_handler((unsigned) irq_exception_entry,
	    (unsigned *) EXC_IRQ_VEC);
	
	install_handler((unsigned) fiq_exception_entry,
	    (unsigned *) EXC_FIQ_VEC);
}

#ifdef HIGH_EXCEPTION_VECTORS
/** Activates use of high exception vectors addresses. */
static void high_vectors(void)
{
	uint32_t control_reg;
	
	asm volatile (
		"mrc p15, 0, %[control_reg], c1, c1"
		: [control_reg] "=r" (control_reg)
	);
	
	/* switch on the high vectors bit */
	control_reg |= CP15_R1_HIGH_VECTORS_BIT;
	
	asm volatile (
		"mcr p15, 0, %[control_reg], c1, c1"
		:: [control_reg] "r" (control_reg)
	);
}
#endif

/** Interrupt Exception handler.
 *
 * Determines the sources of interrupt and calls their handlers.
 */
static void irq_exception(unsigned int exc_no, istate_t *istate)
{
	machine_irq_exception(exc_no, istate);
}

/** Initializes exception handling.
 *
 * Installs low-level exception handlers and then registers
 * exceptions and their handlers to kernel exception dispatcher.
 */
void exception_init(void)
{
#ifdef HIGH_EXCEPTION_VECTORS
	high_vectors();
#endif
	install_exception_handlers();
	
	exc_register(EXC_IRQ, "interrupt", true,
	    (iroutine_t) irq_exception);
	exc_register(EXC_PREFETCH_ABORT, "prefetch abort", true,
	    (iroutine_t) prefetch_abort);
	exc_register(EXC_DATA_ABORT, "data abort", true,
	    (iroutine_t) data_abort);
	exc_register(EXC_SWI, "software interrupt", true,
	    (iroutine_t) swi_exception);
}

/** Prints #istate_t structure content.
 *
 * @param istate Structure to be printed.
 */
void istate_decode(istate_t *istate)
{
	printf("r0 =%0#10" PRIx32 "\tr1 =%0#10" PRIx32 "\t"
	    "r2 =%0#10" PRIx32 "\tr3 =%0#10" PRIx32 "\n",
	    istate->r0, istate->r1, istate->r2, istate->r3);
	printf("r4 =%0#10" PRIx32 "\tr5 =%0#10" PRIx32 "\t"
	    "r6 =%0#10" PRIx32 "\tr7 =%0#10" PRIx32 "\n",
	    istate->r4, istate->r5, istate->r6, istate->r7);
	printf("r8 =%0#10" PRIx32 "\tr9 =%0#10" PRIx32 "\t"
	    "r10=%0#10" PRIx32 "\tfp =%0#10" PRIx32 "\n",
	    istate->r8, istate->r9, istate->r10, istate->fp);
	printf("r12=%0#10" PRIx32 "\tsp =%0#10" PRIx32 "\t"
	    "lr =%0#10" PRIx32 "\tspsr=%0#10" PRIx32 "\n",
	    istate->r12, istate->sp, istate->lr, istate->spsr);
}

/** @}
 */
