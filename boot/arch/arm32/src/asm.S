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

#include <arch/arch.h>

.section BOOTSTRAP

.global start
.global boot_pt
.global boot_stack
.global halt
.global jump_to_kernel

start:
	ldr sp, =boot_stack
	b bootstrap

.section BOOTPT
boot_pt:
	.space PTL0_ENTRIES * PTL0_ENTRY_SIZE

.section BOOTSTACK
	.space 4096
boot_stack:

.text

halt:
	b halt

jump_to_kernel:
	#
	# TODO
	# Make sure that the I-cache, D-cache and memory are mutually coherent
	# before passing control to the copied code.
	#

	#
	# r0 is kernel entry point
	# r1 is pointer to the bootinfo structure
	# r2 is a kernel text end

	
	#Wait for the operations to complete
#ifdef PROCESSOR_ARCH_armv7_a
	dsb
#else
	#cp15 dsb, r4 is ignored (should be zero)
	mcr p15, 0, r4, c7, c10, 4
#endif
	
	# Clean ICache and BPredictors, r4 ignored (SBZ)
	mcr p15, 0, r4, c7, c5, 0
	nop

	#Wait for the operations to complete
#ifdef PROCESSOR_ARCH_armv7_a
	isb
	nop
#else
	# cp15 isb
	mcr p15, 0, r4, c7, c5, 4
	nop
#endif
	

#if defined(MACHINE_gta02)

#define CP15_C1_IC		12
#define CP15_C1_DC		2
#define CP15_C7_SEG_SHIFT	5
#define CP15_C7_SEG_SIZE	3
#define CP15_C7_IDX_SHIFT	26

	# Disable I-cache and D-cache before the kernel is started.
	mrc	p15, 0, r4, c1, c0, 0
	bic	r4, r4, #(1 << CP15_C1_DC)
	bic	r4, r4, #(1 << CP15_C1_IC)
	mcr	p15, 0, r4, c1, c0, 0

	# Now clean D-cache to guarantee coherency between I-cache and D-cache.

	# D-cache clean and invalidate procedure.
	# See ARM920T TRM pages 2-17, 4-17.

	# Initialize segment
	mov	r4, #0
	# Initialize index
1:	mov	r5, #0
2:	orr	r6, r4, r5
	# Clean and invalidate a single line
	mcr	p15, 0, r6, c7, c10, 2
	# Increment index
	add	r5, r5, #(1 << CP15_C7_IDX_SHIFT)
	cmp	r5, #0
	bne	2b
	# Increment segment
	add	r4, #(1 << CP15_C7_SEG_SHIFT)
	tst	r4, #(1 << (CP15_C7_SEG_SHIFT + CP15_C7_SEG_SIZE))
	beq	1b
#endif

	mov pc, r0
