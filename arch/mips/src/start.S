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

#include <arch/asm/macro.h>

#define STACK_SPACE	128

.text

.set noat
.set noreorder
.set nomacro

.global kernel_image_start
.global tlb_refill_entry
.global cache_error_entry
.global exception_entry

.org 0x0
kernel_image_start:
tlb_refill_entry:
    j tlb_refill_handler
    nop

.org 0x100
cache_error_entry:
    j cache_error_handler
    nop
    
.org 0x180
exception_entry:
exception_handler:
    sub $29, STACK_SPACE
    REGISTERS_STORE $29
    
    jal exception
    nop
    
    REGISTERS_LOAD $29
    add $29, STACK_SPACE

    eret

tlb_refill_handler:
    sub $29, STACK_SPACE
    REGISTERS_STORE $29
    
    jal tlb_refill
    nop
    
    REGISTERS_LOAD $29
    add $29, STACK_SPACE
    
    eret

cache_error_handler:
    sub $29, STACK_SPACE
    REGISTERS_STORE $29

    jal cache_error
    nop
    
    REGISTERS_LOAD $29
    add $29, STACK_SPACE

    eret
