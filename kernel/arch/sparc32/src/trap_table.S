#
# Copyright (c) 2013 Jakub Klama
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

#include <arch/trap.h>
#include <arch/regwin.h>

.text

.global trap_table
.global reset_trap
.global preemptible_trap
.global interrupt_trap
.global syscall_trap
.global window_overflow_trap
.global window_underflow_trap
.global write_to_invalid
.global read_from_invalid
.global flush_windows

.macro get_wim_number reg
	clr \reg
	mov %wim, %g5
	
	1:
		andcc %g5, 1, %g0
		bne 2f
		nop
		srl %g5, 1, %g5
		inc \reg
		b 1b
	
	2:
		nop
.endm

.macro get_cwp reg
	mov %psr, \reg
	and \reg, 0x7, \reg
.endm

.macro switch_to_invalid saved_wim, saved_psr
	get_wim_number %l3
	mov %wim, \saved_wim    ! save WIM
	mov %g0, %wim           ! clear WIM
	mov %psr, \saved_psr    ! read PSR
	and \saved_psr, 0xfffffff0, %l4
	or %l4, %l3, %l3        ! set CWP
	mov %l3, %psr           ! write PSR
	nop
	nop
	nop
	nop                     ! wait for PSR to be effective
.endm

.macro switch_back wim, psr
	mov \wim, %wim          ! saved WIM
	mov \psr, %psr          ! saved PSR
	nop
	nop
	nop
	nop
.endm

/* Save next window to kernel stack or UWB */
.macro inline_save_kernel
	mov %wim, %l3
	sll %l3, 7, %l4
	srl %l3, 1, %l3
	or %l3, %l4, %l3
	and %l3, 0xff, %l3
	mov %g0, %wim
	
	mov %l5, %g5
	mov %l6, %g6
	mov %l7, %g7
	
	save
	std %l0, [%sp + 0]
	std %l2, [%sp + 8]
	std %l4, [%sp + 16]
	std %l6, [%sp + 24]
	std %i0, [%sp + 32]
	std %i2, [%sp + 40]
	std %i4, [%sp + 48]
	std %i6, [%sp + 56]
	mov %g5, %l5
	mov %g6, %l6
	mov %g7, %l7
	restore
	mov %l3, %wim
.endm

.macro inline_save_uspace uwb
	mov %wim, %l3
	sll %l3, 7, %l4
	srl %l3, 1, %l3
	or %l3, %l4, %l3
	and %l3, 0xff, %l3
	mov %g0, %wim
	mov \uwb, %g3
	
	mov %l5, %g5
	mov %l6, %g6
	mov %l7, %g7
	
	save
	std %l0, [%g3 + 0]
	std %l2, [%g3 + 8]
	std %l4, [%g3 + 16]
	std %l6, [%g3 + 24]
	std %i0, [%g3 + 32]
	std %i2, [%g3 + 40]
	std %i4, [%g3 + 48]
	std %i6, [%g3 + 56]
	mov %g5, %l5
	mov %g6, %l6
	mov %g7, %l7
	add \uwb, 64, \uwb
	restore
	mov %l3, %wim
.endm

/* Restore window from kernel stack or UWB */
.macro inline_restore_kernel
	mov %wim, %l3
	srl %l3, 7, %l4
	sll %l3, 1, %l3
	or %l3, %l4, %l3
	and %l3, 0xff, %l3
	
	mov %g0, %wim
	restore
	mov %l5, %g5
	mov %l6, %g6
	mov %l7, %g7
	restore
	mov %g5, %l5
	mov %g6, %l6
	mov %g7, %l7
	save
	
	ldd [%sp + 0], %l0
	ldd [%sp + 8], %l2
	ldd [%sp + 16], %l4
	ldd [%sp + 24], %l6
	ldd [%sp + 32], %i0
	ldd [%sp + 40], %i2
	ldd [%sp + 48], %i4
	ldd [%sp + 56], %i6
	save
	
	mov %l3, %wim
.endm

