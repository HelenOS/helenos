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
#include <arch/types.h>
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
	FPU_VFPv3_COMMONv2 = 0x02,
	FPU_VFPv3_NO_COMMON = 0x3, /* Does not support fpu exc. traps */
	FPU_VFPv3_COMMONv3 = 0x4,
};

enum {
	FPEXC_EX_FLAG = (1 << 31),
	FPEXC_ENABLED_FLAG = (1 << 30),
};

/* See Architecture reference manual ch. B4.1.40 */
enum {
	CPACR_CP10_MASK = 0x3 << 20,
	CPACR_CP11_MASK = 0x3 << 22,
	CPACR_CP10_USER_ACCESS = CPACR_CP10_MASK,
	CPACR_CP11_USER_ACCESS = CPACR_CP11_MASK,
};

/** ARM Architecture Reference Manual ch. B4.1.58, p. B$-1551 */
enum {
	FPSCR_N_FLAG = (1 << 31),
	FPSCR_Z_FLAG = (1 << 30),
	FPSCR_C_FLAG = (1 << 29),
	FPSCR_V_FLAG = (1 << 28),
	FPSCR_QC_FLAG = (1 << 27),
	FPSCR_AHP_FLAG = (1 << 26),
	FPSCR_DN_FLAG = (1 << 25),
	FPSCR_FZ_FLAG = (1 << 24),
	FPSCR_ROUND_MODE_MASK = (0x3 << 22),
	FPSCR_ROUND_TO_NEAREST = (0x0 << 22),
	FPSCR_ROUND_TO_POS_INF = (0x1 << 22),
	FPSCR_ROUND_TO_NEG_INF = (0x2 << 22),
	FPSCR_ROUND_TO_ZERO = (0x3 << 22),
	FPSCR_STRIDE_MASK = (0x3 << 20),
	FPSCR_STRIDE_SHIFT = 20,
	FPSCR_LEN_MASK = (0x7 << 16),
	FPSCR_LEN_SHIFT = 16,
	FPSCR_DENORMAL_EN_FLAG = (1 << 15),
	FPSCR_INEXACT_EN_FLAG = (1 << 12),
	FPSCR_UNDERFLOW_EN_FLAG = (1 << 11),
	FPSCR_OVERFLOW_EN_FLAG = (1 << 10),
	FPSCR_ZERO_DIV_EN_FLAG = (1 << 9),
	FPSCR_INVALID_OP_EN_FLAG = (1 << 8),
	FPSCR_DENORMAL_FLAG = (1 << 7),
	FPSCR_INEXACT_FLAG = (1 << 4),
	FPSCR_UNDERFLOW_FLAG = (1 << 3),
	FPSCR_OVERLOW_FLAG = (1 << 2),
	FPSCR_DIV_ZERO_FLAG = (1 << 1),
	FPSCR_INVALID_OP_FLAG = (1 << 0),

	FPSCR_EN_ALL = FPSCR_DENORMAL_EN_FLAG | FPSCR_INEXACT_EN_FLAG | FPSCR_UNDERFLOW_EN_FLAG | FPSCR_OVERFLOW_EN_FLAG | FPSCR_ZERO_DIV_EN_FLAG | FPSCR_INVALID_OP_EN_FLAG,
};

static inline uint32_t fpscr_read()
{
	uint32_t reg;
	asm volatile (
		"vmrs %0, fpscr\n"
		:"=r" (reg)::
	);
	return reg;
}

static inline void fpscr_write(uint32_t val)
{
	asm volatile (
		"vmsr fpscr, %0\n"
		::"r" (val):
	);
}

static inline uint32_t fpexc_read()
{
	uint32_t reg;
	asm volatile (
		"vmrs %0, fpexc\n"
		:"=r" (reg)::
	);
	return reg;
}

static inline void fpexc_write(uint32_t val)
{
	asm volatile (
		"vmsr fpexc, %0\n"
		::"r" (val):
	);
}

static void (*save_context)(fpu_context_t *ctx);
static void (*restore_context)(fpu_context_t *ctx);

/** Saves 32 single precision fpu registers.
 * @param ctx FPU context area.
 * Used by VFPv1
 */
static void fpu_context_save_s32(fpu_context_t *ctx)
{
	asm volatile (
		"vmrs r1, fpexc\n"
		"vmrs r2, fpscr\n"
		"stmia %0!, {r1, r2}\n"
		"vstmia %0!, {s0-s31}\n"
		::"r" (ctx): "r1","r2","memory"
	);
}

/** Restores 32 single precision fpu registers.
 * @param ctx FPU context area.
 * Used by VFPv1
 */
static void fpu_context_restore_s32(fpu_context_t *ctx)
{
	asm volatile (
		"ldmia %0!, {r1, r2}\n"
		"vmsr fpexc, r1\n"
		"vmsr fpscr, r2\n"
		"vldmia %0!, {s0-s31}\n"
		::"r" (ctx): "r1","r2"
	);
}

/** Saves 16 double precision fpu registers.
 * @param ctx FPU context area.
 * Used by VFPv2, VFPv3-d16, and VFPv4-d16.
 */
static void fpu_context_save_d16(fpu_context_t *ctx)
{
	asm volatile (
		"vmrs r1, fpexc\n"
		"vmrs r2, fpscr\n"
		"stmia %0!, {r1, r2}\n"
		"vstmia %0!, {d0-d15}\n"
		::"r" (ctx): "r1","r2","memory"
	);
}

