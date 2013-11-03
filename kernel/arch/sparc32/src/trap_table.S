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

.text

.global trap_table
.global reset_trap
.global preemptible_trap
.global interrupt_trap
.global syscall_trap
.global window_overflow_trap
.global window_underflow_trap
.global write_to_invalid

.macro get_wim_number reg
	clr \reg
	mov %wim, %g5
1:	andcc %g5, 1, %g0
	bne 2f
	nop
	srl %g5, 1, %g5
	inc \reg
	b 1b
2:	nop

.endm

.macro get_cwp reg
	mov %psr, \reg
	and \reg, 0x7, \reg
.endm

.macro switch_to_invalid saved_wim, saved_psr
	get_wim_number %l3
	mov %wim, \saved_wim	! save WIM
	mov %g0, %wim		! clear WIM
	mov %psr, \saved_psr	! read PSR
	and \saved_psr, 0xfffffff0, %l4
	or %l4, %l3, %l3	! set CWP
	mov %l3, %psr		! write PSR
	nop
	nop
	nop
	nop			! wait for PSR to be effective
.endm	

.macro switch_back wim, psr
	mov \wim, %wim		! saved WIM
	mov \psr, %psr		! saved PSR
	nop
	nop
	nop
	nop
.endm

/* Save next window to kernel stack or UWB */
.macro inline_save_kernel
	set 0x80000100, %g2
	set 'k', %g1
	sta %g1, [%g2] 0x1c
	save
	std %l0, [%sp +  0]
	std %l2, [%sp +  8]
	std %l4, [%sp + 16]
	std %l6, [%sp + 24]
	std %i0, [%sp + 32]
	std %i2, [%sp + 40]
	std %i4, [%sp + 48]
	std %i6, [%sp + 56]
	restore
.endm

.macro inline_save_uspace uwb
	set 0x80000100, %g2
	set 'u', %g1
	sta %g1, [%g2] 0x1c
	save
	ld [\uwb], %g1
	std %l0, [%g1 +  0]
	std %l2, [%g1 +  8]
	std %l4, [%g1 + 16]
	std %l6, [%g1 + 24]
	std %i0, [%g1 + 32]
	std %i2, [%g1 + 40]
	std %i4, [%g1 + 48]
	std %i6, [%g1 + 56]
	add \uwb, 64, \uwb
	restore
.endm

/* Restore window from kernel stack or UWB */
.macro inline_restore_kernel
	save
        ldd [%sp +  0], %l0
        ldd [%sp +  8], %l2
        ldd [%sp + 16], %l4
        ldd [%sp + 24], %l6
        ldd [%sp + 32], %i0
        ldd [%sp + 40], %i2
        ldd [%sp + 48], %i4
        ldd [%sp + 56], %i6
	restore
.endm

.macro inline_restore_uspace uwb
	save
	ld [\uwb], %g1
	std %l0, [%g1 +  0]
	std %l2, [%g1 +  8]
	std %l4, [%g1 + 16]
	std %l6, [%g1 + 24]
	std %i0, [%g1 + 32]
	std %i2, [%g1 + 40]
	std %i4, [%g1 + 48]
	std %i6, [%g1 + 56]
	sub \uwb, 64, \uwb
	restore
.endm

.macro if_from_kernel label
	mov %psr, %g2
	and %g2, (1 << 6), %g2
	cmp %g2, 0
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

reset_trap:
	set 0x80000100, %l0
	set 'r', %l1
	sta %l1, [%l0] 0x1c
	rett

window_overflow_trap:
	/* rotate WIM on bit right, we have 8 windows */
	mov %wim, %l3
	sll %l3, 7, %l4
	srl %l3, 1, %l3
	or  %l3, %l4, %l3
	and %l3, 0xff, %l3

	/* disable WIM traps */
	mov %g0,%wim
	nop; nop; nop

	/* Save invalid window data */
	mov %l5, %g5		! kernel stack pointer
	mov %l6, %g6		! kernel wbuf
	mov %l7, %g7

	/* Check whether previous mode was usermode */
	and %l0, (1 << 6), %l0
	cmp %l0, 0
	beq 1f
	nop

	/* kernel: */
	/* we should check whether window needs to be saved
	 * to kernel stack or uwb
	 */
	cmp %g7, 0
	bne 2f
	nop

	/* dump registers to stack */
	save
	st %l0, [%sp + 0] ! XXX
	std %l0, [%sp +  0]
	std %l2, [%sp +  8]
	std %l4, [%sp + 16]
	std %l6, [%sp + 24]
	std %i0, [%sp + 32]
	std %i2, [%sp + 40]
	std %i4, [%sp + 48]
	std %i6, [%sp + 56]
	b 3f
	nop

