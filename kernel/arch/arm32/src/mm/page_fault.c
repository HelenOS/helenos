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

/** @addtogroup kernel_arm32_mm
 * @{
 */
/** @file
 *  @brief Page fault related functions.
 */
#include <panic.h>
#include <arch/cp15.h>
#include <arch/exception.h>
#include <arch/mm/page_fault.h>
#include <mm/as.h>
#include <genarch/mm/page_pt.h>
#include <arch.h>
#include <interrupt.h>

/**
 * FSR encoding ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition.
 *
 * B3.13.3 page B3-1406 (PDF page 1406)
 */
typedef enum {
	DFSR_SOURCE_ALIGN = 0x0001,
	DFSR_SOURCE_CACHE_MAINTENANCE = 0x0004,
	DFSR_SOURCE_SYNC_EXTERNAL_TRANSLATION_L1 = 0x000c,
	DFSR_SOURCE_SYNC_EXTERNAL_TRANSLATION_L2 = 0x000e,
	DFSR_SOURCE_SYNC_PARITY_TRANSLATION_L1 = 0x040c,
	DFSR_SOURCE_SYNC_PARITY_TRANSLATION_L2 = 0x040e,
	DFSR_SOURCE_TRANSLATION_L1 = 0x0005,
	DFSR_SOURCE_TRANSLATION_L2 = 0x0007,
	DFSR_SOURCE_ACCESS_FLAG_L1 = 0x0003,  /**< @note: This used to be alignment enc. */
	DFSR_SOURCE_ACCESS_FLAG_L2 = 0x0006,
	DFSR_SOURCE_DOMAIN_L1 = 0x0009,
	DFSR_SOURCE_DOMAIN_L2 = 0x000b,
	DFSR_SOURCE_PERMISSION_L1 = 0x000d,
	DFSR_SOURCE_PERMISSION_L2 = 0x000f,
	DFSR_SOURCE_DEBUG = 0x0002,
	DFSR_SOURCE_SYNC_EXTERNAL = 0x0008,
	DFSR_SOURCE_TLB_CONFLICT = 0x0400,
	DFSR_SOURCE_LOCKDOWN = 0x0404, /**< @note: Implementation defined */
	DFSR_SOURCE_COPROCESSOR = 0x040a, /**< @note Implementation defined */
	DFSR_SOURCE_SYNC_PARITY = 0x0409,
	DFSR_SOURCE_ASYNC_EXTERNAL = 0x0406,
	DFSR_SOURCE_ASYNC_PARITY = 0x0408,
	DFSR_SOURCE_MASK = 0x0000040f,
} dfsr_source_t;

static inline const char *dfsr_source_to_str(dfsr_source_t source)
{
	switch (source)	{
	case DFSR_SOURCE_TRANSLATION_L1:
		return "Translation fault L1";
	case DFSR_SOURCE_TRANSLATION_L2:
		return "Translation fault L2";
	case DFSR_SOURCE_PERMISSION_L1:
		return "Permission fault L1";
	case DFSR_SOURCE_PERMISSION_L2:
		return "Permission fault L2";
	case DFSR_SOURCE_ALIGN:
		return "Alignment fault";
	case DFSR_SOURCE_CACHE_MAINTENANCE:
		return "Instruction cache maintenance fault";
	case DFSR_SOURCE_SYNC_EXTERNAL_TRANSLATION_L1:
		return "Synchronous external abort on translation table walk level 1";
	case DFSR_SOURCE_SYNC_EXTERNAL_TRANSLATION_L2:
		return "Synchronous external abort on translation table walk level 2";
	case DFSR_SOURCE_SYNC_PARITY_TRANSLATION_L1:
		return "Synchronous parity error on translation table walk level 1";
	case DFSR_SOURCE_SYNC_PARITY_TRANSLATION_L2:
		return "Synchronous parity error on translation table walk level 2";
	case DFSR_SOURCE_ACCESS_FLAG_L1:
		return "Access flag fault L1";
	case DFSR_SOURCE_ACCESS_FLAG_L2:
		return "Access flag fault L2";
	case DFSR_SOURCE_DOMAIN_L1:
		return "Domain fault L1";
	case DFSR_SOURCE_DOMAIN_L2:
		return "Domain flault L2";
	case DFSR_SOURCE_DEBUG:
		return "Debug event";
	case DFSR_SOURCE_SYNC_EXTERNAL:
		return "Synchronous external abort";
	case DFSR_SOURCE_TLB_CONFLICT:
		return "TLB conflict abort";
	case DFSR_SOURCE_LOCKDOWN:
		return "Lockdown (Implementation defined)";
	case DFSR_SOURCE_COPROCESSOR:
		return "Coprocessor abort (Implementation defined)";
	case DFSR_SOURCE_SYNC_PARITY:
		return "Synchronous parity error on memory access";
	case DFSR_SOURCE_ASYNC_EXTERNAL:
		return "Asynchronous external abort";
	case DFSR_SOURCE_ASYNC_PARITY:
		return "Asynchronous parity error on memory access";
	case DFSR_SOURCE_MASK:
		break;
	}
	return "Unknown data abort";
}

