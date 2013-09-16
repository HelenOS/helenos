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
.global window_overflow_trap
.global window_underflow_trap

reset_trap:
	set 0x80000100, %l0
	set 'r', %l1
	sta %l1, [%l0] 0x1c
	rett

window_overflow_trap:
        /* rotate WIM on bit right, we have 8 windows */
        mov %wim,%l3
        sll %l3,7,%l4
        srl %l3,1,%l3
        or  %l3,%l4,%l3
        and %l3,0xff,%l3

        /* disable WIM traps */
        mov %g0,%wim
        nop; nop; nop

        /* point to correct window */
        save

        /* dump registers to stack */
        std %l0, [%sp +  0]
        std %l2, [%sp +  8]
        std %l4, [%sp + 16]
        std %l6, [%sp + 24]
        std %i0, [%sp + 32]
        std %i2, [%sp + 40]
        std %i4, [%sp + 48]
        std %i6, [%sp + 56]

        /* back to where we should be */
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

        /* point to correct window */
        restore
        restore

        /* dump registers to stack */
        ldd [%sp +  0], %l0
        ldd [%sp +  8], %l2
        ldd [%sp + 16], %l4
        ldd [%sp + 24], %l6
        ldd [%sp + 32], %i0
        ldd [%sp + 40], %i2
        ldd [%sp + 48], %i4
        ldd [%sp + 56], %i6

        /* back to where we should be */
        save
        save

        /* set new value of window */
        mov %l3,%wim
        nop; nop; nop

        /* go home */
        jmp %l1
        rett %l2

#define	TRAP(_vector, _handler) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE; \
	mov %psr, %l0 ; \
	sethi %hi(_handler), %l4 ; \
	jmp %l4 + %lo(_handler); \
	mov _vector, %l3 ;

#define	INTERRUPT(_vector, _priority) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE; \
	mov %psr, %l0 ; \
	mov _priority, %g2 ; \
	call exc_dispatch ; \
	nop ;

#define	BADTRAP(_vector) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE ; \
	ta 0 ;

.align TRAP_TABLE_SIZE
trap_table:
	TRAP(0x0, reset_trap)
	TRAP(0x1, instruction_access_exception)
	TRAP(0x2, illegal_instruction)
	TRAP(0x3, privileged_instruction)
	TRAP(0x4, fp_disabled)
	TRAP(0x5, window_overflow_trap)
	TRAP(0x6, window_underflow_trap)
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

