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
#include <arch/memstr.h>
#include <arch/regutils.h>
#include <interrupt.h>
#include <arch/mm/page_fault.h>
#include <arch/barrier.h>
#include <arch/drivers/gxemul.h>
#include <print.h>
#include <syscall/syscall.h>

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

/** Switches to kernel stack and saves all registers there.
 *
 * Temporary exception stack is used to save a few registers
 * before stack switch takes place.
 *
 */
inline static void setup_stack_and_save_regs()
{
	asm volatile (
		"ldr r13, =exc_stack\n"
		"stmfd r13!, {r0}\n"
		"mrs r0, spsr\n"
		"and r0, r0, #0x1f\n"
		"cmp r0, #0x10\n"
		"bne 1f\n"
		
		/* prev mode was usermode */
		"ldmfd r13!, {r0}\n"
		"ldr r13, =supervisor_sp\n"
		"ldr r13, [r13]\n"
		"stmfd r13!, {lr}\n"
		"stmfd r13!, {r0-r12}\n"
		"stmfd r13!, {r13, lr}^\n"
		"mrs r0, spsr\n"
		"stmfd r13!, {r0}\n"
		"b 2f\n"
		
		/* mode was not usermode */
		"1:\n"
			"stmfd r13!, {r1, r2, r3}\n"
			"mrs r1, cpsr\n"
			"mov r2, lr\n"
			"bic r1, r1, #0x1f\n"
			"orr r1, r1, r0\n"
			"mrs r0, cpsr\n"
			"msr cpsr_c, r1\n"
			
			"mov r3, r13\n"
			"stmfd r13!, {r2}\n"
			"mov r2, lr\n"
			"stmfd r13!, {r4-r12}\n"
			"mov r1, r13\n"
			
			/* the following two lines are for debugging */
			"mov sp, #0\n"
			"mov lr, #0\n"
			"msr cpsr_c, r0\n"
			
			"ldmfd r13!, {r4, r5, r6, r7}\n"
			"stmfd r1!, {r4, r5, r6}\n"
			"stmfd r1!, {r7}\n"
			"stmfd r1!, {r2}\n"
			"stmfd r1!, {r3}\n"
			"mrs r0, spsr\n"
			"stmfd r1!, {r0}\n"
			"mov r13, r1\n"
			
		"2:\n"
	);
}

/** Returns from exception mode.
 * 
 * Previously saved state of registers (including control register)
 * is restored from the stack.
 */
inline static void load_regs()
{
	asm volatile(
		"ldmfd r13!, {r0}		\n"
		"msr spsr, r0			\n"
		"and r0, r0, #0x1f		\n"
		"cmp r0, #0x10			\n"
		"bne 1f				\n"

		/* return to user mode */
		"ldmfd r13!, {r13, lr}^		\n"
		"b 2f				\n"

		/* return to non-user mode */
	"1:\n"
		"ldmfd r13!, {r1, r2}		\n"
		"mrs r3, cpsr			\n"
		"bic r3, r3, #0x1f		\n"
		"orr r3, r3, r0			\n"
		"mrs r0, cpsr			\n"
		"msr cpsr_c, r3			\n"

		"mov r13, r1			\n"
		"mov lr, r2			\n"
		"msr cpsr_c, r0			\n"

		/* actual return */
	"2:\n"
		"ldmfd r13, {r0-r12, pc}^\n"
	);
}


/** Switch CPU to mode in which interrupts are serviced (currently it 
 * is Undefined mode).
 *
 * The default mode for interrupt servicing (Interrupt Mode)
 * can not be used because of nested interrupts (which can occur
 * because interrupts are enabled in higher levels of interrupt handler).
 */
inline static void switch_to_irq_servicing_mode()
{
	/* switch to Undefined mode */
	asm volatile(
		/* save regs used during switching */
		"stmfd sp!, {r0-r3}		\n"

		/* save stack pointer and link register to r1, r2 */
		"mov r1, sp			\n"
		"mov r2, lr			\n"

		/* mode switch */
		"mrs r0, cpsr			\n"
		"bic r0, r0, #0x1f		\n"
		"orr r0, r0, #0x1b		\n"
		"msr cpsr_c, r0			\n"

		/* restore saved sp and lr */
		"mov sp, r1			\n"
		"mov lr, r2			\n"

		/* restore original regs */
		"ldmfd sp!, {r0-r3}		\n"
	);
}

/** Calls exception dispatch routine. */
#define CALL_EXC_DISPATCH(exception) \
	asm volatile ( \
		"mov r0, %[exc]\n" \
		"mov r1, r13\n" \
		"bl exc_dispatch\n" \
		:: [exc] "i" (exception) \
	);\

