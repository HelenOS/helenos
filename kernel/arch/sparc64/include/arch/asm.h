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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_ASM_H_
#define KERN_sparc64_ASM_H_

#include <typedefs.h>
#include <align.h>
#include <arch/register.h>
#include <config.h>
#include <arch/stack.h>
#include <arch/barrier.h>
#include <trace.h>

NO_TRACE static inline void pio_write_8(ioport8_t *port, uint8_t v)
{
	*port = v;
	memory_barrier();
}

NO_TRACE static inline void pio_write_16(ioport16_t *port, uint16_t v)
{
	*port = v;
	memory_barrier();
}

NO_TRACE static inline void pio_write_32(ioport32_t *port, uint32_t v)
{
	*port = v;
	memory_barrier();
}

NO_TRACE static inline uint8_t pio_read_8(ioport8_t *port)
{
	uint8_t rv = *port;
	memory_barrier();
	return rv;
}

NO_TRACE static inline uint16_t pio_read_16(ioport16_t *port)
{
	uint16_t rv = *port;
	memory_barrier();
	return rv;
}

NO_TRACE static inline uint32_t pio_read_32(ioport32_t *port)
{
	uint32_t rv = *port;
	memory_barrier();
	return rv;
}

/** Read Processor State register.
 *
 * @return Value of PSTATE register.
 *
 */
NO_TRACE static inline uint64_t pstate_read(void)
{
	uint64_t v;

	asm volatile (
		"rdpr %%pstate, %[v]\n"
		: [v] "=r" (v)
	);

	return v;
}

/** Write Processor State register.
 *
 * @param v New value of PSTATE register.
 *
 */
NO_TRACE static inline void pstate_write(uint64_t v)
{
	asm volatile (
		"wrpr %[v], %[zero], %%pstate\n"
		:: [v] "r" (v),
		   [zero] "i" (0)
	);
}

/** Read TICK_compare Register.
 *
 * @return Value of TICK_comapre register.
 *
 */
NO_TRACE static inline uint64_t tick_compare_read(void)
{
	uint64_t v;

	asm volatile (
		"rd %%tick_cmpr, %[v]\n"
		: [v] "=r" (v)
	);

	return v;
}

/** Write TICK_compare Register.
 *
 * @param v New value of TICK_comapre register.
 *
 */
NO_TRACE static inline void tick_compare_write(uint64_t v)
{
	asm volatile (
		"wr %[v], %[zero], %%tick_cmpr\n"
		:: [v] "r" (v),
		   [zero] "i" (0)
	);
}

/** Read STICK_compare Register.
 *
 * @return Value of STICK_compare register.
 *
 */
NO_TRACE static inline uint64_t stick_compare_read(void)
{
	uint64_t v;

	asm volatile (
		"rd %%asr25, %[v]\n"
		: [v] "=r" (v)
	);

	return v;
}

/** Write STICK_compare Register.
 *
 * @param v New value of STICK_comapre register.
 *
 */
NO_TRACE static inline void stick_compare_write(uint64_t v)
{
	asm volatile (
		"wr %[v], %[zero], %%asr25\n"
		:: [v] "r" (v),
		   [zero] "i" (0)
	);
}

/** Read TICK Register.
 *
 * @return Value of TICK register.
 *
 */
NO_TRACE static inline uint64_t tick_read(void)
{
	uint64_t v;

	asm volatile (
		"rdpr %%tick, %[v]\n"
		: [v] "=r" (v)
	);

	return v;
}

/** Write TICK Register.
 *
 * @param v New value of TICK register.
 *
 */
NO_TRACE static inline void tick_write(uint64_t v)
{
	asm volatile (
		"wrpr %[v], %[zero], %%tick\n"
		:: [v] "r" (v),
		   [zero] "i" (0)
	);
}

/** Read FPRS Register.
 *
 * @return Value of FPRS register.
 *
 */
