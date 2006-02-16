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

#include <arch/asm/macro.h>
#include <arch/asm/spr.h>

.section K_TEXT_START

.global kernel_image_start

.org 0x0
/* 256 bytes of some data */
/* exception table - must use 'ba' instructions for branches,
 * because it is elsewhere than the linker thinks
 */
.space 4096

kernel_image_start:
	/* Initialize OFW, might be needed before relocate_kernel? */
	lis r4, ofw@ha
	addi r4, r4, ofw@l
	stw r5, 0(r4)
	
	bl ofw_init
	bl preboot_read_config

	bl relocate_kernel
__after_reloc:
	/* Set stack to some more meaningful value */
	/* TODO: This is hardcoded for PearPC, must be changed later */
	lis r1, 0x70
	b main_bsp
	
relocate_kernel:
	/* TODO: We _know_ that pearpc loads it to 8MB, and
	 * but it should be really generic
	 */
	lis r4, 0x80 /* r4 is where data was loaded - 8MB */
	
	bl to_real_mode
	/* Now we are in real mode, copy first block and jump to it,
	 * we are running in the loaded kernel now
	 * We still have in r3 physical load kernel address
	 */
	
	b __after_reloc /* We know in pearpc we are ok, so return,
	                 * otherwise we should relocate kernel
		         * here
	                 */

	

	
/* Turn off page translation
 * - assume that physical-loaded address is in r4
 */
to_real_mode:
	mflr r0
	
	lis   r5, ktext_start@ha // Expected start of kernel
	addi r5, r5, ktext_start@l
	
	add  r0, r4, r0  	
	sub  r0, r0, r5         // r0 now contains physical return address
	
	mfmsr r3
	andis. r3, r3, (~MSR_DR | MSR_IR) >> 16
	mtspr SPRN_SRR0, r0
	mtspr SPRN_SRR1, r3
	sync 			// Really needed? RFI should do it as well?
	RFI

