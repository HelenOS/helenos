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
	mov   %psr, %l0 ; \
	sethi %hi(_handler), %l4 ; \
	jmp   %l4+%lo(_handler); \
	mov   _vector, %l3 ;

#define	BADTRAP(_vector) \
	.org trap_table + _vector * TRAP_ENTRY_SIZE ; \
	ta 0 ;

.align TRAP_TABLE_SIZE
trap_table:
	TRAP(0, reset_trap)
	BADTRAP(1)
	TRAP(2, illegal_instruction)
	TRAP(3, privileged_instruction)
	TRAP(4, fp_disabled)
	TRAP(5, window_overflow_trap)
	TRAP(6, window_underflow_trap)
	BADTRAP(7)
	BADTRAP(8)
	BADTRAP(9)
	BADTRAP(10)
	BADTRAP(11)
	BADTRAP(12)
	BADTRAP(13)
	BADTRAP(14)
	BADTRAP(15)
	BADTRAP(16)
        BADTRAP(17)
        BADTRAP(18)
        BADTRAP(19)
        BADTRAP(20)
        BADTRAP(21)
        BADTRAP(22)
        BADTRAP(23)
        BADTRAP(24)
        BADTRAP(25)
        BADTRAP(26)
        BADTRAP(27)
        BADTRAP(28)
        BADTRAP(29)
        BADTRAP(30)
        BADTRAP(31)
        BADTRAP(32)
        BADTRAP(33)
        BADTRAP(34)
        BADTRAP(35)
        BADTRAP(36)
        BADTRAP(37)
        BADTRAP(38)
        BADTRAP(39)
        BADTRAP(40)
        BADTRAP(41)
        BADTRAP(42)
        BADTRAP(43)
        BADTRAP(44)
        BADTRAP(45)
        BADTRAP(46)
        BADTRAP(47)
        BADTRAP(48)
        BADTRAP(49)
        BADTRAP(50)
        BADTRAP(51)
        BADTRAP(52)
        BADTRAP(53)
        BADTRAP(54)
        BADTRAP(55)
        BADTRAP(56)
        BADTRAP(57)
        BADTRAP(58)
        BADTRAP(59)
        BADTRAP(60)
        BADTRAP(61)
        BADTRAP(62)
        BADTRAP(63)
        BADTRAP(64)
        BADTRAP(65)
        BADTRAP(66)
        BADTRAP(67)
        BADTRAP(68)
        BADTRAP(69)
        BADTRAP(70)
        BADTRAP(71)
        BADTRAP(72)
        BADTRAP(73)
        BADTRAP(74)
        BADTRAP(75)
        BADTRAP(76)
        BADTRAP(77)
        BADTRAP(78)
        BADTRAP(79)
        BADTRAP(80)
        BADTRAP(81)
        BADTRAP(82)
        BADTRAP(83)
        BADTRAP(84)
        BADTRAP(85)
        BADTRAP(86)
        BADTRAP(87)
        BADTRAP(88)
        BADTRAP(89)
        BADTRAP(90)
        BADTRAP(91)
        BADTRAP(92)
        BADTRAP(93)
        BADTRAP(94)
        BADTRAP(95)
        BADTRAP(96)
        BADTRAP(97)
        BADTRAP(98)
        BADTRAP(99)
        BADTRAP(100)
        BADTRAP(101)
        BADTRAP(102)
        BADTRAP(103)