.macro inline_restore_uspace uwb
	mov %wim, %l3
	srl %l3, 7, %l4
	sll %l3, 1, %l3
	or %l3, %l4, %l3
	and %l3, 0xff, %l3
	
	mov %g0, %wim
	restore
	mov %l5, %g5
	mov %l6, %g6
	mov %l7, %g7
	restore
	mov %g5, %l5
	mov %g6, %l6
	mov %g7, %l7
	save
	
	sub \uwb, 64, \uwb
	ldd [\uwb + 0], %l0
	ldd [\uwb + 8], %l2
	ldd [\uwb + 16], %l4
	ldd [\uwb + 24], %l6
	ldd [\uwb + 32], %i0
	ldd [\uwb + 40], %i2
	ldd [\uwb + 48], %i4
	ldd [\uwb + 56], %i6
	save
	
	mov %l3, %wim
.endm

.macro if_from_kernel label
	mov %psr, %l3
	and %l3, (1 << 6), %l3
	cmp %l3, 0
	bne \label
	nop
.endm

write_to_invalid:
	! Write value 1
	mov %o0, %g7
	switch_to_invalid %g3, %g4
	mov %g7, %l5
	switch_back %g3, %g4
	! Write value 2
	mov %o1, %g7
	switch_to_invalid %g3, %g4
	mov %g7, %l6
	switch_back %g3, %g4
	! Write value 3
	mov %o2, %g7
	switch_to_invalid %g3, %g4
	mov %g7, %l7
	switch_back %g3, %g4
	retl
	nop

read_from_invalid:
	! Read value 1
	mov %o0, %g7
	switch_to_invalid %g3, %g4
	st %l5, [%g7]
	switch_back %g3, %g4
	! Write value 2
	mov %o1, %g7
	switch_to_invalid %g3, %g4
	st %l6, [%g7]
	switch_back %g3, %g4
	! Write value 3
	mov %o2, %g7
	switch_to_invalid %g3, %g4
	st %l7, [%g7]
	switch_back %g3, %g4
	retl
	nop

reset_trap:
	set 0x80000100, %l0
	set 'r', %l1
	sta %l1, [%l0] 0x1c
	rett

window_overflow_trap:
	mov %g7, %l0
	
	/* Check whether previous mode was usermode */
	mov %psr, %l4
	and %l4, (1 << 6), %l4
	cmp %l4, 0
	bne 1f
	nop
	
	/* User space: */
	
	/*
	 * Time to check whether desired stack page is mapped
	 * on the MMU. If so, process with saving window directly.
	 * If not, go to preemptible trap handler.
	 */
	mov %wim, %g5
	mov %g0, %wim
	save
	mov %sp, %g4
	restore
	mov %g5, %wim
	
	/* Check beginning of %sp */
	and %g4, 0xfffff000, %l4
	lda [%l4] 0x18, %l4
	cmp %l4, 0
	bne 1f
	nop
	
	/* Prepare args for preemptible handler */
	mov %g4, %o0
	set preemptible_save_uspace, %o2
	b preemptible_trap
	nop
	
	/* Check end of %sp */
	add %g4, 56, %g4
	and %l4, 0xfffff000, %l4
	lda [%l4] 0x18, %l4
	cmp %l4, 0
	bne 1f
	nop
	
	/* Prepare args for preemptible handler */
	mov %g4, %o0
	set preemptible_save_uspace, %o2
	b preemptible_trap
	nop
	
	/* Kernel: */
	
	1:
		/* Rotate WIM on bit right, we have 8 windows */
		mov %wim, %l3
		sll %l3, 7, %l4
		srl %l3, 1, %l3
		or %l3, %l4, %l3
		and %l3, 0xff, %l3
		
		/* Disable WIM traps */
		mov %g0,%wim
		nop; nop; nop
		
		/* Save invalid window data */
		mov %l5, %g5        ! kernel stack pointer
		mov %l6, %g6        ! kernel wbuf
		mov %l7, %g7
		
		/*
		 * We should check whether window needs to be saved
		 * to kernel stack or uwb.
		 */
		cmp %g7, 0
		bne 2f
		nop
	
	/* Dump registers to stack */
	save
	std %l0, [%sp + 0]
	std %l2, [%sp + 8]
	std %l4, [%sp + 16]
	std %l6, [%sp + 24]
	std %i0, [%sp + 32]
	std %i2, [%sp + 40]
	std %i4, [%sp + 48]
	std %i6, [%sp + 56]
	b 3f
	nop
	
	/* Dump registers to uwb */
	2:
		save
		std %l0, [%g6 + 0]
		std %l2, [%g6 + 8]
		std %l4, [%g6 + 16]
		std %l6, [%g6 + 24]
		std %i0, [%g6 + 32]
		std %i2, [%g6 + 40]
		std %i4, [%g6 + 48]
		std %i6, [%g6 + 56]
		add %g6, 64, %g6
		
		/* Check whether it's the last user window to be saved */
		and %g7, 0x7, %l5
		mov %psr, %l4
		and %l4, 0x7, %l4
		cmp %l5, %l4
		bne 3f
		nop
	
	/* Clear uspace window mark */
	clr %g7
	
	3:
		/* Back to where we should be */
		mov %g5, %l5
		mov %g6, %l6
		mov %g7, %l7
		restore
		
		/* Set new value of window */
		mov %l3,%wim
		nop; nop; nop
		
		/* Go home */
		mov %l0, %g7
		jmp %l1
		rett %l2

