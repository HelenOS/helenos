#
# Copyright (c) 2005 Jakub Jermar
# Copyright (c) 2008 Pavel Rimsky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#include <abi/asmtool.h>
#include <arch/arch.h>
#include <arch/stack.h>
#include <arch/regdef.h>
#include <arch/context_struct.h>
#include <arch/sun4v/regdef.h>
#include <arch/sun4v/hypercall.h>
#include <arch/sun4v/arch.h>
#include <arch/sun4v/cpu.h>
#include <arch/mm/pagesize.h>
#include <arch/mm/sun4v/tte.h>
#include <arch/mm/sun4v/mmu.h>
#include <arch/mm/sun4v/tlb.h>

.register %g2, #scratch
.register %g3, #scratch

.section K_TEXT_START, "ax"

#define BSP_FLAG		1
#define PHYSMEM_ADDR_SIZE	56

/*
 * Flags set in the TTE data entry mapping the kernel.
 */
#ifdef CONFIG_VIRT_IDX_DCACHE
	#define TTE_FLAGS \
		(1 << TTE_V_SHIFT) \
		| (1 << TTE_EP_SHIFT) \
		| (1 << TTE_CP_SHIFT) \
		| (1 << TTE_CV_SHIFT) \
		| (1 << TTE_P_SHIFT) \
		| (1 << TTE_W_SHIFT)
#else
	#define TTE_FLAGS \
		(1 << TTE_V_SHIFT) \
		| (1 << TTE_EP_SHIFT) \
		| (1 << TTE_CP_SHIFT) \
		| (1 << TTE_P_SHIFT) \
		| (1 << TTE_W_SHIFT)
#endif


/*
 * Fills a register with a TTE Data item. The item will map the given virtual
 * address to a real address which will be computed by adding the starting
 * address of the physical memory to the virtual address.
 *
 * parameters:
 *  addr:           virtual address to be mapped
 *  rphysmem_start: register containing the starting address
 *                  of the physical memory
 *  rtmp1:          a register to be used as temporary
 *  rtmp2:          a register to be used as temporary
 *  rd:             register where the result will be saved
 *
 */
#define TTE_DATA(addr, rphysmem_start, rtmp1, rtmp2, rd) \
	setx TTE_FLAGS | PAGESIZE_4M, rtmp1, rd; \
	add rd, rphysmem_start, rd; \
	setx (addr), rtmp1, rtmp2; \
	add rd, rtmp2, rd;

/*
 * Here is where the kernel is passed control from the boot loader.
 *
 * The registers are expected to be in this state:
 *  - %o0 starting address of physical memory
 *        + bootstrap processor flag
 *          bits 63...1: physical memory starting address / 2
 *          bit 0:       non-zero on BSP processor, zero on AP processors
 *  - %o1 bootinfo structure address (BSP only)
 *
 *
 * Moreover, we depend on boot having established the following environment:
 *  - TLBs are on
 *  - identity mapping for the kernel image
 *
 */
