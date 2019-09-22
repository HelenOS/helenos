#
# Copyright (c) 2006 Martin Decky
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

#include <abi/asmtool.h>
#include <arch/arch.h>
#include <arch/regname.h>

.set noat
.set noreorder

.section BOOTSTRAP

/*
 * Registers on entry:
 *
 *   a0: kernel argument count (kargc)
 *   a1: kernel argument vector (kargv) of kargc elements
 *   a2: unused here (kenvp on Malta)
 *   a3: RAM size on Malta
 *
 * This is the case on Malta.
 * msim clears these registers, so it is de facto correct as well.
 */
SYMBOL(start)
	/*
	 * Setup the CP0 configuration
	 *  - Disable 64-bit kernel addressing mode
	 *  - Disable 64-bit supervisor adressing mode
	 *  - Disable 64-bit user addressing mode
	 */
	mfc0 $t0, $status
	la $t1, 0xffffff1f
	and $t0, $t1, $t0
	mtc0 $t0, $status

#if defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
	/*
	 * Remember the size of the SDRAM in bootinfo.
	 */
	la $t0, PA2KA(BOOTINFO_OFFSET)
	sw $a3, 0($t0)
#endif

	/*
	 * Setup CPU map (on msim this code
	 * is executed in parallel on all CPUs,
	 * but it not an issue).
	 */
	la $t0, PA2KA(CPUMAP_OFFSET)

	sw $zero, 0($t0)
	sw $zero, 4($t0)
	sw $zero, 8($t0)
	sw $zero, 12($t0)

	sw $zero, 16($t0)
	sw $zero, 20($t0)
	sw $zero, 24($t0)
	sw $zero, 28($t0)

	sw $zero, 32($t0)
	sw $zero, 36($t0)
	sw $zero, 40($t0)
	sw $zero, 44($t0)

	sw $zero, 48($t0)
	sw $zero, 52($t0)
	sw $zero, 56($t0)
	sw $zero, 60($t0)

	sw $zero, 64($t0)
	sw $zero, 68($t0)
	sw $zero, 72($t0)
	sw $zero, 76($t0)

	sw $zero, 80($t0)
	sw $zero, 84($t0)
	sw $zero, 88($t0)
	sw $zero, 92($t0)

	sw $zero, 96($t0)
	sw $zero, 100($t0)
	sw $zero, 104($t0)
	sw $zero, 108($t0)

	sw $zero, 112($t0)
	sw $zero, 116($t0)
	sw $zero, 120($t0)
	sw $zero, 124($t0)

	lui $t1, 1

#ifdef MACHINE_msim

	/* Read dorder value */
	la $k0, MSIM_DORDER_ADDRESS
	lw $k1, ($k0)

	/*
	 * If we are not running on BSP
	 * then end in an infinite loop.
	 */
	beq $k1, $zero, bsp
	nop

	/* Record CPU presence */
	sll $t2, $k1, 2
	addu $t2, $t2, $t0
	sw $t1, ($t2)

	loop:
		j loop
		nop

#endif

	bsp:
		/* Record CPU presence */
		sw $t1, ($t0)

		/* Setup initial stack */
		la $sp, PA2KA(STACK_OFFSET)

		/* a0=kargc, a1=kargv */
		j bootstrap
		nop

.text

FUNCTION_BEGIN(halt)
	j halt
	nop
FUNCTION_END(halt)

FUNCTION_BEGIN(jump_to_kernel)
	j $a0
	nop
FUNCTION_END(jump_to_kernel)
