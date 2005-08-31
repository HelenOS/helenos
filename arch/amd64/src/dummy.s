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

.global userspace
.global before_thread_runs_arch
.global panic_printf
.global cpu_identify
.global cpu_arch_init
.global cpu_sleep
.global cpu_print_report
.global get_memory_size
.global arch_late_init
.global calibrate_delay_loop
.global cpu_halt
.global page_arch_init
.global frame_arch_init
.global dummy
.global trap_register
.global trap_virtual_eoi
.global trap_virtual_enable_irqs
.global rdtsc
.global trap_virtual_disable_irqs
.global enable_irqs_function
.global disable_irqs_function
.global eoi_function
.global syscall
	
.global null_interrupt
.global interrupt_handler_size
.global gp_fault
.global nm_fault
.global ss_fault
.global interrupt_handlers
.global memcpy

null_interrupt:
interrupt_handler_size:
interrupt_handlers:	
gp_fault:
nm_fault:
ss_fault:	
eoi_function:
syscall:	
enable_irqs_function:
disable_irqs_function:	
rdtsc:
trap_virtual_eoi:
trap_virtual_enable_irqs:
trap_virtual_disable_irqs:	
trap_register:	
before_thread_runs_arch:
userspace:
calibrate_delay_loop:
panic_printf:
cpu_identify:
cpu_arch_init:
cpu_sleep:
cpu_print_report:
get_memory_size:
arch_late_init:
calibrate_delay_loop:
cpu_halt:
page_arch_init:
frame_arch_init:
memcpy:

dummy:
0:
	ret