window_underflow_trap:
	mov %g7, %l0
	
	/* Check whether previous mode was usermode */
	mov %psr, %l4
	and %l4, (1 << 6), %l4
	cmp %l4, 0
	bne 1f
	nop
	
	/* User space: */
	
	/*
	 * Time to check whether desired stack page is mapped
	 * on the MMU. If so, process with saving window directly.
	 * If not, go to preemptible trap handler.
	 */
	mov %wim, %g5
	mov %g0, %wim
	restore
	restore
	mov %sp, %g4
	save
	save
	mov %g5, %wim
	and %sp, 0xfffff000, %l4
	lda [%l4] 0x18, %l4
	cmp %l4, 0
	bne 1f
	nop
	
	/* Prepare args for preemptible handler */
	mov %g4, %o0
	set preemptible_restore_uspace, %o2
	b preemptible_trap
	nop
	
	1:
		/* Rotate WIM on bit LEFT, we have 8 windows */
		mov %wim, %l3
		srl %l3, 7, %l4
		sll %l3, 1, %l3
		or %l3, %l4, %l3
		and %l3, 0xff, %l3
		
		/* Disable WIM traps */
		mov %g0, %wim
		nop; nop; nop
	
	/* Kernel: */
	restore
	restore
	mov %l5, %g5            ! kernel stack pointer
	mov %l6, %g6            ! kernel wbuf
	mov %l7, %g7
	
	ldd [%sp + 0], %l0
	ldd [%sp + 8], %l2
	ldd [%sp + 16], %l4
	ldd [%sp + 24], %l6
	ldd [%sp + 32], %i0
	ldd [%sp + 40], %i2
	ldd [%sp + 48], %i4
	ldd [%sp + 56], %i6
	b 2f
	nop
	
	2:
		/* Restore invalid window data */
		restore
		mov %g5, %l5
		mov %g6, %l6
		mov %g7, %l7
		save
		save
		save
		
		/* Set new value of window */
		mov %l3, %wim
		nop; nop; nop
		
		/* Go home */
		mov %l0, %g7
		jmp %l1
		rett %l2

flush_windows:
	mov 7, %g1
	1:
		subcc %g1, 1, %g1
		bg 1b
		save %sp, -64, %sp
	
	mov 7, %g1
	1:
		subcc %g1, 1, %g1
		bg 1b
		restore
	
	retl
	nop

