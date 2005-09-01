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




dump_gregs:
mov r16 = REG_DUMP;;
st8 [r16] = r0;;
add r16 = 8,r16 ;;
st8 [r16] = r1;;
add r16 = 8,r16 ;;
st8 [r16] = r2;;
add r16 = 8,r16 ;;
st8 [r16] = r3;;
add r16 = 8,r16 ;;
st8 [r16] = r4;;
add r16 = 8,r16 ;;
st8 [r16] = r5;;
add r16 = 8,r16 ;;
st8 [r16] = r6;;
add r16 = 8,r16 ;;
st8 [r16] = r7;;
add r16 = 8,r16 ;;
st8 [r16] = r8;;
add r16 = 8,r16 ;;
st8 [r16] = r9;;
add r16 = 8,r16 ;;
st8 [r16] = r10;;
add r16 = 8,r16 ;;
st8 [r16] = r11;;
add r16 = 8,r16 ;;
st8 [r16] = r12;;
add r16 = 8,r16 ;;
st8 [r16] = r13;;
add r16 = 8,r16 ;;
st8 [r16] = r14;;
add r16 = 8,r16 ;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;

bsw.1;;
mov r15 = r16;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r17;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r18;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r19;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r20;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r21;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r22;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r23;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r24;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r25;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r26;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r27;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r28;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r29;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r30;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;
bsw.1;;
mov r15 = r31;;
bsw.0;;
st8 [r16] = r15;;
add r16 = 8,r16 ;;


st8 [r16] = r32;;
add r16 = 8,r16 ;;
st8 [r16] = r33;;
add r16 = 8,r16 ;;
st8 [r16] = r34;;
add r16 = 8,r16 ;;
st8 [r16] = r35;;
add r16 = 8,r16 ;;
st8 [r16] = r36;;
add r16 = 8,r16 ;;
st8 [r16] = r37;;
add r16 = 8,r16 ;;
st8 [r16] = r38;;
add r16 = 8,r16 ;;
st8 [r16] = r39;;
add r16 = 8,r16 ;;
st8 [r16] = r40;;
add r16 = 8,r16 ;;
st8 [r16] = r41;;
add r16 = 8,r16 ;;
st8 [r16] = r42;;
add r16 = 8,r16 ;;
st8 [r16] = r43;;
add r16 = 8,r16 ;;
st8 [r16] = r44;;
add r16 = 8,r16 ;;
st8 [r16] = r45;;
add r16 = 8,r16 ;;
st8 [r16] = r46;;
add r16 = 8,r16 ;;
st8 [r16] = r47;;
add r16 = 8,r16 ;;
st8 [r16] = r48;;
add r16 = 8,r16 ;;
st8 [r16] = r49;;
add r16 = 8,r16 ;;
st8 [r16] = r50;;
add r16 = 8,r16 ;;
st8 [r16] = r51;;
add r16 = 8,r16 ;;
st8 [r16] = r52;;
add r16 = 8,r16 ;;
st8 [r16] = r53;;
add r16 = 8,r16 ;;
st8 [r16] = r54;;
add r16 = 8,r16 ;;
st8 [r16] = r55;;
add r16 = 8,r16 ;;
st8 [r16] = r56;;
add r16 = 8,r16 ;;
st8 [r16] = r57;;
add r16 = 8,r16 ;;
st8 [r16] = r58;;
add r16 = 8,r16 ;;
st8 [r16] = r59;;
add r16 = 8,r16 ;;
st8 [r16] = r60;;
add r16 = 8,r16 ;;
st8 [r16] = r61;;
add r16 = 8,r16 ;;
st8 [r16] = r62;;
add r16 = 8,r16 ;;
st8 [r16] = r63;;
add r16 = 8,r16 ;;



