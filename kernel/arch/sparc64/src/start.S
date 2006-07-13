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

#include <arch/regdef.h>

.register %g2, #scratch
.register %g3, #scratch
.register %g6, #scratch
.register %g7, #scratch

.section K_TEXT_START, "ax"

/*
 * Here is where the kernel is passed control
 * from the boot loader.
 * 
 * The registers are expected to be in this state:
 * - %o0 bootinfo structure address
 * - %o1 bootinfo structure size
 *
 * Moreover, we depend on boot having established the
 * following environment:
 * - TLBs are on
 * - identity mapping for the kernel image
 * - identity mapping for memory stack
 */

.global kernel_image_start
kernel_image_start:
	flushw				! flush all but the active register window

	/*
	 * Disable interrupts and disable 32-bit address masking.
	 */
	rdpr %pstate, %l0
	and %l0, ~(PSTATE_AM_BIT|PSTATE_IE_BIT), %l0
	wrpr %l0, 0, %pstate

	/*
	 * Copy the bootinfo structure passed from the boot loader
	 * to the kernel bootinfo structure.
	 */
	mov %o1, %o2
	mov %o0, %o1
	set bootinfo, %o0
	call memcpy
	nop

	/*
	 * Take over control of identity mapping.
	 * Take over control of trap table.
	 *
	 * After this call, the kernel is entirely self-sufficient
	 * and independent on OpenFirmware.
	 */
	set kernel_image_start, %o0
	call take_over_tlb_and_tt
	nop

	wrpr %r0, 0, %pil

	call main_bsp
	nop

	/* Not reached. */

2:
	b 2b
	nop
