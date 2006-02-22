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

#include <arch/boot/boot.h>

.register %g2, #scratch
.register %g3, #scratch
.register %g6, #scratch
.register %g7, #scratch

.section K_TEXT_START, "ax"

/*
 * Here is where the kernel is passed control.
 * The code must be position independent until
 * the kernel relocates itself to its VMA.
 */

.global kernel_image_start
kernel_image_start:
0:
	b 1f
	nop

/*
 * This header forces SILO to load the kernel at 0x4000.
 * More precisely, SILO will think this is an old version of Linux.
 */
.ascii "HdrS"
.word 0
.half 0

.align 8

1:
	flushw				! flush all but the active register window

	/*
	 * Prepare to copy the kernel image to 4M.
	 */

	set LMA, %g1			! source address
	set VMA, %g2			! destination address
	set hardcoded_ktext_size, %g3	! link address of hardcoded_ktext_size
	set hardcoded_kdata_size, %g4	! link address of hardcoded_kdata_size

	sub %g3, %g2, %g3		! offset of hardcoded_ktext_size from the beginning of the load address
	sub %g4, %g2, %g4		! offset of hardcoded_kdata_size from the beginning of the load address
	
	ldx [%g3 + %g1], %g3		! hardcoded_ktext_size -> %g3
	ldx [%g4 + %g1], %g4		! hardcoded_kdata_size -> %g4

	add %g3, %g4, %g3		! size of kernel text and data

	/*
	 * Start copying the kernel from higher addresses towards lower addresses.
	 */

2:	dec %g3
	ldub [%g1 + %g3], %g4
	stb %g4, [%g2 + %g3]
	cmp %g3, 0
	bnz 2b
	nop

	set relocated, %g1
	jmp %g1
	nop

relocated:
	set ofw, %l0

	call ofw_init
	stx %o4, [%l0]

	call ofw_init_memmap
	nop

	call main_bsp
	nop

	/* Not reached. */

2:
	b 2b
	nop
