#
# Copyright (c) 2007 Michal Kebrt
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

.section BOOTSTRAP

#define CP15_C1_U		22
#define CP15_C1_IC		12
#define CP15_C1_BP		11
#define CP15_C1_DC		2

SYMBOL(start)

#ifdef PROCESSOR_ARCH_armv6
	/*
	 * Enable unaligned doubleword memory accesses (STRD/LDRD) if the
	 * processor supports it. Note that that boils down to ARMv6 processors
	 * only as the older architectures require doubleword alignment and
	 * ARMv7 always assumes the U bit is 1.
	 */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #(1 << CP15_C1_U)
	mcr	p15, 0, r0, c1, c0, 0
#endif

	ldr sp, =boot_stack
	b bootstrap

.section BOOTPT
SYMBOL(boot_pt)
	.space PTL0_ENTRIES * PTL0_ENTRY_SIZE

.section BOOTSTACK
	.space 4096
SYMBOL(boot_stack)

.text

FUNCTION_BEGIN(halt)
	b halt
FUNCTION_END(halt)

FUNCTION_BEGIN(jump_to_kernel)
	#
	# Make sure that the I-cache, D-cache and memory are mutually coherent
	# before passing control to the copied code.
	#

	#
	# r0 is kernel entry point
	# r1 is pointer to the bootinfo structure

#ifndef PROCESSOR_ARCH_armv7_a
	mrc	p15, 0, r4, c1, c0, 0

	# Disable D-cache before the kernel is started.
	bic	r4, r4, #(1 << CP15_C1_DC)

	# Disable I-cache and Branch predictors.
	bic	r4, r4, #(1 << CP15_C1_IC)
#ifdef PROCESSOR_ARCH_armv6
	bic	r4, r4, #(1 << CP15_C1_BP)
#endif

	mcr	p15, 0, r4, c1, c0, 0
#endif

	# Wait for the operations to complete
#ifdef PROCESSOR_ARCH_armv7_a
	dsb
#else
	# cp15 dsb, r4 is ignored (should be zero)
	mov r4, #0
	mcr p15, 0, r4, c7, c10, 4
#endif

	# Clean ICache and BPredictors, r4 ignored (SBZ)
	mov r4, #0
	mcr p15, 0, r4, c7, c5, 0
	nop

	# Wait for the operations to complete
#ifdef PROCESSOR_ARCH_armv7_a
	isb
	nop
#elif defined(PROCESSOR_ARCH_armv6)
	# cp15 isb
	mcr p15, 0, r4, c7, c5, 4
	nop
#endif
	mov pc, r0
FUNCTION_END(jump_to_kernel)

