#
# Copyright (c) 2006 Martin Decky
# Copyright (c) 2006 Jakub Jermar
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

#include <stack.h>
#include <register.h>

#define INITIAL_STACK_SIZE 	8192

#define NWINDOWS		8

.register %g2, #scratch
.register %g3, #scratch
.register %g6, #scratch
.register %g7, #scratch

.section BOOTSTRAP, "ax"

.global start
start:
	b 1f
	nop

/*
 * This header forces SILO to load the image at 0x4000.
 * More precisely, SILO will think this is an old version of Linux.
 */
	.ascii "HdrS"
	.word 0
	.half 0
	.half 0
	.half 0
	.half 0
.global silo_ramdisk_image
silo_ramdisk_image:
	.word 0
.global silo_ramdisk_size
silo_ramdisk_size:
	.word 0

.align 8
1:	
	/*
	 * Disable interrupts and disable address masking.
	 */
	wrpr %g0, PSTATE_PRIV_BIT, %pstate
	
	wrpr %g0, NWINDOWS - 2, %cansave	! set maximum saveable windows
	wrpr %g0, 0, %canrestore		! get rid of windows we will never need again
	wrpr %g0, 0, %otherwin			! make sure the window state is consistent
	wrpr %g0, NWINDOWS - 1, %cleanwin	! prevent needless clean_window traps for kernel

	set initial_stack_top, %sp
	add %sp, -STACK_BIAS, %sp

	set ofw_cif, %l0
 
	call ofw_init		! initialize OpenFirmware
	stx %o4, [%l0]
	
	b bootstrap
	nop

.align STACK_ALIGNMENT
initial_stack:
	.space INITIAL_STACK_SIZE
initial_stack_top:
	.space STACK_WINDOW_SAVE_AREA_SIZE
