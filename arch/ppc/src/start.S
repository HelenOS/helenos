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

.section K_TEXT_START

.global kernel_image_start

kernel_image_start:
	lis r4, ofw@ha
	addi r4, r4, ofw@l
	stw r5, 0(r4)
	
	lis r3, _hardcoded_ktext_size@ha
	addi r3, r3, _hardcoded_ktext_size@l
	lis r4, hardcoded_ktext_size@ha
	addi r4, r4, hardcoded_ktext_size@l
	stw r3, 0(r4)
	
	lis r3, _hardcoded_kdata_size@ha
	addi r3, r3, _hardcoded_kdata_size@l
	lis r4, hardcoded_kdata_size@ha
	addi r4, r4, hardcoded_kdata_size@l
	stw r3, 0(r4)
	
	lis r3, _hardcoded_load_address@ha
	addi r3, r3, _hardcoded_load_address@l
	lis r4, hardcoded_load_address@ha
	addi r4, r4, hardcoded_load_address@l
	stw r3, 0(r4)
	
	bl ofw_init
	
	b main_bsp
