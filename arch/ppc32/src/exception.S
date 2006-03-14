#
# Copyright (C) 2006 Martin Decky
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
#include <arch/asm/spr.h>

.section K_UNMAPPED_TEXT_START, "ax"

.org 0x100
.global exc_system_reset
exc_system_reset:
	b exc_system_reset

.org 0x200
.global exc_machine_check
exc_machine_check:
	b exc_machine_check

.org 0x300
.global exc_data_storage
exc_data_storage:
	b exc_data_storage

.org 0x380
.global exc_data_segment
exc_data_segment:
	b exc_data_segment

.org 0x400
.global exc_instruction_storage
exc_instruction_storage:
	b exc_instruction_storage

.org 0x480
.global exc_instruction_segment
exc_instruction_segment:
	b exc_instruction_segment

.org 0x500
.global exc_external
exc_external:
	b exc_external

.org 0x600
.global exc_alignment
exc_alignment:
	b exc_alignment

.org 0x700
.global exc_program
exc_program:
	b exc_program

.org 0x800
.global exc_fp_unavailable
exc_fp_unavailable:
	b exc_fp_unavailable

.org 0x900
.global exc_decrementer
exc_decrementer:
	rfi
	b exc_decrementer

.org 0xa00
.global exc_reserved0
exc_reserved0:
	b exc_reserved0

.org 0xb00
.global exc_reserved1
exc_reserved1:
	b exc_reserved1

.org 0xc00
.global exc_syscall
exc_syscall:
	b exc_syscall

.org 0xd00
.global exc_trace
exc_trace:
	b exc_trace