NO_TRACE static inline uint64_t fprs_read(void)
{
	uint64_t v;

	asm volatile (
		"rd %%fprs, %[v]\n"
		: [v] "=r" (v)
	);

	return v;
}

/** Write FPRS Register.
 *
 * @param v New value of FPRS register.
 *
 */
NO_TRACE static inline void fprs_write(uint64_t v)
{
	asm volatile (
		"wr %[v], %[zero], %%fprs\n"
		:: [v] "r" (v),
		   [zero] "i" (0)
	);
}

/** Read SOFTINT Register.
 *
 * @return Value of SOFTINT register.
 *
 */
NO_TRACE static inline uint64_t softint_read(void)
{
	uint64_t v;

	asm volatile (
		"rd %%softint, %[v]\n"
		: [v] "=r" (v)
	);

	return v;
}

/** Write SOFTINT Register.
 *
 * @param v New value of SOFTINT register.
 *
 */
NO_TRACE static inline void softint_write(uint64_t v)
{
	asm volatile (
		"wr %[v], %[zero], %%softint\n"
		:: [v] "r" (v),
		   [zero] "i" (0)
	);
}

/** Write CLEAR_SOFTINT Register.
 *
 * Bits set in CLEAR_SOFTINT register will be cleared in SOFTINT register.
 *
 * @param v New value of CLEAR_SOFTINT register.
 *
 */
NO_TRACE static inline void clear_softint_write(uint64_t v)
{
	asm volatile (
		"wr %[v], %[zero], %%clear_softint\n"
		:: [v] "r" (v),
		   [zero] "i" (0)
	);
}

/** Write SET_SOFTINT Register.
 *
 * Bits set in SET_SOFTINT register will be set in SOFTINT register.
 *
 * @param v New value of SET_SOFTINT register.
 *
 */
NO_TRACE static inline void set_softint_write(uint64_t v)
{
	asm volatile (
		"wr %[v], %[zero], %%set_softint\n"
		:: [v] "r" (v),
		   [zero] "i" (0)
	);
}

/** Enable interrupts.
 *
 * Enable interrupts and return previous
 * value of IPL.
 *
 * @return Old interrupt priority level.
 *
 */
NO_TRACE static inline ipl_t interrupts_enable(void) {
	pstate_reg_t pstate;
	uint64_t value = pstate_read();

	pstate.value = value;
	pstate.ie = true;
	pstate_write(pstate.value);

	return (ipl_t) value;
}

/** Disable interrupts.
 *
 * Disable interrupts and return previous
 * value of IPL.
 *
 * @return Old interrupt priority level.
 *
 */
NO_TRACE static inline ipl_t interrupts_disable(void) {
	pstate_reg_t pstate;
	uint64_t value = pstate_read();

	pstate.value = value;
	pstate.ie = false;
	pstate_write(pstate.value);

	return (ipl_t) value;
}

/** Restore interrupt priority level.
 *
 * Restore IPL.
 *
 * @param ipl Saved interrupt priority level.
 *
 */
NO_TRACE static inline void interrupts_restore(ipl_t ipl) {
	pstate_reg_t pstate;

	pstate.value = pstate_read();
	pstate.ie = ((pstate_reg_t)(uint64_t) ipl).ie;
	pstate_write(pstate.value);
}

/** Return interrupt priority level.
 *
 * Return IPL.
 *
 * @return Current interrupt priority level.
 *
 */
NO_TRACE static inline ipl_t interrupts_read(void) {
	return (ipl_t) pstate_read();
}

/** Check interrupts state.
 *
 * @return True if interrupts are disabled.
 *
 */
NO_TRACE static inline bool interrupts_disabled(void)
{
	pstate_reg_t pstate;

	pstate.value = pstate_read();
	return !pstate.ie;
}

/** Return base address of current stack.
 *
 * Return the base address of the current stack.
 * The stack is assumed to be STACK_SIZE bytes long.
 * The stack must start on page boundary.
 *
 */
