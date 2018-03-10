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
#include <arch/security_ext.h>
#include <arch/cp15.h>
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

extern uint32_t fpsid_read(void);
extern uint32_t mvfr0_read(void);

enum {
	FPEXC_EX_FLAG = (1 << 31),
	FPEXC_ENABLED_FLAG = (1 << 30),
};
extern uint32_t fpexc_read(void);
extern void fpexc_write(uint32_t);

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

extern uint32_t fpscr_read(void);
extern void fpscr_write(uint32_t);

extern void fpu_context_save_s32(fpu_context_t *);
extern void fpu_context_restore_s32(fpu_context_t *);
extern void fpu_context_save_d16(fpu_context_t *);
extern void fpu_context_restore_d16(fpu_context_t *);
extern void fpu_context_save_d32(fpu_context_t *);
extern void fpu_context_restore_d32(fpu_context_t *);

static void (*save_context)(fpu_context_t *ctx);
static void (*restore_context)(fpu_context_t *ctx);

static int fpu_have_coprocessor_access(void)
{
/*
 * The register containing the information (CPACR) is not available on armv6-
 * rely on user decision to use CONFIG_FPU.
 */
#ifdef PROCESSOR_ARCH_armv7_a
	const uint32_t cpacr = CPACR_read();
	/* FPU needs access to coprocessor 10 and 11.
	 * Moreover they need to have same access enabled */
	if (((cpacr & CPACR_CP_MASK(10)) != CPACR_CP_FULL_ACCESS(10)) &&
	   ((cpacr & CPACR_CP_MASK(11)) != CPACR_CP_FULL_ACCESS(11))) {
		printf("No access to CP10 and CP11: %" PRIx32 "\n", cpacr);
		return 0;
	}
#endif
	return 1;
}

/** Enable coprocessor access. Turn both non-secure mode bit and generic access.
 * Cortex A8 Manual says:
 * "You must execute an Instruction Memory Barrier (IMB) sequence immediately
 * after an update of the Coprocessor Access Control Register, see Memory
 * Barriers in the ARM Architecture Reference Manual. You must not attempt to
 * execute any instructions that are affected by the change of access rights
 * between the IMB sequence and the register update."
 * Cortex a8 TRM ch. 3.2.27. c1, Coprocessor Access Control Register
 *
 * @note do we need to call secure monitor here?
 */
static void fpu_enable_coprocessor_access(void)
{
/*
 * The register containing the information (CPACR) is not available on armv6-
 * rely on user decision to use CONFIG_FPU.
 */
#ifdef PROCESSOR_ARCH_armv7_a
	/* Allow coprocessor access */
	uint32_t cpacr = CPACR_read();
	/* FPU needs access to coprocessor 10 and 11.
	 * Moreover, they need to have same access enabled */
	cpacr &= ~(CPACR_CP_MASK(10) | CPACR_CP_MASK(11));
	cpacr |= (CPACR_CP_FULL_ACCESS(10) | CPACR_CP_FULL_ACCESS(11));
	CPACR_write(cpacr);
#endif
}


void fpu_init(void)
{
	/* Check if we have access */
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
	uint32_t mvfr0;

	/* Enable coprocessor access*/
	fpu_enable_coprocessor_access();

	/* Check if we succeeded */
	if (!fpu_have_coprocessor_access())
		return;

	const uint32_t fpsid = fpsid_read();
	if (fpsid & FPSID_SW_ONLY_FLAG) {
		printf("No FPU avaiable\n");
		return;
	}
	switch (FPSID_SUBACHITECTURE(fpsid)) {
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
	case FPU_VFPv3_COMMONv3:
		mvfr0 = mvfr0_read();
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
	/* This is only necessary if we enable fpu exceptions. */
#if 0
	const uint32_t fpexc = fpexc_read();

	if (fpexc & FPEXC_EX_FLAG) {
		printf("EX FPU flag is on, things will fail\n");
		//TODO implement common subarch context saving
	}
#endif
	if (save_context)
		save_context(ctx);
}

void fpu_context_restore(fpu_context_t *ctx)
{
	if (restore_context)
		restore_context(ctx);
}