preemptible_trap:
	/* Save %g7 */
	mov %g7, %l0
	
	/* Check whether we landed in invalid window */
	get_wim_number %g6
	get_cwp %g7
	cmp %g6, %g7
	bne 4f
	nop
	
	/* We are in invalid window. Check whether previous mode was usermode. */
	if_from_kernel 3f
	
	/* Trap originated from user space */
	/* Kernel stack pointer is at %l5, uwb is at %l6 */
	inline_save_uspace %l6
	
	/* Set uspace window mark */
	mov %psr, %l7
	inc %l7
	and %l7, 0x7, %l7
	or %l7, 0x10, %l7
	b 4f
	nop
	
	3:
		/* Trap originated from kernel */
		inline_save_kernel
	
	4:
		/* Check whether previous mode was user mode */
		if_from_kernel 5f
	
	/* Load kernel stack pointer from invalid window */
	switch_to_invalid %g5, %g6
	
	/* set uspace window mark */
	mov %g6, %l7
	inc %l7
	and %l7, 0x7, %l7
	or %l7, 0x10, %l7
	
	/* Save stack pointer */
	mov %l5, %g7
	switch_back %g5, %g6
	mov %g7, %sp
	##	mov %sp, %fp
	
	5:
		/* Set up stack frame */
		sub %sp, 128, %sp
		
		/* Save trap data on stack */
		mov %psr, %l5
		st %l1, [%sp + 92]
		st %l2, [%sp + 96]
		st %l5, [%sp + 100]
		st %g1, [%sp + 104]
		st %g2, [%sp + 108]
		st %g3, [%sp + 112]
		st %g4, [%sp + 116]
		st %l0, [%sp + 120]
		
		/* Enable traps */
		mov %psr, %l0
		or %l0, (1 << 5), %l0
		or %l0, 0xf00, %l0
		mov %l0, %psr
		nop
		nop
		nop
		nop
		
		/* Flush windows to stack */
		call flush_windows
		nop
		
		/* Jump to actual subroutine */
		call %o2
		add %sp, 128, %o1
		
		/* Return from handler (leave PIL disabled) */
		ld [%sp + 92], %l1
		ld [%sp + 96], %l2
		ld [%sp + 100], %l0
		or %l0, 0xf00, %l0
		mov %l0, %psr
		nop
		nop
		nop
		nop
		nop
		
		/*
		 * If trap originated from user space, clear user space window mark
		 * and save uwb address for future use.
		 */
		if_from_kernel 9f
		switch_to_invalid %g5, %g6
		clr %l7
		mov %l5, %g2
		mov %l6, %g7
		switch_back %g5, %g6
		mov %g7, %g1
		
		/* If trap originated from user space, restore all windows from UWB */
		/* UWB pointer is at %g1 */
	0:
		mov %g0, %wim
		clr %g5
		andcc %g1, UWB_ALIGNMENT - 1, %g0
		bz 0f
		nop
	
	restore
	sub %g1, 64, %g1
	ldd [%g1 + 0], %l0
	ldd [%g1 + 8], %l2
	ldd [%g1 + 16], %l4
	ldd [%g1 + 24], %l6
	ldd [%g1 + 32], %i0
	ldd [%g1 + 40], %i2
	ldd [%g1 + 48], %i4
	ldd [%g1 + 56], %i6
	inc %g5
	and %g5, 0x7, %g5
	ba 0b
	nop
	
	/*
	 * We've restored all uspace windows. Now time to
	 * fix CWP and WIM.
	 */
	0:
		restore
		get_cwp %g7
		clr %g5
		inc %g5
		sll %g5, %g7, %g5
		
		/* Write values to invalid window and switch back */
		mov %g2, %l5
		mov %g1, %l6
		clr %l7
		switch_back %g5, %g6
		
		mov %sp, %l3
		sub %g2, 128, %sp
		ld [%sp + 104], %g1
		ld [%sp + 108], %g2
		ld [%sp + 112], %g3
		ld [%sp + 116], %g4
		ld [%sp + 120], %g7
		
		mov %l3, %sp
		b 10f
		nop
	
	9:
		inline_restore_kernel
		
		ld [%sp + 104], %g1
		ld [%sp + 108], %g2
		ld [%sp + 112], %g3
		ld [%sp + 116], %g4
		ld [%sp + 120], %g7
	
		/* Restore old sp */
		add %sp, 128, %sp
	
	10:
		jmp %l1
		rett %l2

