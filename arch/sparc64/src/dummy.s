#
# Copyright (C) 2005 Jakub Jermar
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

.global arch_late_init
.global arch_post_mm_init
.global arch_pre_mm_init
.global asm_delay_loop
.global before_thread_runs_arch
.global calibrate_delay_loop
.global context_restore_arch
.global context_save_arch
.global cpu_arch_init
.global cpu_halt
.global cpu_identify
.global cpu_print_report
.global cpu_sleep
.global fmath_dpow
.global fmath_fint
.global fmath_get_decimal_exponent
.global fmath_is_infinity
.global fmath_is_nan
.global fpu_context_restore
.global fpu_context_save
.global fpu_enable
.global fpu_init
.global frame_arch_init
.global memcpy
.global memsetb
.global page_arch_init
.global panic_printf
.global userspace

.global dummy

arch_late_init:
arch_post_mm_init:
arch_pre_mm_init:
asm_delay_loop:
before_thread_runs_arch:
calibrate_delay_loop:
context_restore_arch:
context_save_arch:
cpu_arch_init:
cpu_halt:
cpu_identify:
cpu_print_report:
cpu_sleep:
fmath_dpow:
fmath_fint:
fmath_get_decimal_exponent:
fmath_is_infinity:
fmath_is_nan:
fpu_context_restore:
fpu_context_save:
fpu_enable:
fpu_init:
frame_arch_init:
memcpy:
memsetb:
page_arch_init:
panic_printf:
userspace:


dummy:
0:
	b 0b
	nop
