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

/** @addtogroup sparc64	
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_ASM_H_
#define KERN_sparc64_ASM_H_

#include <typedefs.h>
#include <arch/types.h>
#include <arch/register.h>
#include <config.h>

/** Read Processor State register.
 *
 * @return Value of PSTATE register.
 */
static inline uint64_t pstate_read(void)
{
	uint64_t v;
	
	__asm__ volatile ("rdpr %%pstate, %0\n" : "=r" (v));
	
	return v;
}

/** Write Processor State register.
 *
 * @param v New value of PSTATE register.
 */
static inline void pstate_write(uint64_t v)
{
	__asm__ volatile ("wrpr %0, %1, %%pstate\n" : : "r" (v), "i" (0));
}

/** Read TICK_compare Register.
 *
 * @return Value of TICK_comapre register.
 */
static inline uint64_t tick_compare_read(void)
{
	uint64_t v;
	
	__asm__ volatile ("rd %%tick_cmpr, %0\n" : "=r" (v));
	
	return v;
}

/** Write TICK_compare Register.
 *
 * @param v New value of TICK_comapre register.
 */
static inline void tick_compare_write(uint64_t v)
{
	__asm__ volatile ("wr %0, %1, %%tick_cmpr\n" : : "r" (v), "i" (0));
}

/** Read TICK Register.
 *
 * @return Value of TICK register.
 */
static inline uint64_t tick_read(void)
{
	uint64_t v;
	
	__asm__ volatile ("rdpr %%tick, %0\n" : "=r" (v));
	
	return v;
}

/** Write TICK Register.
 *
 * @param v New value of TICK register.
 */
static inline void tick_write(uint64_t v)
{
	__asm__ volatile ("wrpr %0, %1, %%tick\n" : : "r" (v), "i" (0));
}

/** Read SOFTINT Register.
 *
 * @return Value of SOFTINT register.
 */
static inline uint64_t softint_read(void)
{
	uint64_t v;

	__asm__ volatile ("rd %%softint, %0\n" : "=r" (v));

	return v;
}

/** Write SOFTINT Register.
 *
 * @param v New value of SOFTINT register.
 */
static inline void softint_write(uint64_t v)
{
	__asm__ volatile ("wr %0, %1, %%softint\n" : : "r" (v), "i" (0));
}

/** Write CLEAR_SOFTINT Register.
 *
 * Bits set in CLEAR_SOFTINT register will be cleared in SOFTINT register.
 *
 * @param v New value of CLEAR_SOFTINT register.
 */
static inline void clear_softint_write(uint64_t v)
{
	__asm__ volatile ("wr %0, %1, %%clear_softint\n" : : "r" (v), "i" (0));
}

/** Write SET_SOFTINT Register.
 *
 * Bits set in SET_SOFTINT register will be set in SOFTINT register.
 *
 * @param v New value of SET_SOFTINT register.
 */
static inline void set_softint_write(uint64_t v)
{
	__asm__ volatile ("wr %0, %1, %%set_softint\n" : : "r" (v), "i" (0));
}

/** Enable interrupts.
 *
 * Enable interrupts and return previous
 * value of IPL.
 *
 * @return Old interrupt priority level.
 */
static inline ipl_t interrupts_enable(void) {
	pstate_reg_t pstate;
	uint64_t value;
	
	value = pstate_read();
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
 */
static inline ipl_t interrupts_disable(void) {
	pstate_reg_t pstate;
	uint64_t value;
	
	value = pstate_read();
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
 */
static inline void interrupts_restore(ipl_t ipl) {
	pstate_reg_t pstate;
	
	pstate.value = pstate_read();
	pstate.ie = ((pstate_reg_t) ipl).ie;
	pstate_write(pstate.value);
}

/** Return interrupt priority level.
 *
 * Return IPL.
 *
 * @return Current interrupt priority level.
 */
static inline ipl_t interrupts_read(void) {
	return (ipl_t) pstate_read();
}

/** Return base address of current stack.
 *
 * Return the base address of the current stack.
 * The stack is assumed to be STACK_SIZE bytes long.
 * The stack must start on page boundary.
 */
static inline uintptr_t get_stack_base(void)
{
	uintptr_t v;
	
	__asm__ volatile ("and %%sp, %1, %0\n" : "=r" (v) : "r" (~(STACK_SIZE-1)));
	
	return v;
}

/** Read Version Register.
 *
 * @return Value of VER register.
 */
static inline uint64_t ver_read(void)
{
	uint64_t v;
	
	__asm__ volatile ("rdpr %%ver, %0\n" : "=r" (v));
	
	return v;
}

/** Read Trap Base Address register.
 *
 * @return Current value in TBA.
 */
static inline uint64_t tba_read(void)
{
	uint64_t v;
	
	__asm__ volatile ("rdpr %%tba, %0\n" : "=r" (v));
	
	return v;
}

/** Read Trap Program Counter register.
 *
 * @return Current value in TPC.
 */
static inline uint64_t tpc_read(void)
{
	uint64_t v;
	
	__asm__ volatile ("rdpr %%tpc, %0\n" : "=r" (v));
	
	return v;
}

/** Read Trap Level register.
 *
 * @return Current value in TL.
 */
static inline uint64_t tl_read(void)
{
	uint64_t v;
	
	__asm__ volatile ("rdpr %%tl, %0\n" : "=r" (v));
	
	return v;
}

/** Write Trap Base Address register.
 *
 * @param v New value of TBA.
 */
static inline void tba_write(uint64_t v)
{
	__asm__ volatile ("wrpr %0, %1, %%tba\n" : : "r" (v), "i" (0));
}

/** Load uint64_t from alternate space.
 *
 * @param asi ASI determining the alternate space.
 * @param va Virtual address within the ASI.
 *
 * @return Value read from the virtual address in the specified address space.
 */
static inline uint64_t asi_u64_read(asi_t asi, uintptr_t va)
{
	uint64_t v;
	
	__asm__ volatile ("ldxa [%1] %2, %0\n" : "=r" (v) : "r" (va), "i" (asi));
	
	return v;
}

/** Store uint64_t to alternate space.
 *
 * @param asi ASI determining the alternate space.
 * @param va Virtual address within the ASI.
 * @param v Value to be written.
 */
static inline void asi_u64_write(asi_t asi, uintptr_t va, uint64_t v)
{
	__asm__ volatile ("stxa %0, [%1] %2\n" : :  "r" (v), "r" (va), "i" (asi) : "memory");
}

void cpu_halt(void);
void cpu_sleep(void);
void asm_delay_loop(uint32_t t);

#endif

/** @}
 */
