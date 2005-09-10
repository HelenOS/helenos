#
# Copyright (C) 2003-2004 Jakub Jermar
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

#include <arch/asm/regname.h>
	
.text

.macro cp0_read reg
	mfc0 $2,\reg
	j $31
	nop
.endm

.macro cp0_write reg
	mtc0 $4,\reg
	j $31
	nop
.endm

.set noat
.set noreorder
.set nomacro

.global cp0_index_read
.global cp0_index_write
.global cp0_random_read
.global cp0_entry_lo0_read
.global cp0_entry_lo0_write
.global cp0_entry_lo1_read
.global cp0_entry_lo1_write
.global cp0_context_read
.global cp0_context_write
.global cp0_pagemask_read
.global cp0_pagemask_write
.global cp0_wired_read
.global cp0_wired_write
.global cp0_badvaddr_read
.global cp0_count_read
.global cp0_count_write
.global cp0_hi_read
.global cp0_hi_write
.global cp0_compare_read
.global cp0_compare_write
.global cp0_status_read
.global cp0_status_write
.global cp0_cause_read
.global cp0_cause_write
.global cp0_epc_read
.global cp0_epc_write
.global cp0_prid_read

cp0_index_read:		cp0_read $0
cp0_index_write:	cp0_write $0

cp0_random_read:	cp0_read $1

cp0_entry_lo0_read:	cp0_read $2
cp0_entry_lo0_write:	cp0_write $2

cp0_entry_lo1_read:	cp0_read $3
cp0_entry_lo1_write:	cp0_write $3

cp0_context_read:	cp0_read $4
cp0_context_write:	cp0_write $4

cp0_pagemask_read:	cp0_read $5
cp0_pagemask_write:	cp0_write $5

cp0_wired_read:		cp0_read $6
cp0_wired_write:	cp0_write $6

cp0_badvaddr_read:	cp0_read $8

cp0_count_read:		cp0_read $9
cp0_count_write:	cp0_write $9

cp0_entry_hi_read:	cp0_read $10
cp0_entry_hi_write:	cp0_write $10

cp0_compare_read:	cp0_read $11
cp0_compare_write:	cp0_write $11

cp0_status_read:	cp0_read $12
cp0_status_write:	cp0_write $12

cp0_cause_read:		cp0_read $13
cp0_cause_write:	cp0_write $13

cp0_epc_read:		cp0_read $14
cp0_epc_write:		cp0_write $14

cp0_prid_read:		cp0_read $15


.global bios_write
bios_write:
	lw	$2, 0x80001020
	lw	$2, 0x6c($2)
	j	$2
	nop
	
.global cpu_halt
cpu_halt:
	j cpu_halt
	nop


.global memsetb
memsetb:
	j _memsetb
	nop

.global memcpy
memcpy:
	j _memcpy
	nop

.macro fpu_gp_save reg ctx
	mfc1 $t0,$\reg
	sw $t0, \reg*4(\ctx)
.endm

.macro fpu_gp_restore reg ctx
	lw $t0, \reg*4(\ctx)
	mtc1 $t0,$\reg
.endm

.macro fpu_ct_save reg ctx
	cfc1 $t0,$1
	sw $t0, (\reg+32)*4(\ctx)
.endm	

.macro fpu_ct_restore reg ctx
	lw $t0, (\reg+32)*4(\ctx)
	ctc1 $t0,$\reg
.endm