/** General exception handler.
 *
 *  Stores registers, dispatches the exception,
 *  and finally restores registers and returns from exception processing.
 *
 *  @param exception Exception number.
 */
#define PROCESS_EXCEPTION(exception) \
	setup_stack_and_save_regs(); \
	CALL_EXC_DISPATCH(exception) \
	load_regs();

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

/** Low-level Reset Exception handler. */
static void reset_exception_entry(void)
{
	PROCESS_EXCEPTION(EXC_RESET);
}

/** Low-level Software Interrupt Exception handler. */
static void swi_exception_entry(void)
{
	PROCESS_EXCEPTION(EXC_SWI);
}

/** Low-level Undefined Instruction Exception handler. */
static void undef_instr_exception_entry(void)
{
	PROCESS_EXCEPTION(EXC_UNDEF_INSTR);
}

/** Low-level Fast Interrupt Exception handler. */
static void fiq_exception_entry(void)
{
	PROCESS_EXCEPTION(EXC_FIQ);
}

/** Low-level Prefetch Abort Exception handler. */
static void prefetch_abort_exception_entry(void)
{
	asm volatile (
		"sub lr, lr, #4"
	);
	
	PROCESS_EXCEPTION(EXC_PREFETCH_ABORT);
} 

/** Low-level Data Abort Exception handler. */
static void data_abort_exception_entry(void)
{
	asm volatile (
		"sub lr, lr, #8"
	);
	
	PROCESS_EXCEPTION(EXC_DATA_ABORT);
}

/** Low-level Interrupt Exception handler.
 *
 * CPU is switched to Undefined mode before further interrupt processing
 * because of possible occurence of nested interrupt exception, which
 * would overwrite (and thus spoil) stack pointer.
 */
static void irq_exception_entry(void)
{
	asm volatile (
		"sub lr, lr, #4"
	);
	
	setup_stack_and_save_regs();
	
	switch_to_irq_servicing_mode();
	
	CALL_EXC_DISPATCH(EXC_IRQ)

	load_regs();
}

/** Software Interrupt handler.
 *
 * Dispatches the syscall.
 */
static void swi_exception(int exc_no, istate_t *istate)
{
	istate->r0 = syscall_handler(istate->r0, istate->r1, istate->r2,
	    istate->r3, istate->r4, istate->r5, istate->r6);
}

/** Returns the mask of active interrupts. */
static inline uint32_t gxemul_irqc_get_sources(void)
{
	return *((uint32_t *) gxemul_irqc);
}

/** Interrupt Exception handler.
 *
 * Determines the sources of interrupt and calls their handlers.
 */
static void irq_exception(int exc_no, istate_t *istate)
{
	uint32_t sources = gxemul_irqc_get_sources();
	unsigned int i;
	
	for (i = 0; i < GXEMUL_IRQC_MAX_IRQ; i++) {
		if (sources & (1 << i)) {
			irq_t *irq = irq_dispatch_and_lock(i);
			if (irq) {
				/* The IRQ handler was found. */
				irq->handler(irq);
				spinlock_unlock(&irq->lock);
			} else {
				/* Spurious interrupt.*/
				printf("cpu%d: spurious interrupt (inum=%d)\n",
				    CPU->id, i);
			}
		}
	}
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
	
	exc_register(EXC_IRQ, "interrupt", (iroutine) irq_exception);
	exc_register(EXC_PREFETCH_ABORT, "prefetch abort",
	    (iroutine) prefetch_abort);
	exc_register(EXC_DATA_ABORT, "data abort", (iroutine) data_abort);
	exc_register(EXC_SWI, "software interrupt", (iroutine) swi_exception);
}

/** Prints #istate_t structure content.
 *
 * @param istate Structure to be printed.
 */
void print_istate(istate_t *istate)
{
	printf("istate dump:\n");
	
	printf(" r0: %x    r1: %x    r2: %x    r3: %x\n",
	    istate->r0, istate->r1, istate->r2, istate->r3);
	printf(" r4: %x    r5: %x    r6: %x    r7: %x\n", 
	    istate->r4, istate->r5, istate->r6, istate->r7);
	printf(" r8: %x    r8: %x   r10: %x   r11: %x\n", 
	    istate->r8, istate->r9, istate->r10, istate->r11);
	printf(" r12: %x    sp: %x    lr: %x  spsr: %x\n",
	    istate->r12, istate->sp, istate->lr, istate->spsr);
	
	printf(" pc: %x\n", istate->pc);
}

/** @}
 */
