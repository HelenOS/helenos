#
# Copyright (C) 2005 Jakub Vana
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



.macro Handler o h
.org IVT + \o
br \h;;
.endm

.macro Handler2 o 
.org IVT + \o
mov r12 = \o
br universal_handler;;
.endm



.global IVT
.align 32768
IVT:


Handler2 0x0000
Handler2 0x0400
Handler2 0x0800
Handler2 0x0c00
Handler2 0x1000
Handler2 0x1400
Handler2 0x1800
Handler2 0x1c00
Handler2 0x2000
Handler2 0x2400
Handler2 0x2800
Handler 0x2c00 break_instruction
Handler2 0x3000
Handler2 0x3400
Handler2 0x3800
Handler2 0x3c00
Handler2 0x4000
Handler2 0x4400
Handler2 0x4800
Handler2 0x4c00

Handler2 0x5000
Handler2 0x5100
Handler2 0x5200
Handler2 0x5300
Handler 0x5400 general_exception
Handler2 0x5500
Handler2 0x5600
Handler2 0x5700
Handler2 0x5800
Handler2 0x5900
Handler2 0x5a00
Handler2 0x5b00
Handler2 0x5c00
Handler2 0x5d00
Handler2 0x5e00
Handler2 0x5f00

Handler2 0x6000
Handler2 0x6100
Handler2 0x6200
Handler2 0x6300
Handler2 0x6400
Handler2 0x6500
Handler2 0x6600
Handler2 0x6700
Handler2 0x6800
Handler2 0x6900
Handler2 0x6a00
Handler2 0x6b00
Handler2 0x6c00
Handler2 0x6d00
Handler2 0x6e00
Handler2 0x6f00

Handler2 0x7000
Handler2 0x7100
Handler2 0x7200
Handler2 0x7300
Handler2 0x7400
Handler2 0x7500
Handler2 0x7600
Handler2 0x7700
Handler2 0x7800
Handler2 0x7900
Handler2 0x7a00
Handler2 0x7b00
Handler2 0x7c00
Handler2 0x7d00
Handler2 0x7e00
Handler2 0x7f00








.align 32768
.byte 0
