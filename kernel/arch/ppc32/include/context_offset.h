/*
 * Copyright (c) 2005 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef KERN_ppc32_CONTEXT_OFFSET_H_
#define KERN_ppc32_CONTEXT_OFFSET_H_

#define OFFSET_SP    0x0
#define OFFSET_PC    0x4
#define OFFSET_R2    0x8
#define OFFSET_R13   0xc
#define OFFSET_R14   0x10
#define OFFSET_R15   0x14
#define OFFSET_R16   0x18
#define OFFSET_R17   0x1c
#define OFFSET_R18   0x20
#define OFFSET_R19   0x24
#define OFFSET_R20   0x28
#define OFFSET_R21   0x2c
#define OFFSET_R22   0x30
#define OFFSET_R23   0x34
#define OFFSET_R24   0x38
#define OFFSET_R25   0x3c
#define OFFSET_R26   0x40
#define OFFSET_R27   0x44
#define OFFSET_R28   0x48
#define OFFSET_R29   0x4c
#define OFFSET_R30   0x50
#define OFFSET_R31   0x54
#define OFFSET_CR    0x58

#define OFFSET_FR14  0x0
#define OFFSET_FR15  0x8
#define OFFSET_FR16  0x10
#define OFFSET_FR17  0x18
#define OFFSET_FR18  0x20
#define OFFSET_FR19  0x28
#define OFFSET_FR20  0x30
#define OFFSET_FR21  0x38
#define OFFSET_FR22  0x40
#define OFFSET_FR23  0x48
#define OFFSET_FR24  0x50
#define OFFSET_FR25  0x58
#define OFFSET_FR26  0x60
#define OFFSET_FR27  0x68
#define OFFSET_FR28  0x70
#define OFFSET_FR29  0x78
#define OFFSET_FR30  0x80
#define OFFSET_FR31  0x88
#define OFFSET_FPSCR 0x90

#endif
