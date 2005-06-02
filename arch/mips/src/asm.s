#
# Copyright (C) 2001-2004 Jakub Jermar
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

cp0_epc_read:		cp0_read $14
cp0_epc_write:		cp0_write $14

cp0_prid_read:		cp0_read $15


.global tlbp
tlbp:
	tlbp
	j $31
	nop

.global tlbr
tlbr:
	tlbr
	j $31
	nop

.global tlbwi
tlbwi:
	tlbwi
	j $31
	nop

.global tlbwr
tlbwr:
	tlbwr
	j $31
	nop

.global cpu_halt
cpu_halt:
	j cpu_halt
	nop


.global memsetb
memsetb:
	j _memsetb
	nop

.global memcopy
memcopy:
	j _memcopy
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
 