interrupt_trap:
	/* Save %g7 */
	mov %g7, %l0
	
	/* Check whether we landed in invalid window */
	get_wim_number %g6
	get_cwp %g7
	cmp %g6, %g7
	bne 4f
	nop
	
	/* We are in invalid window. Check whether previous mode was user mode */
	if_from_kernel 3f
	
	/* Trap originated from user space */
	/* Kernel stack pointer is at %l5, uwb is at %l6 */
	inline_save_uspace %l6
	
	/* Set uspace window mark */
	mov %psr, %l7
	inc %l7
	and %l7, 0x7, %l7
	or %l7, 0x10, %l7
	b 4f
	nop
	
	3:
		/* Trap originated from kernel */
		inline_save_kernel
	
	4:
		/* Check whether previous mode was user mode */
		if_from_kernel 5f
		
		/* Load kernel stack pointer from invalid window */
		switch_to_invalid %g5, %g6
		
		/* Set user space window mark */
		mov %g6, %l7
		inc %l7
		and %l7, 0x7, %l7
		or %l7, 0x10, %l7
		
		/* Save stack pointer */
		mov %l5, %g7
		switch_back %g5, %g6
		mov %g7, %sp
	
	5:
		/* Set up stack frame */
		sub %sp, 128, %sp
		
		/* Save trap data on stack */
		mov %psr, %l5
		st %l1, [%sp + 92]
		st %l2, [%sp + 96]
		st %l5, [%sp + 100]
		st %g1, [%sp + 104]
		st %g2, [%sp + 108]
		st %g3, [%sp + 112]
		st %g4, [%sp + 116]
		st %l0, [%sp + 120]
		
		/* Enable traps (without PIL) */
		mov %psr, %l0
		or %l0, (1 << 5), %l0
		or %l0, 0xf00, %l0
		mov %l0, %psr
		nop
		nop
		nop
		nop
		
		/* Flush windows to stack */
		call flush_windows
		nop
		
		/* Jump to actual subroutine */
		call irq_exception
		add %sp, 128, %o1
		
		/* Return from handler (leave PIL disabled) */
		ld [%sp + 92], %l1
		ld [%sp + 96], %l2
		ld [%sp + 100], %l0
		or %l0, 0xf00, %l0
		mov %l0, %psr
		nop
		nop
		nop
		nop
		nop
		
		/*
		 * If trap originated from user space, clear user space window mark
		 * and save uwb address for future use.
		 */
		if_from_kernel 9f
		switch_to_invalid %g5, %g6
		clr %l7
		mov %l5, %g2
		mov %l6, %g7
		switch_back %g5, %g6
		mov %g7, %g1
		
		/* If trap originated from user space, restore all windows from UWB */
		/* UWB pointer is at %g1 */
	0:
		mov %g0, %wim
		clr %g5
		andcc %g1, UWB_ALIGNMENT - 1, %g0
		bz 0f
		nop
		
		restore
		sub %g1, 64, %g1
		ldd [%g1 + 0], %l0
		ldd [%g1 + 8], %l2
		ldd [%g1 + 16], %l4
		ldd [%g1 + 24], %l6
		ldd [%g1 + 32], %i0
		ldd [%g1 + 40], %i2
		ldd [%g1 + 48], %i4
		ldd [%g1 + 56], %i6
		inc %g5
		and %g5, 0x7, %g5
		ba 0b
		nop
	
	/*
	 * We've restored all user space windows. Now time to
	 * fix CWP and WIM.
	 */
	0:
		restore
		get_cwp %g7
		clr %g5
		inc %g5
		sll %g5, %g7, %g5
		
		/* Write values to invalid window and switch back */
		mov %g2, %l5
		mov %g1, %l6
		clr %l7
		switch_back %g5, %g6
		
		mov %sp, %l3
		sub %g2, 128, %sp
		ld [%sp + 104], %g1
		ld [%sp + 108], %g2
		ld [%sp + 112], %g3
		ld [%sp + 116], %g4
		ld [%sp + 120], %g7
		
		mov %l3, %sp
		b 10f
		nop
	
	9:
		inline_restore_kernel
		
		ld [%sp + 104], %g1
		ld [%sp + 108], %g2
		ld [%sp + 112], %g3
		ld [%sp + 116], %g4
		ld [%sp + 120], %g7
		
		/* Restore old sp */
		add %sp, 128, %sp
	
	10:
		jmp %l1
		rett %l2