NO_TRACE static inline uintptr_t get_stack_base(void)
{
	uintptr_t unbiased_sp;

	asm volatile (
		"add %%sp, %[stack_bias], %[unbiased_sp]\n"
		: [unbiased_sp] "=r" (unbiased_sp)
		: [stack_bias] "i" (STACK_BIAS)
	);

	return ALIGN_DOWN(unbiased_sp, STACK_SIZE);
}

/** Read Version Register.
 *
 * @return Value of VER register.
 *
 */
NO_TRACE static inline uint64_t ver_read(void)
{
	uint64_t v;

	asm volatile (
		"rdpr %%ver, %[v]\n"
		: [v] "=r" (v)
	);

	return v;
}

/** Read Trap Program Counter register.
 *
 * @return Current value in TPC.
 *
 */
NO_TRACE static inline uint64_t tpc_read(void)
{
	uint64_t v;

	asm volatile (
		"rdpr %%tpc, %[v]\n"
		: [v] "=r" (v)
	);

	return v;
}

/** Read Trap Level register.
 *
 * @return Current value in TL.
 *
 */
NO_TRACE static inline uint64_t tl_read(void)
{
	uint64_t v;

	asm volatile (
		"rdpr %%tl, %[v]\n"
		: [v] "=r" (v)
	);

	return v;
}

/** Read Trap Base Address register.
 *
 * @return Current value in TBA.
 *
 */
NO_TRACE static inline uint64_t tba_read(void)
{
	uint64_t v;

	asm volatile (
		"rdpr %%tba, %[v]\n"
		: [v] "=r" (v)
	);

	return v;
}

/** Write Trap Base Address register.
 *
 * @param v New value of TBA.
 *
 */
NO_TRACE static inline void tba_write(uint64_t v)
{
	asm volatile (
		"wrpr %[v], %[zero], %%tba\n"
		:: [v] "r" (v),
		   [zero] "i" (0)
	);
}

/** Load uint64_t from alternate space.
 *
 * @param asi ASI determining the alternate space.
 * @param va  Virtual address within the ASI.
 *
 * @return Value read from the virtual address in
 *         the specified address space.
 *
 */
NO_TRACE static inline uint64_t asi_u64_read(asi_t asi, uintptr_t va)
{
	uint64_t v;

	asm volatile (
		"ldxa [%[va]] %[asi], %[v]\n"
		: [v] "=r" (v)
		: [va] "r" (va),
		  [asi] "i" ((unsigned int) asi)
	);

	return v;
}

/** Store uint64_t to alternate space.
 *
 * @param asi ASI determining the alternate space.
 * @param va  Virtual address within the ASI.
 * @param v   Value to be written.
 *
 */
NO_TRACE static inline void asi_u64_write(asi_t asi, uintptr_t va, uint64_t v)
{
	asm volatile (
		"stxa %[v], [%[va]] %[asi]\n"
		:: [v] "r" (v),
		   [va] "r" (va),
		   [asi] "i" ((unsigned int) asi)
		: "memory"
	);
}

/** Flush all valid register windows to memory. */
NO_TRACE static inline void flushw(void)
{
	asm volatile ("flushw\n");
}

/** Switch to nucleus by setting TL to 1. */
NO_TRACE static inline void nucleus_enter(void)
{
	asm volatile ("wrpr %g0, 1, %tl\n");
}

/** Switch from nucleus by setting TL to 0. */
NO_TRACE static inline void nucleus_leave(void)
{
	asm volatile ("wrpr %g0, %g0, %tl\n");
}

extern void cpu_halt(void) __attribute__((noreturn));
extern void cpu_sleep(void);
extern void asm_delay_loop(const uint32_t usec);

extern uint64_t read_from_ag_g6(void);
extern uint64_t read_from_ag_g7(void);
extern void write_to_ag_g6(uint64_t val);
extern void write_to_ag_g7(uint64_t val);
extern void write_to_ig_g6(uint64_t val);

extern void switch_to_userspace(uint64_t pc, uint64_t sp, uint64_t uarg);

#endif

/** @}
 */
