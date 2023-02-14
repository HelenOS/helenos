#
# Copyright (c) 2001-2004 Jakub Jermar
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

/*
 * Init code for application processors.
 */

#include <abi/asmtool.h>
#include <arch/boot/boot.h>
#include <arch/boot/memmap.h>
#include <arch/mm/page.h>
#include <arch/pm.h>
#include <arch/cpu.h>
#include <arch/context_struct.h>

.section K_TEXT_START, "ax"

#ifdef CONFIG_SMP

KTEXT=8
KDATA=16

/*
 * This piece of code is real-mode and is meant to be aligned at 4K boundary.
 * The requirement for such an alignment comes from MP Specification's
 * STARTUP IPI requirements.
 */

.align 4096
SYMBOL(unmapped_ap_boot)
.code16
	cli
	xorw %ax, %ax
	movw %ax, %ds

	/* initialize Global Descriptor Table register */
	lgdtl ap_gdtr

	/* switch to protected mode */
	movl %cr0, %eax
	orl $CR0_PE, %eax
	movl %eax, %cr0
	jmpl $KTEXT, $jump_to_kernel - BOOT_OFFSET + AP_BOOT_OFFSET

jump_to_kernel:
.code32
	movw $KDATA, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movl $KA2PA(bootstrap_stack_top), %eax   /* KA2PA((uintptr_t) &bootstrap_stack_top) */
	movl (%eax), %esp
	leal KA2PA(0)(%esp), %esp       /* KA2PA(ctx.sp) */

	/*
	 * Map kernel and turn paging on.
	 * We assume that when using SMP, PSE is always available
	 */
	call map_kernel_pse

	addl $PA2KA(0), %esp            /* PA2KA(ctx.sp) */

	/* create the first stack frame */
	pushl $0
	movl %esp, %ebp

	jmpl $KTEXT, $main_ap

#endif /* CONFIG_SMP */


.section K_DATA_START, "aw", @progbits

#ifdef CONFIG_SMP

SYMBOL(unmapped_ap_gdtr)
	.word 0
	.long 0

#endif /* CONFIG_SMP */