.global fpu_context_save
fpu_context_save:
#ifdef HAVE_FPU
	fpu_gp_save 0,$a0
	fpu_gp_save 1,$a0
	fpu_gp_save 2,$a0
	fpu_gp_save 3,$a0
	fpu_gp_save 4,$a0
	fpu_gp_save 5,$a0
	fpu_gp_save 6,$a0
	fpu_gp_save 7,$a0
	fpu_gp_save 8,$a0
	fpu_gp_save 9,$a0
	fpu_gp_save 10,$a0
	fpu_gp_save 11,$a0
	fpu_gp_save 12,$a0
	fpu_gp_save 13,$a0
	fpu_gp_save 14,$a0
	fpu_gp_save 15,$a0
	fpu_gp_save 16,$a0
	fpu_gp_save 17,$a0
	fpu_gp_save 18,$a0
	fpu_gp_save 19,$a0
	fpu_gp_save 20,$a0
	fpu_gp_save 21,$a0
	fpu_gp_save 22,$a0
	fpu_gp_save 23,$a0
	fpu_gp_save 24,$a0
	fpu_gp_save 25,$a0
	fpu_gp_save 26,$a0
	fpu_gp_save 27,$a0
	fpu_gp_save 28,$a0
	fpu_gp_save 29,$a0
	fpu_gp_save 30,$a0
	fpu_gp_save 31,$a0

	fpu_ct_save 1,$a0
	fpu_ct_save 2,$a0
	fpu_ct_save 3,$a0
	fpu_ct_save 4,$a0
	fpu_ct_save 5,$a0
	fpu_ct_save 6,$a0
	fpu_ct_save 7,$a0
	fpu_ct_save 8,$a0
	fpu_ct_save 9,$a0
	fpu_ct_save 10,$a0
	fpu_ct_save 11,$a0
	fpu_ct_save 12,$a0
	fpu_ct_save 13,$a0
	fpu_ct_save 14,$a0
	fpu_ct_save 15,$a0
	fpu_ct_save 16,$a0
	fpu_ct_save 17,$a0
	fpu_ct_save 18,$a0
	fpu_ct_save 19,$a0
	fpu_ct_save 20,$a0
	fpu_ct_save 21,$a0
	fpu_ct_save 22,$a0
	fpu_ct_save 23,$a0
	fpu_ct_save 24,$a0
	fpu_ct_save 25,$a0
	fpu_ct_save 26,$a0
	fpu_ct_save 27,$a0
	fpu_ct_save 28,$a0
	fpu_ct_save 29,$a0
	fpu_ct_save 30,$a0
	fpu_ct_save 31,$a0
#endif		
	j $ra
	nop

.global fpu_context_restore
fpu_context_restore:
#ifdef HAVE_FPU
	fpu_gp_restore 0,$a0
	fpu_gp_restore 1,$a0
	fpu_gp_restore 2,$a0
	fpu_gp_restore 3,$a0
	fpu_gp_restore 4,$a0
	fpu_gp_restore 5,$a0
	fpu_gp_restore 6,$a0
	fpu_gp_restore 7,$a0
	fpu_gp_restore 8,$a0
	fpu_gp_restore 9,$a0
	fpu_gp_restore 10,$a0
	fpu_gp_restore 11,$a0
	fpu_gp_restore 12,$a0
	fpu_gp_restore 13,$a0
	fpu_gp_restore 14,$a0
	fpu_gp_restore 15,$a0
	fpu_gp_restore 16,$a0
	fpu_gp_restore 17,$a0
	fpu_gp_restore 18,$a0
	fpu_gp_restore 19,$a0
	fpu_gp_restore 20,$a0
	fpu_gp_restore 21,$a0
	fpu_gp_restore 22,$a0
	fpu_gp_restore 23,$a0
	fpu_gp_restore 24,$a0
	fpu_gp_restore 25,$a0
	fpu_gp_restore 26,$a0
	fpu_gp_restore 27,$a0
	fpu_gp_restore 28,$a0
	fpu_gp_restore 29,$a0
	fpu_gp_restore 30,$a0
	fpu_gp_restore 31,$a0

	fpu_ct_restore 1,$a0
	fpu_ct_restore 2,$a0
	fpu_ct_restore 3,$a0
	fpu_ct_restore 4,$a0
	fpu_ct_restore 5,$a0
	fpu_ct_restore 6,$a0
	fpu_ct_restore 7,$a0
	fpu_ct_restore 8,$a0
	fpu_ct_restore 9,$a0
	fpu_ct_restore 10,$a0
	fpu_ct_restore 11,$a0
	fpu_ct_restore 12,$a0
	fpu_ct_restore 13,$a0
	fpu_ct_restore 14,$a0
	fpu_ct_restore 15,$a0
	fpu_ct_restore 16,$a0
	fpu_ct_restore 17,$a0
	fpu_ct_restore 18,$a0
	fpu_ct_restore 19,$a0
	fpu_ct_restore 20,$a0
	fpu_ct_restore 21,$a0
	fpu_ct_restore 22,$a0
	fpu_ct_restore 23,$a0
	fpu_ct_restore 24,$a0
	fpu_ct_restore 25,$a0
	fpu_ct_restore 26,$a0
	fpu_ct_restore 27,$a0
	fpu_ct_restore 28,$a0
	fpu_ct_restore 29,$a0
	fpu_ct_restore 30,$a0
	fpu_ct_restore 31,$a0
#endif	
	j $ra
	nop
	
# THIS IS USERSPACE CODE
.global utext
utext:
	j $31
	nop
utext_end:

.data
.global utext_size
utext_size:
	.long utext_end-utext
 