syscall_trap:
	/* Save %g7 */
	mov %g7, %l0
	
	/* Check whether we landed in invalid window */
	get_wim_number %g6
	get_cwp %g7
	cmp %g6, %g7
	bne 4f
	nop
	
	/* We are in invalid window. Check whether previous mode was user mode */
	if_from_kernel 3f
	
	/* Trap originated from user space */
	/* Kernel stack pointer is at %l5, uwb is at %l6 */
	inline_save_uspace %l6
	
	/* Set user space window mark */
	mov %psr, %l7
	inc %l7
	and %l7, 0x7, %l7
	or %l7, 0x10, %l7
	b 4f
	nop
	
	3:
		/* Trap originated from kernel */
		inline_save_kernel
	
	4:
		/* Check whether previous mode was user mode */
		if_from_kernel 5f
		
		/* Load kernel stack pointer from invalid window */
		switch_to_invalid %g5, %g6
		
		/* Set user space window mark */
		mov %g6, %l7
		inc %l7
		and %l7, 0x7, %l7
		or %l7, 0x10, %l7
		
		/* Save stack pointer */
		mov %l5, %g7
		switch_back %g5, %g6
		mov %g7, %sp
		## mov %sp, %fp
		
	5:
		/* Set up stack frame */
		sub %sp, 128, %sp
		
		/* Save trap data on stack */
		mov %psr, %l5
		st %l1, [%sp + 92]
		st %l2, [%sp + 96]
		st %l5, [%sp + 100]
		st %g1, [%sp + 104]
		st %g2, [%sp + 108]
		st %g3, [%sp + 112]
		st %g4, [%sp + 116]
		st %l0, [%sp + 120]
		
		/* Enable traps (without PIL) */
		mov %psr, %l0
		or %l0, (1 << 5), %l0
		or %l0, 0xf00, %l0
		mov %l0, %psr
		nop
		nop
		nop
		nop
		
		/* Flush windows */
		call flush_windows
		nop
		
		/* Jump to actual subroutine */
		sub %o0, 0x80, %o0
		st %o0, [%sp + 92]
		mov %i0, %o0
		mov %i1, %o1
		mov %i2, %o2
		mov %i3, %o3
		mov %i4, %o4
		call syscall
		mov %i5, %o5
		
		/* Return from handler (with PIL disabled) */
		ld [%sp + 92], %l1
		ld [%sp + 96], %l2
		ld [%sp + 100], %l0
		mov %o0, %i0
		mov %psr, %l1
		and %l1, 0xf, %l1
		or %l1, 0x00000f00, %l1
		and %l0, 0xfffffff0, %l0
		or %l0, %l1, %l0
		mov %l0, %psr
		nop
		nop
		nop
		nop
		nop
		
		/*
		 * If trap originated from user space, clear user space window mark
		 * and save uwb address for future use.
		 */
		if_from_kernel 8f
		switch_to_invalid %g5, %g6
		mov %l5, %g2
		mov %l6, %g1
		mov %l7, %g7
		switch_back %g5, %g6
		
		/* If trap originated from user space, restore all windows from UWB */
		/* UWB pointer is at %g1 */
	0:
		mov %g0, %wim
		clr %g5
		andcc %g1, UWB_ALIGNMENT - 1, %g0
		bz 0f
		nop
		
		restore
		sub %g1, 64, %g1
		ldd [%g1 + 0], %l0
		ldd [%g1 + 8], %l2
		ldd [%g1 + 16], %l4
		ldd [%g1 + 24], %l6
		ldd [%g1 + 32], %i0
		ldd [%g1 + 40], %i2
		ldd [%g1 + 48], %i4
		ldd [%g1 + 56], %i6
		inc %g5
		and %g5, 0x7, %g5
		ba 0b
		nop
		
		/*
		 * We've restored all user space windows. Now time to
		 * fix CWP and WIM.
		 */
	0:
		restore
		get_cwp %g7
		clr %g5
		inc %g5
		sll %g5, %g7, %g5
		
		/* Write values to invalid window and switch back */
		mov %g2, %l5
		mov %g1, %l6
		clr %l7
		switch_back %g5, %g6
	
	8:
		mov %sp, %l1
		sub %g2, 128, %sp
		ld [%sp + 104], %g1
		ld [%sp + 108], %g2
		ld [%sp + 112], %g3
		ld [%sp + 116], %g4
		ld [%sp + 120], %g7
		
		mov %l1, %sp
		nop
		nop
		nop
		
		jmp %l2
		rett %l2 + 4

