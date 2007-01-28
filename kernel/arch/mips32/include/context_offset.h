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

#ifndef KERN_mips32_CONTEXT_OFFSET_H_
#define KERN_mips32_CONTEXT_OFFSET_H_

#define OFFSET_SP      0x0
#define OFFSET_PC      0x4
#define OFFSET_S0      0x8
#define OFFSET_S1      0xc
#define OFFSET_S2      0x10
#define OFFSET_S3      0x14
#define OFFSET_S4      0x18
#define OFFSET_S5      0x1c
#define OFFSET_S6      0x20
#define OFFSET_S7      0x24
#define OFFSET_S8      0x28
#define OFFSET_GP      0x2c

/* istate_t */
#define EOFFSET_AT     0x0
#define EOFFSET_V0     0x4
#define EOFFSET_V1     0x8
#define EOFFSET_A0     0xc
#define EOFFSET_A1     0x10
#define EOFFSET_A2     0x14
#define EOFFSET_A3     0x18
#define EOFFSET_T0     0x1c
#define EOFFSET_T1     0x20
#define EOFFSET_T2     0x24
#define EOFFSET_T3     0x28
#define EOFFSET_T4     0x2c
#define EOFFSET_T5     0x30
#define EOFFSET_T6     0x34
#define EOFFSET_T7     0x38
#define EOFFSET_S0     0x3c
#define EOFFSET_S1     0x40
#define EOFFSET_S2     0x44
#define EOFFSET_S3     0x48
#define EOFFSET_S4     0x4c
#define EOFFSET_S5     0x50
#define EOFFSET_S6     0x54
#define EOFFSET_S7     0x58
#define EOFFSET_T8     0x5c
#define EOFFSET_T9     0x60
#define EOFFSET_GP     0x64
#define EOFFSET_SP     0x68
#define EOFFSET_S8     0x6c
#define EOFFSET_RA     0x70
#define EOFFSET_LO     0x74
#define EOFFSET_HI     0x78
#define EOFFSET_STATUS 0x7c
#define EOFFSET_EPC    0x80
#define EOFFSET_K1     0x84
#define REGISTER_SPACE 136

#endif