1:	/* uspace: */
	/* set uspace window mark */
	mov %psr, %g7
	and %g7, 0x7, %g7
	inc %g7

2:	save
	std %l0, [%g6 +  0]
	std %l2, [%g6 +  8]
	std %l4, [%g6 + 16]
	std %l6, [%g6 + 24]
	std %i0, [%g6 + 32]
	std %i2, [%g6 + 40]
	std %i4, [%g6 + 48]
	std %i6, [%g6 + 56]
	add %g4, 64, %g4

	/* check whether it's the last user window to be saved */
	mov %psr, %l4
	and %l4, 0x7, %l4
	inc %l4
	cmp %g7, %l4
	bne 3f
	nop

	/* clear uspace window mark */
	clr %g7

3:	/* back to where we should be */
	mov %g5, %l5
	mov %g6, %l6
	mov %g7, %l7
	restore

	/* set new value of window */
	mov %l3,%wim
	nop; nop; nop

	/* go home */
	jmp %l1
	rett %l2

window_underflow_trap:
        /* rotate WIM on bit LEFT, we have 8 windows */ 
        mov %wim,%l3
        srl %l3,7,%l4
        sll %l3,1,%l3
        or  %l3,%l4,%l3
        and %l3, 0xff,%l3

        /* disable WIM traps */
        mov %g0,%wim
        nop; nop; nop

	/* Check whether previous mode was usermode */
	mov %psr, %l0
	and %l0, (1 << 6), %l0
	cmp %l0, 0
	beq 1f
	nop

	restore
	restore
	mov %l5, %g5		! kernel stack pointer
	mov %l6, %g6		! kernel wbuf
	mov %l7, %g7

        ldd [%sp +  0], %l0
        ldd [%sp +  8], %l2
        ldd [%sp + 16], %l4
        ldd [%sp + 24], %l6
        ldd [%sp + 32], %i0
        ldd [%sp + 40], %i2
        ldd [%sp + 48], %i4
        ldd [%sp + 56], %i6
	b 2f
	nop

1:	restore
	restore
	mov %l5, %g5		! kernel stack pointer
	mov %l6, %g6		! kernel wbuf
	mov %l7, %g7
	sub %g6, 64, %g6
        ldd [%g6 +  0], %l0
        ldd [%g6 +  8], %l2
        ldd [%g6 + 16], %l4
        ldd [%g6 + 24], %l6
        ldd [%g6 + 32], %i0
        ldd [%g6 + 40], %i2
        ldd [%g6 + 48], %i4
        ldd [%g6 + 56], %i6

2:	/* Restore invalid window data */
	restore
	mov %g5, %l5
	mov %g6, %l6
	mov %g7, %l7
	save
	save
	save

	/* Set new value of window */
        mov %l3,%wim
        nop; nop; nop

        /* go home */
        jmp %l1
        rett %l2

preemptible_trap:
	/* Check whether we landed in invalid window */
	get_wim_number %g6
	get_cwp %g7
	cmp %g6, %g7
	bne 4f
	nop

	/* We are in invalid window. Check whether previous mode was usermode */
	if_from_kernel 3f

	/* Trap originated from uspace */
	/* Kernel stack pointer is at %l5, uwb is at %l6 */
	inline_save_uspace %l6

	/* set uspace window mark */
	mov %psr, %l7
	and %l7, 0x7, %l7
	inc %l7

3:	/* Trap originated from kernel */
	inline_save_kernel 

4:	/* Check whether previous mode was usermode */
	if_from_kernel 5f

	/* Load kernel stack pointer from invalid window */
	switch_to_invalid %g5, %g6
	/* set uspace window mark */
	mov %g6, %l7
	and %l7, 0x7, %l7
	inc %l7
	/* Save stack pointer */
	mov %l5, %g7
	switch_back %g5, %g6
	mov %g7, %sp
//	mov %sp, %fp

