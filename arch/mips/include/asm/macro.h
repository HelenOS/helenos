/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

/*
 * MIPS assembler macros
 */

.macro REGISTERS_STORE r
	sw $0,0(\r)
	sw $1,4(\r)
	sw $2,8(\r)	# meaningless
	sw $3,12(\r)
	sw $4,16(\r)
	sw $5,20(\r)
	sw $6,24(\r)
	sw $7,28(\r)
	sw $8,32(\r)
	sw $9,36(\r)
	sw $10,40(\r)
	sw $11,44(\r)
	sw $12,48(\r)
	sw $13,52(\r)
	sw $14,56(\r)
	sw $15,60(\r)
	sw $16,64(\r)
	sw $17,68(\r)
	sw $18,72(\r)
	sw $19,76(\r)
	sw $20,80(\r)
	sw $21,84(\r)
	sw $22,88(\r)
	sw $23,92(\r)
	sw $24,96(\r)
	sw $25,100(\r)
	sw $26,104(\r)
	sw $27,108(\r)
	sw $28,112(\r)
	sw $29,116(\r)
	sw $30,120(\r)
	sw $31,124(\r)
.endm

.macro REGISTERS_LOAD r
	lw $0,0(\r)
	lw $1,4(\r)
	lw $2,8(\r)  # meaningless
	lw $3,12(\r)
	lw $4,16(\r) # this is ok, $4 == 16(\r)
	lw $5,20(\r)
	lw $6,24(\r)
	lw $7,28(\r)
	lw $8,32(\r)
	lw $9,36(\r)
	lw $10,40(\r)
	lw $11,44(\r)
	lw $12,48(\r)
	lw $13,52(\r)
	lw $14,56(\r)
	lw $15,60(\r)
	lw $16,64(\r)
	lw $17,68(\r)
	lw $18,72(\r)
	lw $19,76(\r)
	lw $20,80(\r)
	lw $21,84(\r)
	lw $22,88(\r)
	lw $23,92(\r)
	lw $24,96(\r)
	lw $25,100(\r)
	lw $26,104(\r)
	lw $27,108(\r)
	lw $28,112(\r)
	lw $29,116(\r)
	lw $30,120(\r)
	lw $31,124(\r)
.endm
