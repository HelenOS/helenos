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

#ifndef __sparc64_ASM_H__
#define __sparc64_ASM_H__

#include <typedefs.h>
#include <arch/types.h>
#include <arch/register.h>
#include <config.h>

/** Read Processor State register.
 *
 * @return Value of PSTATE register.
 */
static inline __u64 pstate_read(void)
{
	__u64 v;
	
	__asm__ volatile ("rdpr %%pstate, %0\n" : "=r" (v));
	
	return v;
}

/** Write Processor State register.
 *
 * @param New value of PSTATE register.
 */
static inline void pstate_write(__u64 v)
{
	__asm__ volatile ("wrpr %0, %1, %%pstate\n" : : "r" (v), "i" (0));
}

/** Read TICK_compare Register.
 *
 * @return Value of TICK_comapre register.
 */
static inline __u64 tick_compare_read(void)
{
	__u64 v;
	
	__asm__ volatile ("rd %%tick_cmpr, %0\n" : "=r" (v));
	
	return v;
}

/** Write TICK_compare Register.
 *
 * @param New value of TICK_comapre register.
 */
static inline void tick_compare_write(__u64 v)
{
	__asm__ volatile ("wr %0, %1, %%tick_cmpr\n" : : "r" (v), "i" (0));
}

/** Read TICK Register.
 *
 * @return Value of TICK register.
 */
static inline __u64 tick_read(void)
{
	__u64 v;
	
	__asm__ volatile ("rdpr %%tick, %0\n" : "=r" (v));
	
	return v;
}

/** Write TICK Register.
 *
 * @param New value of TICK register.
 */
static inline void tick_write(__u64 v)
{
	__asm__ volatile ("wrpr %0, %1, %%tick\n" : : "r" (v), "i" (0));
}

/** Read SOFTINT Register.
 *
 * @return Value of SOFTINT register.
 */
static inline __u64 softint_read(void)
{
	__u64 v;

	__asm__ volatile ("rd %%softint, %0\n" : "=r" (v));

	return v;
}

/** Write SOFTINT Register.
 *
 * @param New value of SOFTINT register.
 */
static inline void softint_write(__u64 v)
{
	__asm__ volatile ("wr %0, %1, %%softint\n" : : "r" (v), "i" (0));
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
	__u64 value;
	
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
	__u64 value;
	
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
static inline __address get_stack_base(void)
{
	__address v;
	
	__asm__ volatile ("and %%sp, %1, %0\n" : "=r" (v) : "r" (~(STACK_SIZE-1)));
	
	return v;
}

/** Read Version Register.
 *
 * @return Value of VER register.
 */
static inline __u64 ver_read(void)
{
	__u64 v;
	
	__asm__ volatile ("rdpr %%ver, %0\n" : "=r" (v));
	
	return v;
}

/** Read Trap Base Address register.
 *
 * @return Current value in TBA.
 */
static inline __u64 tba_read(void)
{
	__u64 v;
	
	__asm__ volatile ("rdpr %%tba, %0\n" : "=r" (v));
	
	return v;
}

/** Write Trap Base Address register.
 *
 * @param New value of TBA.
 */
static inline void tba_write(__u64 v)
{
	__asm__ volatile ("wrpr %0, %1, %%tba\n" : : "r" (v), "i" (0));
}

/** Load __u64 from alternate space.
 *
 * @param asi ASI determining the alternate space.
 * @param va Virtual address within the ASI.
 *
 * @return Value read from the virtual address in the specified address space.
 */
static inline __u64 asi_u64_read(asi_t asi, __address va)
{
	__u64 v;
	
	__asm__ volatile ("ldxa [%1] %2, %0\n" : "=r" (v) : "r" (va), "i" (asi));
	
	return v;
}

/** Store __u64 to alternate space.
 *
 * @param asi ASI determining the alternate space.
 * @param va Virtual address within the ASI.
 * @param v Value to be written.
 */
static inline void asi_u64_write(asi_t asi, __address va, __u64 v)
{
	__asm__ volatile ("stxa %0, [%1] %2\n" : :  "r" (v), "r" (va), "i" (asi) : "memory");
}



void cpu_halt(void);
void cpu_sleep(void);
void asm_delay_loop(__u32 t);

#endif