5:	/* Set up stack frame */
	sub %sp, 112, %sp

	/* Save trap data on stack */
	mov %psr, %l0
	st %l1, [%sp + 92]
	st %l2, [%sp + 96]
	st %l0, [%sp + 100]

	/* Enable traps */
	mov %psr, %l0
	or %l0, (1 << 5), %l0
	mov %l0, %psr
	nop
	nop
	nop
	nop

	/* Jump to actual subroutine */
	call %o2
	sub %fp, 12, %o1

	/* Return from handler */
	ld [%sp + 92], %l1
	ld [%sp + 96], %l2
	ld [%sp + 100], %l0
	mov %l0, %psr
	nop
	nop
	nop
	nop
	nop

	/* If trap originated from uspace, clear uspace window mark */
	if_from_kernel 6f
	switch_to_invalid %g5, %g6
	clr %l7
	switch_back %g5, %g6

6:	jmp %l1
	rett %l2

interrupt_trap:
	mov %psr, %l0

	/* Check whether previous mode was usermode */
	and %l0, (1 << 6), %l0
	cmp %l0, 0
	bne 1f
	nop

	/* Set up stack */
	set kernel_sp, %l4
	ld [%l4], %sp
	mov %sp, %fp
1:	sub %sp, 112, %sp

	/* Save trap data on stack */
	mov %psr, %l0
	st %l1, [%fp - 4]
	st %l2, [%fp - 8]
	st %l0, [%fp - 12]

	/* Enable traps */
	mov %psr, %l0
	or %l0, (1 << 5), %l0
	mov %l0, %psr
	nop
	nop
	nop
	nop

	/* Jump to actual subroutine */
	mov %g2, %o0
	call exc_dispatch
	sub %fp, 12, %o1

	/* Return from handler */
	ld [%fp - 4], %l1
	ld [%fp - 8], %l2
	ld [%fp - 12], %l0
	mov %l0, %psr
	nop
	nop
	nop
	nop
	nop
	jmp %l1
	rett %l2

syscall_trap:
	/* Check whether we landed in invalid window */
	get_wim_number %g6
	get_cwp %g7
	cmp %g6, %g7
	bne 4f
	nop

	/* We are in invalid window. Check whether previous mode was usermode */
	if_from_kernel 3f

	/* Kernel stack pointer is at %l5, uwb is at %l6 */
	inline_save_uspace %l6
3:	inline_save_kernel 

4:	/* Check whether previous mode was usermode */
	if_from_kernel 5f

	/* Load kernel stack pointer from invalid window */
	switch_to_invalid %g5, %g6
	restore
	mov %l5, %g7		! stack pointer
	switch_back %g5, %g6
	mov %g7, %sp
	mov %sp, %fp
5:	sub %sp, 112, %sp

	/* Save trap data on stack */
	mov %psr, %l0
	st %l1, [%fp - 4]
	st %l2, [%fp - 8]
	st %l0, [%fp - 12]

	/* Enable traps */
	mov %psr, %l0
	or %l0, (1 << 5), %l0
	mov %l0, %psr
	nop
	nop
	nop
	nop

	/* Jump to actual subroutine */
	sub %o0, 0x80, %o0
	st %o0, [ %sp + 92 ]
	mov %i0, %o0
	mov %i1, %o1
	mov %i2, %o2
	mov %i3, %o3
	mov %i4, %o4
	call syscall
	mov %i5, %o5

	/* Return from handler */
	ld [%fp - 4], %l1
	ld [%fp - 8], %l2
	ld [%fp - 12], %l0
	mov %l0, %psr
	nop
	nop
	nop
	nop
	nop
	jmp %l2
	rett %l2 + 4

#define	STRAP(_vector, _handler) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE; \
	mov %psr, %l0 ; \
	sethi %hi(_handler), %l4 ; \
	jmp %lo(_handler) + %l4 ; \
	nop

#define	TRAP(_vector, _handler) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE; \
	set _vector, %o0 ; \
	sethi %hi(_handler), %o2 ; \
	b preemptible_trap ; \
	or %o2, %lo(_handler), %o2 ;

#define	SYSCALL(_vector) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE; \
	set _vector, %o0 ; \
	b syscall_trap ; \
	nop ;

#define	INTERRUPT(_vector, _priority) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE; \
	mov %psr, %l0 ; \
	mov _priority, %g2 ; \
	b interrupt_trap ; \
	nop ;

#define	BADTRAP(_vector) \
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