#if defined(PROCESSOR_ARCH_armv4) | defined(PROCESSOR_ARCH_armv5)
/** Decides whether read or write into memory is requested.
 *
 * @param instr_addr   Address of instruction which tries to access memory.
 * @param badvaddr     Virtual address the instruction tries to access.
 *
 * @return Type of access into memory, PF_ACCESS_EXEC if no memory access is
 *	   requested.
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
		    *(uint32_t *)instr_union.instr, (void *) badvaddr);
		return PF_ACCESS_EXEC;
	}

	/*
	 * See ARM Architecture reference manual ARMv7-A and ARMV7-R edition
	 * A5.3 (PDF p. 206)
	 */
	static const struct {
		uint32_t mask;
		uint32_t value;
		pf_access_t access;
	} ls_inst[] = {
		/* Store word/byte */
		{ 0x0e100000, 0x04000000, PF_ACCESS_WRITE }, /* STR(B) imm */
		{ 0x0e100010, 0x06000000, PF_ACCESS_WRITE }, /* STR(B) reg */
		/* Load word/byte */
		{ 0x0e100000, 0x04100000, PF_ACCESS_READ }, /* LDR(B) imm */
		{ 0x0e100010, 0x06100000, PF_ACCESS_READ }, /* LDR(B) reg */
		/* Store half-word/dual  A5.2.8 */
		{ 0x0e1000b0, 0x000000b0, PF_ACCESS_WRITE }, /* STRH imm reg */
		/* Load half-word/dual A5.2.8 */
		{ 0x0e0000f0, 0x000000d0, PF_ACCESS_READ }, /* LDRH imm reg */
		{ 0x0e1000b0, 0x001000b0, PF_ACCESS_READ }, /* LDRH imm reg */
		/* Block data transfer, Store */
		{ 0x0e100000, 0x08000000, PF_ACCESS_WRITE }, /* STM variants */
		{ 0x0e100000, 0x08100000, PF_ACCESS_READ },  /* LDM variants */
		/* Swap */
		{ 0x0fb00000, 0x01000000, PF_ACCESS_WRITE },
	};
	const uint32_t inst = *(uint32_t *)instr_addr;
	for (unsigned i = 0; i < sizeof(ls_inst) / sizeof(ls_inst[0]); ++i) {
		if ((inst & ls_inst[i].mask) == ls_inst[i].value) {
			return ls_inst[i].access;
		}
	}

	panic("page_fault - instruction doesn't access memory "
	    "(instr_code: %#0" PRIx32 ", badvaddr:%p).",
	    inst, (void *) badvaddr);
}
#endif

/** Handles "data abort" exception (load or store at invalid address).
 *
 * @param exc_no Exception number.
 * @param istate CPU state when exception occured.
 *
 */
void data_abort(unsigned int exc_no, istate_t *istate)
{
	const uintptr_t badvaddr = DFAR_read();
	const fault_status_t fsr = { .raw = DFSR_read() };
	const dfsr_source_t source = fsr.raw & DFSR_SOURCE_MASK;

	switch (source)	{
	case DFSR_SOURCE_TRANSLATION_L1:
	case DFSR_SOURCE_TRANSLATION_L2:
	case DFSR_SOURCE_PERMISSION_L1:
	case DFSR_SOURCE_PERMISSION_L2:
		/* Page fault is handled further down */
		break;
	case DFSR_SOURCE_ALIGN:
	case DFSR_SOURCE_CACHE_MAINTENANCE:
	case DFSR_SOURCE_SYNC_EXTERNAL_TRANSLATION_L1:
	case DFSR_SOURCE_SYNC_EXTERNAL_TRANSLATION_L2:
	case DFSR_SOURCE_SYNC_PARITY_TRANSLATION_L1:
	case DFSR_SOURCE_SYNC_PARITY_TRANSLATION_L2:
	case DFSR_SOURCE_ACCESS_FLAG_L1:
	case DFSR_SOURCE_ACCESS_FLAG_L2:
	case DFSR_SOURCE_DOMAIN_L1:
	case DFSR_SOURCE_DOMAIN_L2:
	case DFSR_SOURCE_DEBUG:
	case DFSR_SOURCE_SYNC_EXTERNAL:
	case DFSR_SOURCE_TLB_CONFLICT:
	case DFSR_SOURCE_LOCKDOWN:
	case DFSR_SOURCE_COPROCESSOR:
	case DFSR_SOURCE_SYNC_PARITY:
	case DFSR_SOURCE_ASYNC_EXTERNAL:
	case DFSR_SOURCE_ASYNC_PARITY:
	case DFSR_SOURCE_MASK:
		/* Weird abort stuff */
		fault_if_from_uspace(istate, "Unhandled abort %s at address: "
		    "%#x.", dfsr_source_to_str(source), badvaddr);
		panic("Unhandled abort %s at address: %#x.",
		    dfsr_source_to_str(source), badvaddr);
	}

#if defined(PROCESSOR_ARCH_armv6) | defined(PROCESSOR_ARCH_armv7_a)
	const pf_access_t access =
	    fsr.data.wr ? PF_ACCESS_WRITE : PF_ACCESS_READ;
#elif defined(PROCESSOR_ARCH_armv4) | defined(PROCESSOR_ARCH_armv5)
	const pf_access_t access = get_memory_access_type(istate->pc, badvaddr);
#else
#error "Unsupported architecture"
#endif
	as_page_fault(badvaddr, access, istate);
}

/** Handles "prefetch abort" exception (instruction couldn't be executed).
 *
 * @param exc_no Exception number.
 * @param istate CPU state when exception occured.
 *
 */
void prefetch_abort(unsigned int exc_no, istate_t *istate)
{
	as_page_fault(istate->pc, PF_ACCESS_EXEC, istate);
}

/** @}
 */
