/*
 * Copyright (c) 2013 Martin Decky
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

.text

.set noat
.set noreorder
.set nomacro

.global memset
.global memcpy

memset:
	move $v0, $a0
	beqz $a2, 0f
	addiu $t1, $a2, -1
	
	negu $t0, $a0
	andi $t0, $t0, 0x3
	sltu $v1, $a2, $t0
	bnez $v1, 1f
	andi $a1, $a1, 0xff
	
	7:
		sltiu $v1, $a2, 4
		beqz $v1, 2f
		move $v1, $v0
		
		 move $t0, $a2
	
	3:
		sb $a1, 0($v1)
		addiu $v1, $v1, 1
		subu $a3, $v1, $v0
		sltu $a3, $a3, $t0
		bnez $a3, 3b
		addiu $t1, $t1, -1
		
		beq $a2, $t0, 0f
		subu $a2, $a2, $t0
	
	8:
		srl $a0, $a2, 0x2
		sll $t4, $a0, 0x2
		beqz $t4, 4f
		sll $t2, $a1, 0x10
		
		sll $t3, $a1, 0x8
		or $t3, $a1, $t3
		sll $a3, $a1, 0x18
		or $t3, $t3, $t2
		or $t3, $t3, $a3
		addu $t0, $v0, $t0
		move $a3, $zero
	
	5:
		addiu $a3, $a3, 1
		sltu $t2, $a3, $a0
		sw $t3, 0($t0)
		bnez $t2, 5b
		addiu $t0, $t0, 4
		
		addu $v1, $v1, $t4
		beq $a2, $t4, 0f
		subu $t1, $t1, $t4
	
	4:
		addiu $t1, $t1, 1
		addu $t1, $v1, $t1
		sb $a1, 0($v1)
	
	6:
		addiu $v1, $v1, 1
		bnel $v1, $t1, 6b
		sb $a1, 0($v1)
	
	0:
		jr $ra
		nop
	
	1:
		j 7b
		move $t0, $a2
	
	2:
		bnez $t0, 3b
		nop
		
		j 8b
		subu $a2, $a2, $t0

memcpy:
	move $t2, $a0  /* save dst */
	
	addiu $v0, $a1, 3
	li $v1, -4  /* 0xfffffffffffffffc */
	and $v0, $v0, $v1
	beq $a1, $v0, 3f
	move $t0, $a0
	
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