SYMBOL(kernel_image_start)
	mov BSP_FLAG, %l0
	and %o0, %l0, %l7			! l7 <= bootstrap processor?
	andn %o0, %l0, %l6			! l6 <= start of physical memory
	or %o1, %g0, %l1

	! Get bits (PHYSMEM_ADDR_SIZE - 1):13 of physmem_base.
	srlx %l6, 13, %l5

	! l5 <= physmem_base[(PHYSMEM_ADDR_SIZE - 1):13]
	sllx %l5, 13 + (63 - (PHYSMEM_ADDR_SIZE - 1)), %l5
	srlx %l5, 63 - (PHYSMEM_ADDR_SIZE - 1), %l5

	/*
	 * Setup basic runtime environment.
	 */
	wrpr %g0, NWINDOWS - 2, %cansave	! set maximum saveable windows
	wrpr %g0, 0, %canrestore		! get rid of windows we will
						! never need again
	wrpr %g0, 0, %otherwin			! make sure the window state is
						! consistent
	wrpr %g0, NWINDOWS - 1, %cleanwin	! prevent needless clean_window
						! traps for kernel

	wrpr %g0, 0, %wstate			! use default spill/fill trap

	wrpr %g0, 0, %tl			! TL = 0, primary context
						! register is used
	wrpr %g0, 0, %gl

	wrpr %g0, PSTATE_PRIV_BIT, %pstate	! disable interrupts and disable
						! 32-bit address masking

	wrpr %g0, 0, %pil			! intialize %pil

	/*
	 * Switch to kernel trap table.
	 */
	sethi %hi(trap_table), %g1
	wrpr %g1, %lo(trap_table), %tba

	/* Explicitly switch to hypervisor API 1.1. */
	mov 1, %o0
   	mov 1, %o1
   	mov 1, %o2
   	mov 0, %o3
   	mov 0, %o4
   	mov 0, %o5
   	ta 0xff

	/*
	 * Take over the MMU.
	 */

	! map kernel in context 1
	set kernel_load_address, %o0				! virt. address
	set 1, %o1						! context
	TTE_DATA(kernel_load_address, %l5, %g2, %g3, %o2)	! TTE data
	set MMU_FLAG_DTLB | MMU_FLAG_ITLB, %o3			! MMU flags
	__HYPERCALL_HYPERFAST(MMU_MAP_ADDR)

	! switch to context 1
	set 1, %o0
	set VA_PRIMARY_CONTEXT_REG, %o1
	stxa %o0, [%o1] ASI_PRIMARY_CONTEXT_REG

	! demap all in context 0
	set 0, %o0						! reserved
	set 0, %o1						! reserved
	set 0, %o2						! context
	set MMU_FLAG_DTLB | MMU_FLAG_ITLB, %o3			! MMU flags
	__HYPERCALL_FAST(MMU_DEMAP_CTX)

	! install permanent mapping for kernel in context 0
	set kernel_load_address, %o0				! virtual address
	set 0, %o1						! context
	TTE_DATA(kernel_load_address, %l5, %g2, %g3, %o2)	! TTE data
	set MMU_FLAG_DTLB | MMU_FLAG_ITLB, %o3			! MMU flags
	__HYPERCALL_FAST(MMU_MAP_PERM_ADDR)

	! switch to context 0
	mov 0, %o0
	set VA_PRIMARY_CONTEXT_REG, %o1
	stxa %o0, [%o1] ASI_PRIMARY_CONTEXT_REG

	! demap all in context 1 (cleanup)
	set 0, %o0						! reserved
	set 0, %o1						! reserved
	set 1, %o2						! context
	set MMU_FLAG_DTLB | MMU_FLAG_ITLB, %o3			! MMU flags
	__HYPERCALL_FAST(MMU_DEMAP_CTX)

	/*
	 * Set CPUID.
	 */
	__HYPERCALL_FAST(CPU_MYID)
	mov SCRATCHPAD_CPUID, %g1
	stxa %o1, [%g1] ASI_SCRATCHPAD

	/*
	 * Set MMU fault status area for the current CPU.
	 */
	set mmu_fsas, %o0			! o0 <= addr. of fault status areas array
	add %o0, %l6, %o0			! kernel address to real address
	mulx %o1, MMU_FSA_SIZE, %g1		! g1 <= offset of current CPU's fault status area
	add %g1, %o0, %o0			! o0 <= FSA of the current CPU
	mov SCRATCHPAD_MMU_FSA, %g1
	stxa %o0, [%g1] ASI_SCRATCHPAD		! remember MMU fault status area to speed up miss handler
	__HYPERCALL_FAST(MMU_FAULT_AREA_CONF)

	! on APs skip executing the following code
	cmp %l7, 0
	be %xcc, 1f
	nop

	/*
	 * Save physmem_base for use by the mm subsystem.
	 * %l6 contains starting physical address
	 */
	sethi %hi(physmem_base), %l4
	stx %l6, [%l4 + %lo(physmem_base)]

	/*
	 * Store a template of a TTE Data entry for kernel mappings.
	 * This template will be used from the kernel MMU miss handler.
	 */
	!TTE_DATA(0, %l5, %g2, %g3, %g1)
	setx TTE_FLAGS | PAGESIZE_8K, %g2, %g1; \
	add %g1, %l5, %g1; \
	set kernel_8k_tlb_data_template, %g4
	stx %g1, [%g4]

	/*
	 * So far, we have not touched the stack.
	 * It is a good idea to set the kernel stack to a known state now.
	 */
	sethi %hi(temporary_boot_stack), %sp
	or %sp, %lo(temporary_boot_stack), %sp
	sub %sp, STACK_BIAS, %sp

	/*
	 * Call sparc64_pre_main(bootinfo)
	 */
	call sparc64_pre_main
	or %l1, %g0, %o0

	/*
	 * Create the first stack frame.
	 */
	save %sp, -(STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE), %sp
	flushw
	add %g0, -STACK_BIAS, %fp

	call main_bsp
	nop

	/* Not reached. */

0:
	ba,a %xcc, 0b

1:

#ifdef CONFIG_SMP

	/*
	 * Configure stack for the AP.
	 * The AP is expected to use the stack saved
	 * in the ctx global variable.
	 */

	mov 1, %o0		! MMU enable flag
	set mmu_enabled, %o1
	mov MMU_ENABLE, %o5	! MMU enable HV call
	ta 0x80			! call HV

	mmu_enabled:

	/*
	 * Configure stack for the AP.
	 * The AP is expected to use the stack saved
	 * in the ctx global variable.
	 */
	set ctx, %g1
	add %g1, CONTEXT_OFFSET_SP, %g1
	ldx [%g1], %o6

	/*
	 * Create the first stack frame.
	 */
	save %sp, -(STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE), %sp
	flushw
	add %g0, -STACK_BIAS, %fp

	call main_ap
	nop
#endif

	/* Not reached. */
0:
	ba,a %xcc, 0b

.section K_DATA_START, "aw", @progbits

#define INITIAL_STACK_SIZE		1024

.align STACK_ALIGNMENT
	.space INITIAL_STACK_SIZE
.align STACK_ALIGNMENT
temporary_boot_stack:
	.space STACK_WINDOW_SAVE_AREA_SIZE


.data

.align 8
SYMBOL(physmem_base)	! copy of the physical memory base address
	.quad 0

/*
 * This variable is used by the fast_data_access_MMU_miss trap handler.
 * In runtime, it is modified to contain the address of the end of physical
 * memory.
 */
SYMBOL(end_of_identity)
	.quad -1

SYMBOL(kernel_8k_tlb_data_template)
	.quad 0

/* MMU fault status areas for all CPUs */
.align MMU_FSA_ALIGNMENT
SYMBOL(mmu_fsas)
	.space (MMU_FSA_SIZE * MAX_NUM_STRANDS)