/** Restores 16 double precision fpu registers.
 * @param ctx FPU context area.
 * Used by VFPv2, VFPv3-d16, and VFPv4-d16.
 */
static void fpu_context_restore_d16(fpu_context_t *ctx)
{
	asm volatile (
		"ldmia %0!, {r1, r2}\n"
		"vmsr fpexc, r1\n"
		"vmsr fpscr, r2\n"
		"vldmia %0!, {d0-d15}\n"
		::"r" (ctx): "r1","r2"
	);
}

/** Saves 32 double precision fpu registers.
 * @param ctx FPU context area.
 * Used by VFPv3-d32, VFPv4-d32, and advanced SIMD.
 */
static void fpu_context_save_d32(fpu_context_t *ctx)
{
	asm volatile (
		"vmrs r1, fpexc\n"
		"stmia %0!, {r1}\n"
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
		"vmsr fpexc, r1\n"
		"ldmia %0!, {r1}\n"
		"vmsr fpscr, r1\n"
		"vldmia %0!, {d0-d15}\n"
		"vldmia %0!, {d16-d31}\n"
		::"r" (ctx): "r1"
	);
}

static int fpu_have_coprocessor_access()
{
	uint32_t cpacr;
	asm volatile ("MRC p15, 0, %0, c1, c0, 2" :"=r" (cpacr)::);
	/* FPU needs access to coprocessor 10 and 11.
	 * Moreover they need to have same access enabledd */
	if (((cpacr & CPACR_CP10_MASK) == CPACR_CP10_USER_ACCESS) &&
	   ((cpacr & CPACR_CP11_MASK) == CPACR_CP11_USER_ACCESS))
		return 1;

	return 0;
}

static void fpu_enable_coprocessor_access()
{
	uint32_t cpacr;
	asm volatile ("mrc p15, 0, %0, c1, c0, 2" :"=r" (cpacr)::);
	/* FPU needs access to coprocessor 10 and 11.
	 * Moreover, they need to have same access enabled */
	cpacr |= CPACR_CP10_USER_ACCESS;
	cpacr |= CPACR_CP11_USER_ACCESS;
	asm volatile ("mcr p15, 0, %0, c1, c0, 2" :"=r" (cpacr)::);
}


void fpu_init(void)
{
	/* Enable coprocessor access*/
	fpu_enable_coprocessor_access();

	/* Check if we succeeded */
	if (!fpu_have_coprocessor_access())
		return;

	/* Clear all fpu flags */
	fpexc_write(0);
	fpu_enable();
	/* Mask all exception traps,
	 * The bits are RAZ/WI on archs that don't support fpu exc traps.
	 */
	fpscr_write(fpscr_read() & ~FPSCR_EN_ALL);
}

void fpu_setup(void)
{
	/* Check if we have access */
	if (!fpu_have_coprocessor_access())
		return;

	uint32_t fpsid = 0;
	asm volatile (
		"vmrs %0, fpsid\n"
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
	case FPU_VFPv3_NO_COMMON:
	case FPU_VFPv3_COMMONv3: {
		uint32_t mvfr0 = 0;
		asm volatile (
			"vmrs %0,mvfr0\n"
			:"=r"(mvfr0)::
		);
		/* See page B4-1637 */
		if ((mvfr0 & 0xf) == 0x1) {
			printf("Detected VFPv3+ with 16 regs\n");
			save_context = fpu_context_save_d16;
			restore_context = fpu_context_restore_d16;
		} else {
			printf("Detected VFPv3+ with 32 regs\n");
			save_context = fpu_context_save_d32;
			restore_context = fpu_context_restore_d32;
		}
		break;
	}

	}
}

bool handle_if_fpu_exception(void)
{
	/* Check if we have access */
	if (!fpu_have_coprocessor_access())
		return false;

	const uint32_t fpexc = fpexc_read();
	if (fpexc & FPEXC_ENABLED_FLAG) {
		const uint32_t fpscr = fpscr_read();
		printf("FPU exception\n"
		    "\tFPEXC: %" PRIx32 " FPSCR: %" PRIx32 "\n", fpexc, fpscr);
		return false;
	}
#ifdef CONFIG_FPU_LAZY
	scheduler_fpu_lazy_request();
	return true;
#else
	return false;
#endif
}

void fpu_enable(void)
{
	/* Check if we have access */
	if (!fpu_have_coprocessor_access())
		return;
	/* Enable FPU instructions */
	fpexc_write(fpexc_read() | FPEXC_ENABLED_FLAG);
}

void fpu_disable(void)
{
	/* Check if we have access */
	if (!fpu_have_coprocessor_access())
		return;
	/* Disable FPU instructions */
	fpexc_write(fpexc_read() & ~FPEXC_ENABLED_FLAG);
}

void fpu_context_save(fpu_context_t *ctx)
{
	/* Check if we have access */
	if (!fpu_have_coprocessor_access())
		return;
	const uint32_t fpexc = fpexc_read();

	if (fpexc & FPEXC_EX_FLAG) {
		printf("EX FPU flag is on, things will fail\n");
		//TODO implement common subarch context saving
	}
	if (save_context)
		save_context(ctx);
}

void fpu_context_restore(fpu_context_t *ctx)
{
	/* Check if we have access */
	if (!fpu_have_coprocessor_access())
		return;
	if (restore_context)
		restore_context(ctx);
}
