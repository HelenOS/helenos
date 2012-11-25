/*
 * Copyright (c) 2012 Jan Vesely
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

/** @addtogroup arm32
 * @{
 */
/** @file
 *  @brief arm32 FPU context
 */

#include <fpu_context.h>
#include <arch.h>
#include <cpu.h>

#define FPSID_IMPLEMENTER(r)   ((r) >> 24)
#define FPSID_SW_ONLY_FLAG   (1 << 23)
#define FPSID_SUBACHITECTURE(r)   (((r) >> 16) & 0x7f)
#define FPSID_PART_NUMBER(r)   (((r) >> 8) & 0xff)
#define FPSID_VARIANT(r)   (((r) >> 4) 0xf)
#define FPSID_REVISION(r)   (((r) >> 0) 0xf)
enum {
	FPU_VFPv1 = 0x00,
	FPU_VFPv2_COMMONv1 = 0x01,
	FPU_VFPv3_COMMONv2 = 0x02, /* Check MVFR0 and MVFR 1*/
	FPU_VFPv3_NOTRAP = 0x3, /* Does not support trap */
	FPU_VFPv3 = 0x4,
};

static void (*save_context)(fpu_context_t *ctx);
static void (*restore_context)(fpu_context_t *ctx);

/** Saves 32 single precision fpu registers.
 * @param ctx FPU context area.
 * Used by VFPv1
 */
static void fpu_context_save_s32(fpu_context_t *ctx)
{
	asm volatile (
		"vmrs r1, fpscr\n"
		"stmia %0!, {r1}\n"
		"vstmia %0!, {s0-s31}\n"
		::"r" (ctx): "r1","memory"
	);
}

/** Restores 32 single precision fpu registers.
 * @param ctx FPU context area.
 * Used by VFPv1
 */
static void fpu_context_restore_s32(fpu_context_t *ctx)
{
	asm volatile (
		"ldmia %0!, {r1}\n"
		"vmsr fpscr, r1\n"
		"vldmia %0!, {s0-s31}\n"
		::"r" (ctx): "r1"
	);
}

/** Saves 16 double precision fpu registers.
 * @param ctx FPU context area.
 * Used by VFPv2, VFPv3-d16, and VFPv4-d16.
 */
static void fpu_context_save_d16(fpu_context_t *ctx)
{
	asm volatile (
		"vmrs r1, fpscr\n"
		"stmia %0!, {r1}\n"
		"vstmia %0!, {d0-d15}\n"
		::"r" (ctx): "r1","memory"
	);
}

/** Restores 16 double precision fpu registers.
 * @param ctx FPU context area.
 * Used by VFPv2, VFPv3-d16, and VFPv4-d16.
 */
static void fpu_context_restore_d16(fpu_context_t *ctx)
{
	asm volatile (
		"ldmia %0!, {r1}\n"
		"vmsr fpscr, r1\n"
		"vldmia %0!, {d0-d15}\n"
		::"r" (ctx): "r1"
	);
}

/** Saves 32 double precision fpu registers.
 * @param ctx FPU context area.
 * Used by VFPv3-d32, VFPv4-d32, and advanced SIMD.
 */
static void fpu_context_save_d32(fpu_context_t *ctx)
{
	asm volatile (
		"vmrs r1, fpscr\n"
		"stmia %0!, {r1}\n"
		"vstmia %0!, {d0-d15}\n"
		"vstmia %0!, {d16-d31}\n"
		::"r" (ctx): "r1","memory"
	);
}

/** Restores 32 double precision fpu registers.
 * @param ctx FPU context area.
 * Used by VFPv3-d32, VFPv4-d32, and advanced SIMD.
 */
static void fpu_context_restore_d32(fpu_context_t *ctx)
{
	asm volatile (
		"ldmia %0!, {r1}\n"
		"vmsr fpscr, r1\n"
		"vldmia %0!, {d0-d15}\n"
		"vldmia %0!, {d16-d31}\n"
		::"r" (ctx): "r1"
	);
}

void fpu_init(void)
{
	fpu_enable();
}

void fpu_setup(void)
{
	uint32_t fpsid = 0;
	asm volatile (
		"vmrs %0,fpsid\n"
		:"=r"(fpsid)::
	);
	if (fpsid & FPSID_SW_ONLY_FLAG) {
		printf("No FPU avaiable\n");
		return;
	}
	switch (FPSID_SUBACHITECTURE(fpsid))
	{
	case FPU_VFPv1:
		printf("Detected VFPv1\n");
		save_context = fpu_context_save_s32;
		restore_context = fpu_context_restore_s32;
		break;
	case FPU_VFPv2_COMMONv1:
		printf("Detected VFPv2\n");
		save_context = fpu_context_save_d16;
		restore_context = fpu_context_restore_d16;
		break;
	case FPU_VFPv3_COMMONv2:
	case FPU_VFPv3_NOTRAP:
	case FPU_VFPv3: {
		uint32_t mvfr0 = 0;
		asm volatile (
			"vmrs %0,mvfr0\n"
			:"=r"(mvfr0)::
		);
		/* See page B4-1637 */
		if ((mvfr0 & 0xf) == 0x1) {
			printf("Detected VFPv3+ with 32 regs\n");
			save_context = fpu_context_save_d32;
			restore_context = fpu_context_restore_d32;
		} else {
			printf("Detected VFPv3+ with 16 regs\n");
			save_context = fpu_context_save_d16;
			restore_context = fpu_context_restore_d16;
		}
		break;
	}

	}
}

void fpu_enable(void)
{
	/* Enable FPU instructions */
	asm volatile (
		"ldr r1, =0x40000000\n"
		"vmsr fpexc, r1\n"
		::: "r1"
	);
}

void fpu_disable(void)
{
	/* Disable FPU instructions */
	asm volatile (
		"ldr r1, =0x00000000\n"
		"vmsr fpexc, r1\n"
		::: "r1"
	);
}

void fpu_context_save(fpu_context_t *ctx)
{
	if (save_context)
		save_context(ctx);
}

void fpu_context_restore(fpu_context_t *ctx)
{
	if (restore_context)
		restore_context(ctx);
}
