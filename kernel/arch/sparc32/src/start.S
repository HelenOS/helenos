#
# Copyright (c) 2013 Jakub Klama
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

.global kernel_image_start
.global early_putchar
.global kernel_sp
.global uspace_wbuf

kernel_image_start:
	# Install trap handlers
	set trap_table, %g1
	mov %g1, %tbr
	
	mov %psr, %g1           ! Initialize WIM
	add %g1, 1, %g2
	and %g2, 0x7, %g2
	set 1, %g3
	sll %g3, %g2, %g3
	mov %g3, %wim
	
	or %g1, 0x20, %g1
	or %g1, 0xf00, %g1      ! PIL
	wr %g1, %psr
	
	set boot_stack, %sp
	mov %sp, %fp
	sub %sp, 96, %sp
	
	mov %o1, %l1
	set 0xdeadbeef, %o0
	set 0xdeadbeef, %o1
	set 0, %o2
	call write_to_invalid
	nop
	
	mov %l1, %o1
	call arch_pre_main
	nop
	
	call main_bsp
	nop

early_putchar:
	set 0x80000100, %l0
	cmp %o0, '\n'
	bne skip
	
	set '\r', %l1
	sta %l1, [%l0] 0x1c
	
	skip:
		sta %o0, [%l0] 0x1c
		retl
		nop

kernel_sp:
.space 4

uspace_wbuf:
.space 4

.align 16
.space 4096
boot_stack:
