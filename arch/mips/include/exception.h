/*
 * Copyright (C) 2003-2004 Jakub Jermar
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

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#ifndef __mips_TYPES_H_
#  include <arch/types.h>
#endif

#define EXC_Int		0
#define EXC_Mod		1
#define EXC_TLBL	2
#define EXC_TLBS	3
#define EXC_AdEL	4
#define EXC_AdES	5
#define EXC_IBE		6
#define EXC_DBE		7
#define EXC_Sys		8
#define EXC_Bp		9
#define EXC_RI		10
#define EXC_CpU		11
#define EXC_Ov		12
#define EXC_Tr		13
#define EXC_VCEI	14
#define EXC_FPE		15
#define EXC_WATCH	23
#define EXC_VCED	31

struct exception_regdump {
	__u32 at;
	__u32 v0;
	__u32 v1;
	__u32 a0;
	__u32 a1;
	__u32 a2;
	__u32 a3;
	__u32 t0;
	__u32 t1;
	__u32 t2;
	__u32 t3;
	__u32 t4;
	__u32 t5;
	__u32 t6;
	__u32 t7;
	__u32 s0;
	__u32 s1;
	__u32 s2;
	__u32 s3;
	__u32 s4;
	__u32 s5;
	__u32 s6;
	__u32 s7;
	__u32 t8;
	__u32 t9;
	__u32 gp;
	__u32 sp;
	__u32 s8;
	__u32 ra;
	
	__u32 lo;
	__u32 hi;

	__u32 status; /* cp0_status */
	__u32 epc; /* cp0_epc */
};

extern void exception(struct exception_regdump *pstate);

#endif
