#
# Copyright (c) 2008 Jakub Jermar
# Copyright (c) 2005-2006 Martin Decky
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

#
# Init code for application processors.
#

#include <abi/asmtool.h>
#include <arch/boot/boot.h>
#include <arch/boot/memmap.h>
#include <arch/mm/page.h>
#include <arch/pm.h>
#include <arch/cpu.h>
#include <arch/cpuid.h>
#include <arch/context_struct.h>

.section K_TEXT_START, "ax"

#ifdef CONFIG_SMP

# This piece of code is real-mode and is meant to be alligned at 4K boundary.
# The requirement for such an alignment comes from MP Specification's STARTUP
# IPI requirements.

.align 4096
SYMBOL(unmapped_ap_boot)
.code16
	cli
	xorw %ax, %ax
	movw %ax, %ds

	lgdtl ap_gdtr       # initialize Global Descriptor Table register

	movl %cr0, %eax
	orl $CR0_PE, %eax
	movl %eax, %cr0     # switch to protected mode
	jmpl $GDT_SELECTOR(KTEXT32_DES), $jump_to_kernel - BOOT_OFFSET + AP_BOOT_OFFSET

jump_to_kernel:
.code32
	movw $GDT_SELECTOR(KDATA_DES), %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $GDT_SELECTOR(UDATA_DES), %ax
	movw %ax, %gs

	# Enable 64-bit page translation entries (CR4.PAE = 1).
	# Paging is not enabled until after long mode is enabled

	movl %cr4, %eax
	orl $CR4_PAE, %eax
	movl %eax, %cr4

	leal ptl_0, %eax
	movl %eax, %cr3

	# Enable long mode
	movl $AMD_MSR_EFER, %ecx  # EFER MSR number
	rdmsr                     # Read EFER
	orl $AMD_LME, %eax        # Set LME=1
	wrmsr                     # Write EFER

	# Enable paging to activate long mode (set CR0.PG = 1)
	movl %cr0, %eax
	orl $CR0_PG, %eax
	movl %eax, %cr0

	# At this point we are in compatibility mode
	jmpl $GDT_SELECTOR(KTEXT_DES), $start64 - BOOT_OFFSET + AP_BOOT_OFFSET

.code64
start64:
	movabsq $bootstrap_stack_top, %rsp
	movq (%rsp), %rsp

	pushq $0
	pushq $0
	movq %rsp, %rbp

	movabsq $main_ap, %rax
	callq *%rax   # never returns

#endif /* CONFIG_SMP */

.section K_DATA_START, "aw", @progbits

#ifdef CONFIG_SMP

SYMBOL(unmapped_ap_gdtr)
	.word 0
	.long 0

#endif /* CONFIG_SMP */
