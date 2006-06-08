#
# Copyright (C) 2005 Martin Decky
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
.section .init, "ax"
.global __entry
.global __entry_driver
.set noreorder
.option pic2

## User-space task entry point
#
#
.ent __entry
__entry:
	.frame $sp, 32, $31
	.cpload $25
	
	
	# Mips o32 may store its arguments on stack, make space (16 bytes),
	# so that it could work with -O0
	# Make space additional 16 bytes for the stack frame

	addiu $sp, -32
	.cprestore 16   # Allow PIC code
	
	jal __main
	nop
	
	jal __io_init
	nop
	
	jal main
	nop
	
	jal __exit
	nop
.end

.ent __entry_driver
__entry_driver:
	.frame $sp, 32, $31
	.cpload $25
	
	
	# Mips o32 may store its arguments on stack, make space (16 bytes),
	# so that it could work with -O0
	# Make space additional 16 bytes for the stack frame

	addiu $sp, -32
	.cprestore 16   # Allow PIC code
	
	jal __main
	nop
	
	jal main
	nop
	
	jal __exit
	nop
.end
# Alignment of output section data to 0x4000
.section .data
.align 14
