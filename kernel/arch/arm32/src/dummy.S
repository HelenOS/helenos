#
# Copyright (c) 2003-2004 Jakub Jermar
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

.global calibrate_delay_loop
.global asm_delay_loop
.global dummy
.global arch_grab_console
.global arch_release_console
.global cpu_halt
.global fpu_context_restore
.global fpu_context_save
.global fpu_enable
.global fpu_init
.global interrupts_disable
.global interrupts_enable
.global interrupts_read
.global interrupts_restore
.global memcpy
.global memcpy_from_uspace
.global memcpy_to_uspace
.global memsetb
.global panic_printf
.global symbol_table
.global sys_tls_set
.global tlb_invalidate_asid
.global tlb_invalidate_pages
.global userspace
	
calibrate_delay_loop:
asm_delay_loop:

arch_grab_console:
arch_release_console:
cpu_halt:
fpu_context_restore:
fpu_context_save:
fpu_enable:
fpu_init:
interrupts_disable:
interrupts_enable:
interrupts_read:
interrupts_restore:
memcpy:
memcpy_from_uspace:
memcpy_to_uspace:
memsetb:
panic_printf:
symbol_table:
sys_tls_set:
tlb_invalidate_asid:
tlb_invalidate_pages:
userspace:

dummy:

0:
	b 0b
