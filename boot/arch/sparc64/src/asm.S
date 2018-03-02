#
# Copyright (c) 2006 Martin Decky
# Copyright (c) 2006 Jakub Jermar
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

#if defined(PROCESSOR_us) || defined(PROCESSOR_us3)
#define ICACHE_SIZE       8192
#define ICACHE_LINE_SIZE  32
#define ICACHE_SET_BIT    (1 << 13)
#define ASI_ICACHE_TAG    0x67
#endif	/* PROCESSOR_us || PROCESSOR_us3 */

.register %g2, #scratch
.register %g3, #scratch
.register %g6, #scratch
.register %g7, #scratch

.section BOOTSTRAP, "ax"

SYMBOL(start)
	ba %xcc, 1f
	nop

/*
 * This header forces SILO to load the image at 0x4000.
 * More precisely, SILO will think this is an old version of Linux.
 */
.ascii "HdrS"
.word 0
.half 0
.half 0
.half 0
.half 0
.word 0
.word 0

.align 8
1:
	! Disable interrupts and disable address masking.

	wrpr %g0, PSTATE_PRIV_BIT, %pstate

	wrpr %g0, NWINDOWS - 2, %cansave   ! Set maximum saveable windows
	wrpr %g0, 0, %canrestore           ! Get rid of windows we will never need again
	wrpr %g0, 0, %otherwin             ! Make sure the window state is consistent
	wrpr %g0, NWINDOWS - 1, %cleanwin  ! Prevent needless clean_window traps for kernel

	set initial_stack, %sp
	add %sp, -STACK_BIAS, %sp

	set ofw_cif, %l0

	! Initialize OpenFirmware

	call ofw_init
	stx %o4, [%l0]

	ba %xcc, bootstrap
	nop

.align STACK_ALIGNMENT
	.space STACK_SIZE
initial_stack:
	.space STACK_WINDOW_SAVE_AREA_SIZE

.text

FUNCTION_BEGIN(halt)
	ba %xcc, halt
	nop
FUNCTION_END(halt)

FUNCTION_BEGIN(jump_to_kernel)
	/*
	 * Guarantee cache coherence:
	 * 1. Make sure that the code we have moved has drained to main memory.
	 * 2. Invalidate I-cache.
	 * 3. Flush instruction pipeline.
	 */

	/*
	 * US3 processors have a write-invalidate cache, so explicitly
	 * invalidating it is not required. Whether to invalidate I-cache
	 * or not is decided according to the value of the 3rd argument
	 * (subarch).
	 */
	cmp %o2, SUBARCH_US3
	be %xcc, 1f
	nop

	0:
		call icache_flush
		nop

	1:
		membar #StoreStore

	/*
	 * Flush the instruction pipeline.
	 */
	flush %i7

	! Jump to kernel
	jmp %o3
	nop
FUNCTION_END(jump_to_kernel)

# Flush I-cache
icache_flush:
#if defined(PROCESSOR_us) || defined(PROCESSOR_us3)
	set ((ICACHE_SIZE - ICACHE_LINE_SIZE) | ICACHE_SET_BIT), %g1
	stxa %g0, [%g1] ASI_ICACHE_TAG

	0:
		membar #Sync
		subcc %g1, ICACHE_LINE_SIZE, %g1
		bnz,pt %xcc, 0b

	stxa %g0, [%g1] ASI_ICACHE_TAG
	membar #Sync
	retl

	! SF Erratum #51

	nop
#else
	// TODO: sun4v
	retl
	nop
#endif	/* PROCESSOR_us || PROCESSOR_us3 */

FUNCTION_BEGIN(ofw)
	save %sp, -(STACK_WINDOW_SAVE_AREA_SIZE + STACK_ARG_SAVE_AREA_SIZE), %sp
	set ofw_cif, %l0
	ldx [%l0], %l0

	rdpr %pstate, %l1
	and %l1, ~PSTATE_AM_BIT, %l2
	wrpr %l2, 0, %pstate

	jmpl %l0, %o7
	mov %i0, %o0

	wrpr %l1, 0, %pstate

	ret
	restore %o0, 0, %o0
FUNCTION_END(ofw)
