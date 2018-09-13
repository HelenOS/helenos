/*
 * Copyright (c) 2014 Jakub Jermar
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

#ifndef KERN_ARCH_ISTATE_STRUCT_H_
#define KERN_ARCH_ISTATE_STRUCT_H_

#define ISTATE_OFFSET_L0              0x00
#define ISTATE_OFFSET_L1              0x08
#define ISTATE_OFFSET_L2              0x10
#define ISTATE_OFFSET_L3              0x18
#define ISTATE_OFFSET_L4              0x20
#define ISTATE_OFFSET_L5              0x28
#define ISTATE_OFFSET_L6              0x30
#define ISTATE_OFFSET_L7              0x38
#define ISTATE_OFFSET_I0              0x40
#define ISTATE_OFFSET_I1              0x48
#define ISTATE_OFFSET_I2              0x50
#define ISTATE_OFFSET_I3              0x58
#define ISTATE_OFFSET_I4              0x60
#define ISTATE_OFFSET_I5              0x68
#define ISTATE_OFFSET_I6              0x70
#define ISTATE_OFFSET_I7              0x78
#define ISTATE_OFFSET_UNDEF_ARG       0x80
#define ISTATE_OFFSET_ARG6            0xb0
#define ISTATE_OFFSET_TNPC            0xb8
#define ISTATE_OFFSET_TPC             0xc0
#define ISTATE_OFFSET_TSTATE          0xc8
#define ISTATE_OFFSET_Y               0xd0
#define ISTATE_OFFSET_O0              0xd8
#define ISTATE_OFFSET_O1              0xe0
#define ISTATE_OFFSET_O2              0xe8
#define ISTATE_OFFSET_O3              0xf0
#define ISTATE_OFFSET_O4              0xf8
#define ISTATE_OFFSET_O5              0x100
#define ISTATE_OFFSET_O6              0x108
#define ISTATE_OFFSET_O7              0x110
#define ISTATE_OFFSET_TLB_TAG_ACCESS  0x118
#define ISTATE_SIZE                   0x120

#ifndef __ASSEMBLER__

#include <stdint.h>

#ifdef KERNEL
#include <typedefs.h>
#else
#include <stddef.h>
#endif

typedef struct istate {
	/*
	 * Window save area for locals and inputs. Required by ABI.
	 * Before using these, make sure that the corresponding register
	 * window has been spilled into memory, otherwise l0-l7 and
	 * i0-i7 will have undefined values.
	 */
	uint64_t l0;
	uint64_t l1;
	uint64_t l2;
	uint64_t l3;
	uint64_t l4;
	uint64_t l5;
	uint64_t l6;
	uint64_t l7;
	uint64_t i0;
	uint64_t i1;
	uint64_t i2;
	uint64_t i3;
	uint64_t i4;
	uint64_t i5;
	uint64_t i6;
	uint64_t i7;

	/*
	 * Six mandatory argument slots, required by the ABI, plus an
	 * optional argument slot for the 7th argument used by our
	 * syscalls. Since the preemptible handler is always passing
	 * integral arguments, undef_arg[0] - undef_arg[5] are always
	 * undefined.
	 */
	uint64_t undef_arg[6];
	uint64_t arg6;

	/*
	 * From this point onwards, the istate layout is not dicated by
	 * the ABI. The only requirement is the stack alignment.
	 */
	uint64_t tnpc;
	uint64_t tpc;
	uint64_t tstate;
	uint64_t y;

	/*
	 * At the moment, these are defined only when needed by the
	 * preemptible handler, so consider them undefined for now.
	 */
	uint64_t o0;
	uint64_t o1;
	uint64_t o2;
	uint64_t o3;
	uint64_t o4;
	uint64_t o5;
	uint64_t o6;
	uint64_t o7;

	/*
	 * I/DTLB Tag Access register or zero for non-MMU traps.
	 */
	uint64_t tlb_tag_access;
} istate_t;

#endif
#endif