#define STRAP(_vector, _handler) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE; \
	mov %psr, %l0 ; \
	sethi %hi(_handler), %l4 ; \
	jmp %lo(_handler) + %l4 ; \
	nop

#define TRAP(_vector, _handler) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE; \
	set _vector, %o0 ; \
	sethi %hi(_handler), %o2 ; \
	b preemptible_trap ; \
	or %o2, %lo(_handler), %o2 ;

#define SYSCALL(_vector) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE; \
	set _vector, %o0 ; \
	b syscall_trap ; \
	nop ;

#define INTERRUPT(_vector, _priority) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE; \
	mov _priority, %o0 ; \
	b interrupt_trap ; \
	nop ;
	nop ;

#define BADTRAP(_vector) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE ; \
	ta 0 ;

.align TRAP_TABLE_SIZE
trap_table:
	STRAP(0x0, reset_trap)
	TRAP(0x1, instruction_access_exception)
	TRAP(0x2, illegal_instruction)
	TRAP(0x3, privileged_instruction)
	TRAP(0x4, fp_disabled)
	STRAP(0x5, window_overflow_trap)
	STRAP(0x6, window_underflow_trap)
	TRAP(0x7, mem_address_not_aligned)
	TRAP(0x8, fp_exception)
	TRAP(0x9, data_access_exception)
	TRAP(0xa, tag_overflow)
	BADTRAP(0xb)
	BADTRAP(0xc)
	BADTRAP(0xd)
	BADTRAP(0xe)
	BADTRAP(0xf)
	BADTRAP(0x10)
	INTERRUPT(0x11, 1)
	INTERRUPT(0x12, 2)
	INTERRUPT(0x13, 3)
	INTERRUPT(0x14, 4)
	INTERRUPT(0x15, 5)
	INTERRUPT(0x16, 6)
	INTERRUPT(0x17, 7)
	INTERRUPT(0x18, 8)
	INTERRUPT(0x19, 9)
	INTERRUPT(0x1a, 10)
	INTERRUPT(0x1b, 11)
	INTERRUPT(0x1c, 12)
	INTERRUPT(0x1d, 13)
	INTERRUPT(0x1e, 14)
	INTERRUPT(0x1f, 15)
	TRAP(0x21, instruction_access_error)
	BADTRAP(0x22)
	BADTRAP(0x23)
	BADTRAP(0x24)
	BADTRAP(0x25)
	BADTRAP(0x26)
	BADTRAP(0x27)
	BADTRAP(0x28)
	TRAP(0x29, data_access_error)
	TRAP(0x2a, division_by_zero)
	TRAP(0x2b, data_store_error)
	TRAP(0x2c, data_access_mmu_miss)
	BADTRAP(0x2d)
	BADTRAP(0x2e)
	BADTRAP(0x2f)
	BADTRAP(0x30)
	BADTRAP(0x31)
	BADTRAP(0x32)
	BADTRAP(0x33)
	BADTRAP(0x34)
	BADTRAP(0x35)
	BADTRAP(0x36)
	BADTRAP(0x37)
	BADTRAP(0x38)
	BADTRAP(0x39)
	BADTRAP(0x3a)
	BADTRAP(0x3b)
	BADTRAP(0x3c)
	BADTRAP(0x3d)
	BADTRAP(0x3e)
	BADTRAP(0x3f)
	BADTRAP(0x40)
	BADTRAP(0x41)
	BADTRAP(0x42)
	BADTRAP(0x43)
	BADTRAP(0x44)
	BADTRAP(0x45)
	BADTRAP(0x46)
	BADTRAP(0x47)
	BADTRAP(0x48)
	BADTRAP(0x49)
	BADTRAP(0x4a)
	BADTRAP(0x4b)
	BADTRAP(0x4c)
	BADTRAP(0x4d)
	BADTRAP(0x4e)
	BADTRAP(0x4f)
	BADTRAP(0x50)
	BADTRAP(0x51)
	BADTRAP(0x52)
	BADTRAP(0x53)
	BADTRAP(0x54)
	BADTRAP(0x55)
	BADTRAP(0x56)
	BADTRAP(0x57)
	BADTRAP(0x58)
	BADTRAP(0x59)
	BADTRAP(0x5a)
	BADTRAP(0x5b)
	BADTRAP(0x5c)
	BADTRAP(0x5d)
	BADTRAP(0x5e)
	BADTRAP(0x5f)
	BADTRAP(0x60)
	BADTRAP(0x61)
	BADTRAP(0x62)
	BADTRAP(0x63)
	BADTRAP(0x64)
	BADTRAP(0x65)
	BADTRAP(0x66)
	BADTRAP(0x67)
	BADTRAP(0x68)
	BADTRAP(0x69)
	BADTRAP(0x6a)
	BADTRAP(0x6b)
	BADTRAP(0x6c)
	BADTRAP(0x6d)
	BADTRAP(0x6e)
	BADTRAP(0x6f)
	BADTRAP(0x70)
	BADTRAP(0x71)
	BADTRAP(0x72)
	BADTRAP(0x73)
	BADTRAP(0x74)
	BADTRAP(0x75)
	BADTRAP(0x76)
	BADTRAP(0x77)
	BADTRAP(0x78)
	BADTRAP(0x79)
	BADTRAP(0x7a)
	BADTRAP(0x7b)
	BADTRAP(0x7c)
	BADTRAP(0x7d)
	BADTRAP(0x7e)
	BADTRAP(0x7f)
	SYSCALL(0x80)
	SYSCALL(0x81)
	SYSCALL(0x82)
	SYSCALL(0x83)
	SYSCALL(0x84)
	SYSCALL(0x85)
	SYSCALL(0x86)
	SYSCALL(0x87)
	SYSCALL(0x88)
	SYSCALL(0x89)
	SYSCALL(0x8a)
	SYSCALL(0x8b)
	SYSCALL(0x8c)
	SYSCALL(0x8d)
	SYSCALL(0x8e)
	SYSCALL(0x8f)
	SYSCALL(0x90)
	SYSCALL(0x91)
	SYSCALL(0x92)
	SYSCALL(0x93)
	SYSCALL(0x94)
	SYSCALL(0x95)
	SYSCALL(0x96)
	SYSCALL(0x97)
	SYSCALL(0x98)
	SYSCALL(0x99)
	SYSCALL(0x9a)
	SYSCALL(0x9b)
	SYSCALL(0x9c)
	SYSCALL(0x9d)
	SYSCALL(0x9e)
	SYSCALL(0x9f)
	SYSCALL(0xa0)
	SYSCALL(0xa1)
	SYSCALL(0xa2)
	SYSCALL(0xa3)
	SYSCALL(0xa4)
	SYSCALL(0xa5)
	SYSCALL(0xa6)
	SYSCALL(0xa7)
	SYSCALL(0xa8)
	SYSCALL(0xa9)
	SYSCALL(0xaa)
	SYSCALL(0xab)
	SYSCALL(0xac)
	SYSCALL(0xad)
	SYSCALL(0xae)
	SYSCALL(0xaf)
	SYSCALL(0xb0)
	SYSCALL(0xb1)
	SYSCALL(0xb2)
	SYSCALL(0xb3)
	SYSCALL(0xb4)
	SYSCALL(0xb5)
	SYSCALL(0xb6)
	SYSCALL(0xb7)
	SYSCALL(0xb8)
	SYSCALL(0xb9)
	SYSCALL(0xba)
	SYSCALL(0xbb)
	SYSCALL(0xbc)
	SYSCALL(0xbd)
	SYSCALL(0xbe)
	SYSCALL(0xbf)
	SYSCALL(0xc0)
	SYSCALL(0xc1)
	SYSCALL(0xc2)
	SYSCALL(0xc3)
	SYSCALL(0xc4)
	SYSCALL(0xc5)
	SYSCALL(0xc6)
	SYSCALL(0xc7)
	SYSCALL(0xc8)
	SYSCALL(0xc9)
	SYSCALL(0xca)
	SYSCALL(0xcb)
	SYSCALL(0xcc)
	SYSCALL(0xcd)
	SYSCALL(0xce)
	SYSCALL(0xcf)
