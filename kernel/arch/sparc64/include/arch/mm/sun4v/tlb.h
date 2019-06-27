/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2008 Pavel Rimsky
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

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_TLB_H_
#define KERN_sparc64_sun4v_TLB_H_

#define MMU_FSA_ALIGNMENT	64
#define MMU_FSA_SIZE		128

#ifndef __ASSEMBLER__

#include <arch/mm/tte.h>
#include <trace.h>
#include <arch/mm/mmu.h>
#include <arch/mm/page.h>
#include <arch/asm.h>
#include <barrier.h>
#include <typedefs.h>
#include <arch/register.h>
#include <arch/cpu.h>
#include <arch/sun4v/hypercall.h>

/**
 * Structure filled by hypervisor (or directly CPU, if implemented so) when
 * a MMU fault occurs. The structure describes the exact condition which
 * has caused the fault.
 */
typedef struct mmu_fault_status_area {
	uint64_t ift;		/**< Instruction fault type (IFT) */
	uint64_t ifa;		/**< Instruction fault address (IFA) */
	uint64_t ifc;		/**< Instruction fault context (IFC) */
	uint8_t reserved1[0x28];

	uint64_t dft;		/**< Data fault type (DFT) */
	uint64_t dfa;		/**< Data fault address (DFA) */
	uint64_t dfc;		/**< Data fault context (DFC) */
	uint8_t reserved2[0x28];
} __attribute__((packed)) mmu_fault_status_area_t;

#define DTLB_MAX_LOCKED_ENTRIES		8

/** Bit width of the TLB-locked portion of kernel address space. */
#define KERNEL_PAGE_WIDTH       22	/* 4M */

/*
 * Reading and writing context registers.
 *
 * Note that UltraSPARC Architecture-compatible processors do not require
 * a MEMBAR #Sync, FLUSH, DONE, or RETRY instruction after a store to an
 * MMU register for proper operation.
 *
 */

/** Read MMU Primary Context Register.
 *
 * @return	Current value of Primary Context Register.
 */
_NO_TRACE static inline uint64_t mmu_primary_context_read(void)
{
	return asi_u64_read(ASI_PRIMARY_CONTEXT_REG, VA_PRIMARY_CONTEXT_REG);
}

/** Write MMU Primary Context Register.
 *
 * @param v	New value of Primary Context Register.
 */
_NO_TRACE static inline void mmu_primary_context_write(uint64_t v)
{
	asi_u64_write(ASI_PRIMARY_CONTEXT_REG, VA_PRIMARY_CONTEXT_REG, v);
}

/** Read MMU Secondary Context Register.
 *
 * @return	Current value of Secondary Context Register.
 */
_NO_TRACE static inline uint64_t mmu_secondary_context_read(void)
{
	return asi_u64_read(ASI_SECONDARY_CONTEXT_REG, VA_SECONDARY_CONTEXT_REG);
}

/** Write MMU Secondary Context Register.
 *
 * @param v	New value of Secondary Context Register.
 */
_NO_TRACE static inline void mmu_secondary_context_write(uint64_t v)
{
	asi_u64_write(ASI_SECONDARY_CONTEXT_REG, VA_SECONDARY_CONTEXT_REG, v);
}

/**
 * Demaps all mappings in a context.
 *
 * @param context	number of the context
 * @param mmu_flag	MMU_FLAG_DTLB, MMU_FLAG_ITLB or a combination of both
 */
_NO_TRACE static inline void mmu_demap_ctx(int context, int mmu_flag)
{
	__hypercall_fast4(MMU_DEMAP_CTX, 0, 0, context, mmu_flag);
}

/**
 * Demaps given page.
 *
 * @param vaddr		VA of the page to be demapped
 * @param context	number of the context
 * @param mmu_flag	MMU_FLAG_DTLB, MMU_FLAG_ITLB or a combination of both
 */
_NO_TRACE static inline void mmu_demap_page(uintptr_t vaddr, int context, int mmu_flag)
{
	__hypercall_fast5(MMU_DEMAP_PAGE, 0, 0, vaddr, context, mmu_flag);
}

extern void fast_instruction_access_mmu_miss(unsigned int, istate_t *);
extern void fast_data_access_mmu_miss(unsigned int, istate_t *);
extern void fast_data_access_protection(unsigned int, istate_t *);

extern void dtlb_insert_mapping(uintptr_t, uintptr_t, int, bool, bool);

extern void describe_dmmu_fault(void);

#endif /* !def __ASSEMBLER__ */

#endif

/** @}
 */
