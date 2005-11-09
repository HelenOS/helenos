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

#
# Init code for application processors.
#

#include <arch/boot/boot.h>
#include <arch/pm.h>
#include <arch/cpu.h>
#include <arch/cpuid.h>
#include <arch/mm/page.h>
	
.section K_TEXT_START_2, "ax"

#ifdef CONFIG_SMP

.global ap_boot

# This piece of code is real-mode and is meant to be alligned at 4K boundary.
# The requirement for such an alignment comes from MP Specification's STARTUP IPI
# requirements.

.align 4096
ap_boot:
.code16
	cli
	xorw %ax, %ax
	movw %ax, %ds

	lgdt real_bootstrap_gdtr_boot	# initialize Global Descriptor Table register
	
	movl %cr0, %eax
	orl $1, %eax
	movl %eax, %cr0			# switch to protected mode
	
	jmpl $gdtselector(KTEXT32_DES), $now_in_prot

.code32	
now_in_prot:
	movw $gdtselector(KDATA_DES), %ax
	movw %ax, %ds
	movw %ax, %ss
	

	# Enable 64-bit page transaltion entries - CR4.PAE = 1.
	# Paging is not enabled until after long mode is enabled
	movl %cr4, %eax
	btsl $5, %eax
	movl %eax, %cr4

	# Set up NEW paging tables, that are
	# already moved BOOT_OFFSET up
	leal ptl_0+BOOT_OFFSET, %eax
	movl %eax, %cr3
	
	# Enable long mode
	movl $EFER_MSR_NUM, %ecx   # EFER MSR number
	rdmsr                   # Read EFER
	btsl $AMD_LME_FLAG, %eax            # Set LME=1
	wrmsr                   # Write EFER
	
	# Enable paging to activate long mode (set CR0.PG=1)
	movl %cr0, %eax
	btsl $31, %eax
	movl %eax, %cr0
	
	# At this point we are in compatibility mode
	jmpl $gdtselector(KTEXT_DES), $start64

.code64
start64:
	movq (ctx), %rsp
	call main_ap   # never returns
	
	
#endif /* CONFIG_SMP */
