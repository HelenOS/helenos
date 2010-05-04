#
# Copyright (c) 2006 Martin Decky
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
#include <arch/regname.h>

.set noat
.set noreorder
.set nomacro

.global start
.global halt
.global memcpy
.global jump_to_kernel

.section BOOTSTRAP

start:
	/* Setup CPU map (on msim this code
	   is executed in parallel on all CPUs,
	   but it not an issue) */
	la $a0, PA2KA(CPUMAP_OFFSET)
	
	sw $zero, 0($a0)
	sw $zero, 4($a0)
	sw $zero, 8($a0)
	sw $zero, 12($a0)
	
	sw $zero, 16($a0)
	sw $zero, 20($a0)
	sw $zero, 24($a0)
	sw $zero, 28($a0)
	
	sw $zero, 32($a0)
	sw $zero, 36($a0)
	sw $zero, 40($a0)
	sw $zero, 44($a0)
	
	sw $zero, 48($a0)
	sw $zero, 52($a0)
	sw $zero, 56($a0)
	sw $zero, 60($a0)
	
	sw $zero, 64($a0)
	sw $zero, 68($a0)
	sw $zero, 72($a0)
	sw $zero, 76($a0)
	
	sw $zero, 80($a0)
	sw $zero, 84($a0)
	sw $zero, 88($a0)
	sw $zero, 92($a0)
	
	sw $zero, 96($a0)
	sw $zero, 100($a0)
	sw $zero, 104($a0)
	sw $zero, 108($a0)
	
	sw $zero, 112($a0)
	sw $zero, 116($a0)
	sw $zero, 120($a0)
	sw $zero, 124($a0)
	
	lui $a1, 1
	
#ifdef MACHINE_msim
	
	/* Read dorder value */
	la $k0, MSIM_DORDER_ADDRESS
	lw $k1, ($k0)
	
	/* If we are not running on BSP
	   then end in an infinite loop  */
	beq $k1, $zero, bsp
	nop
	
	/* Record CPU presence */
	sll $a2, $k1, 2
	addu $a2, $a2, $a0
	sw $a1, ($a2)
	
	loop:
		j loop
		nop
	
#endif
	
	bsp:
		/* Record CPU presence */
		sw $a1, ($a0)
		
		/* Setup initial stack */
		la $sp, PA2KA(STACK_OFFSET)
		
		j bootstrap
		nop

.text

halt:
	j halt
	nop

memcpy:
	addiu $v0, $a1, 3
	li $v1, -4
	and $v0, $v0, $v1
	beq $a1, $v0, 3f
	move $t0, $a0
	move $t2, $a0
	
	0:
		beq $a2, $zero, 2f
		move $a3, $zero
	
	1:
		addu $v0, $a1, $a3
		lbu $a0, 0($v0)
		addu $v1, $t0, $a3
		addiu $a3, $a3, 1
		bne $a3, $a2, 1b
		sb $a0, 0($v1)
	
	2:
		jr $ra
		move $v0, $t2
	
	3:
		addiu $v0, $a0, 3
		and $v0, $v0, $v1
		bne $a0, $v0, 0b
		srl $t1, $a2, 2
		
		beq $t1, $zero, 5f
		move $a3, $zero
		
		move $a3, $zero
		move $a0, $zero
	
	4:
		addu $v0, $a1, $a0
		lw $v1, 0($v0)
		addiu $a3, $a3, 1
		addu $v0, $t0, $a0
		sw $v1, 0($v0)
		bne $a3, $t1, 4b
		addiu $a0, $a0, 4
	
	5:
		andi $a2, $a2, 0x3
		beq $a2, $zero, 2b
		nop
		
		sll $v0, $a3, 2
		addu $t1, $v0, $t0
		move $a3, $zero
		addu $t0, $v0, $a1
	
	6:
		addu $v0, $t0, $a3
		lbu $a0, 0($v0)
		addu $v1, $t1, $a3
		addiu $a3, $a3, 1
		bne $a3, $a2, 6b
		sb $a0, 0($v1)
		
		jr $ra
		move $v0, $t2

jump_to_kernel:
	#
	# TODO:
	# Make sure that the I-cache, D-cache and memory are mutually coherent
	# before passing control to the copied code.
	#
	j $a0
	nop
