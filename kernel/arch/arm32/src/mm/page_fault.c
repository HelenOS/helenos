/*
 * Copyright (c) 2007 Pavel Jancik, Michal Kebrt
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

/** @addtogroup arm32mm
 * @{
 */
/** @file
 *  @brief Page fault related functions.
 */
#include <panic.h>
#include <arch/exception.h>
#include <arch/mm/page_fault.h>
#include <mm/as.h>
#include <genarch/mm/page_pt.h>
#include <arch.h>
#include <interrupt.h>
#include <print.h>

/** Returns value stored in fault status register.
 *
 *  @return Value stored in CP15 fault status register (FSR).
 */
static inline fault_status_t read_fault_status_register(void)
{
	fault_status_union_t fsu;
	
	/* fault status is stored in CP15 register 5 */
	asm volatile (
		"mrc p15, 0, %[dummy], c5, c0, 0"
		: [dummy] "=r" (fsu.dummy)
	);
	
	return fsu.fs;
}

/** Returns FAR (fault address register) content.
 *
 * @return FAR (fault address register) content (address that caused a page
 *         fault)
 */
static inline uintptr_t read_fault_address_register(void)
{
	uintptr_t ret;
	
	/* fault adress is stored in CP15 register 6 */
	asm volatile (
		"mrc p15, 0, %[ret], c6, c0, 0"
		: [ret] "=r" (ret)
	);
	
	return ret;
}

/** Decides whether the instruction is load/store or not.
 *
 * @param instr Instruction
 *
 * @return true when instruction is load/store, false otherwise
 *
 */
static inline bool is_load_store_instruction(instruction_t instr)
{
	/* load store immediate offset */
	if (instr.type == 0x2)
		return true;
	
	/* load store register offset */
	if ((instr.type == 0x3) && (instr.bit4 == 0))
		return true;
	
	/* load store multiple */
	if (instr.type == 0x4)
		return true;
	
	/* oprocessor load/store */
	if (instr.type == 0x6)
		return true;
	
	return false;
}

/** Decides whether the instruction is swap or not.
 *
 * @param instr Instruction
 *
 * @return true when instruction is swap, false otherwise
 */
static inline bool is_swap_instruction(instruction_t instr)
{
	/* swap, swapb instruction */
	if ((instr.type == 0x0) &&
	    ((instr.opcode == 0x8) || (instr.opcode == 0xa)) &&
	    (instr.access == 0x0) && (instr.bits567 == 0x4) && (instr.bit4 == 1))
		return true;
	
	return false;
}

/** Decides whether read or write into memory is requested.
 *
 * @param instr_addr   Address of instruction which tries to access memory.
 * @param badvaddr     Virtual address the instruction tries to access.
 *
 * @return Type of access into memory, PF_ACCESS_EXEC if no memory access is
 * 	   requested.
 */
static pf_access_t get_memory_access_type(uint32_t instr_addr,
    uintptr_t badvaddr)
{
	instruction_union_t instr_union;
	instr_union.pc = instr_addr;

	instruction_t instr = *(instr_union.instr);

	/* undefined instructions */
	if (instr.condition == 0xf) {
		panic("page_fault - instruction does not access memory "
		    "(instr_code: %#0" PRIx32 ", badvaddr:%p).",
		    instr_union.pc, (void *) badvaddr);
		return PF_ACCESS_EXEC;
	}

	/* load store instructions */
	if (is_load_store_instruction(instr)) {
		if (instr.access == 1) {
			return PF_ACCESS_READ;
		} else {
			return PF_ACCESS_WRITE;
		}
	}

	/* swap, swpb instruction */
	if (is_swap_instruction(instr)) {
		return PF_ACCESS_WRITE;
	}

	panic("page_fault - instruction doesn't access memory "
	    "(instr_code: %#0" PRIx32 ", badvaddr:%p).",
	    instr_union.pc, (void *) badvaddr);

	return PF_ACCESS_EXEC;
}

/** Handles "data abort" exception (load or store at invalid address).
 *
 * @param exc_no Exception number.
 * @param istate CPU state when exception occured.
 *
 */
void data_abort(unsigned int exc_no, istate_t *istate)
{
	fault_status_t fsr __attribute__ ((unused)) =
	    read_fault_status_register();
	uintptr_t badvaddr = read_fault_address_register();

	pf_access_t access = get_memory_access_type(istate->pc, badvaddr);

	int ret = as_page_fault(badvaddr, access, istate);

	if (ret == AS_PF_FAULT) {
		fault_if_from_uspace(istate, "Page fault: %#x.", badvaddr);
		panic_memtrap(istate, access, badvaddr, NULL);
	}
}

/** Handles "prefetch abort" exception (instruction couldn't be executed).
 *
 * @param exc_no Exception number.
 * @param istate CPU state when exception occured.
 *
 */
void prefetch_abort(unsigned int exc_no, istate_t *istate)
{
	int ret = as_page_fault(istate->pc, PF_ACCESS_EXEC, istate);

	if (ret == AS_PF_FAULT) {
		fault_if_from_uspace(istate,
		    "Page fault - prefetch_abort: %#x.", istate->pc);
		panic_memtrap(istate, PF_ACCESS_EXEC, istate->pc, NULL);
	}
}

/** @}
 */