st8 [r16] = r64;;
add r16 = 8,r16 ;;
st8 [r16] = r65;;
add r16 = 8,r16 ;;
st8 [r16] = r66;;
add r16 = 8,r16 ;;
st8 [r16] = r67;;
add r16 = 8,r16 ;;
st8 [r16] = r68;;
add r16 = 8,r16 ;;
st8 [r16] = r69;;
add r16 = 8,r16 ;;
st8 [r16] = r70;;
add r16 = 8,r16 ;;
st8 [r16] = r71;;
add r16 = 8,r16 ;;
st8 [r16] = r72;;
add r16 = 8,r16 ;;
st8 [r16] = r73;;
add r16 = 8,r16 ;;
st8 [r16] = r74;;
add r16 = 8,r16 ;;
st8 [r16] = r75;;
add r16 = 8,r16 ;;
st8 [r16] = r76;;
add r16 = 8,r16 ;;
st8 [r16] = r77;;
add r16 = 8,r16 ;;
st8 [r16] = r78;;
add r16 = 8,r16 ;;
st8 [r16] = r79;;
add r16 = 8,r16 ;;
st8 [r16] = r80;;
add r16 = 8,r16 ;;
st8 [r16] = r81;;
add r16 = 8,r16 ;;
st8 [r16] = r82;;
add r16 = 8,r16 ;;
st8 [r16] = r83;;
add r16 = 8,r16 ;;
st8 [r16] = r84;;
add r16 = 8,r16 ;;
st8 [r16] = r85;;
add r16 = 8,r16 ;;
st8 [r16] = r86;;
add r16 = 8,r16 ;;
st8 [r16] = r87;;
add r16 = 8,r16 ;;
st8 [r16] = r88;;
add r16 = 8,r16 ;;
st8 [r16] = r89;;
add r16 = 8,r16 ;;
st8 [r16] = r90;;
add r16 = 8,r16 ;;
st8 [r16] = r91;;
add r16 = 8,r16 ;;
st8 [r16] = r92;;
add r16 = 8,r16 ;;
st8 [r16] = r93;;
add r16 = 8,r16 ;;
st8 [r16] = r94;;
add r16 = 8,r16 ;;
st8 [r16] = r95;;
add r16 = 8,r16 ;;



st8 [r16] = r96;;
add r16 = 8,r16 ;;
st8 [r16] = r97;;
add r16 = 8,r16 ;;
st8 [r16] = r98;;
add r16 = 8,r16 ;;
st8 [r16] = r99;;
add r16 = 8,r16 ;;
st8 [r16] = r100;;
add r16 = 8,r16 ;;
st8 [r16] = r101;;
add r16 = 8,r16 ;;
st8 [r16] = r102;;
add r16 = 8,r16 ;;
st8 [r16] = r103;;
add r16 = 8,r16 ;;
st8 [r16] = r104;;
add r16 = 8,r16 ;;
st8 [r16] = r105;;
add r16 = 8,r16 ;;
st8 [r16] = r106;;
add r16 = 8,r16 ;;
st8 [r16] = r107;;
add r16 = 8,r16 ;;
st8 [r16] = r108;;
add r16 = 8,r16 ;;
st8 [r16] = r109;;
add r16 = 8,r16 ;;
st8 [r16] = r110;;
add r16 = 8,r16 ;;
st8 [r16] = r111;;
add r16 = 8,r16 ;;
st8 [r16] = r112;;
add r16 = 8,r16 ;;
st8 [r16] = r113;;
add r16 = 8,r16 ;;
st8 [r16] = r114;;
add r16 = 8,r16 ;;
st8 [r16] = r115;;
add r16 = 8,r16 ;;
st8 [r16] = r116;;
add r16 = 8,r16 ;;
st8 [r16] = r117;;
add r16 = 8,r16 ;;
st8 [r16] = r118;;
add r16 = 8,r16 ;;
st8 [r16] = r119;;
add r16 = 8,r16 ;;
st8 [r16] = r120;;
add r16 = 8,r16 ;;
st8 [r16] = r121;;
add r16 = 8,r16 ;;
st8 [r16] = r122;;
add r16 = 8,r16 ;;
st8 [r16] = r123;;
add r16 = 8,r16 ;;
st8 [r16] = r124;;
add r16 = 8,r16 ;;
st8 [r16] = r125;;
add r16 = 8,r16 ;;
st8 [r16] = r126;;
add r16 = 8,r16 ;;
st8 [r16] = r127;;
add r16 = 8,r16 ;;



br.ret.sptk.many b0;;





.macro Handler o h
.org IVT + \o
br \h;;
.endm

.macro Handler2 o 
.org IVT + \o
br.call.sptk.many b0 = dump_gregs;;
mov r16 = \o ;;
bsw.1;;
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
#Handler 0x5400 general_exception
Handler2 0x5400
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
.global REG_DUMP

REG_DUMP:
.space 128*8

