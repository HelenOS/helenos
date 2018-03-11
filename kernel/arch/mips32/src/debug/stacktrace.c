/*
 * Copyright (c) 2010 Jakub Jermar
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

/** @addtogroup mips32
 * @{
 */
/** @file
 */

/*
 * This stack tracing code is based on the suggested algorithm described on page
 * 3-27 and 3-28 of:
 *
 * SYSTEM V
 * APPLICATION BINARY INTERFACE
 *
 * MIPS RISC Processor
 * Supplement
 * 3rd Edition
 *
 * Unfortunately, GCC generates code which is not entirely compliant with this
 * method. For example, it places the "jr ra" instruction quite arbitrarily in
 * the middle of the function which makes the original algorithm unapplicable.
 *
 * We deal with this problem by simply not using those parts of the algorithm
 * that rely on the "jr ra" instruction occurring in the last basic block of a
 * function, which gives us still usable, but less reliable stack tracer. The
 * unreliability stems from the fact that under some circumstances it can become
 * confused and produce incorrect or incomplete stack trace. We apply extra
 * sanity checks so that the algorithm is still safe and should not crash the
 * system.
 *
 * Even though not perfect, our solution is pretty lightweight, especially when
 * compared with a prospective alternative solution based on additional
 * debugging information stored directly in the kernel image.
 */

#include <stacktrace.h>
#include <stddef.h>
#include <syscall/copy.h>
#include <typedefs.h>
#include <arch/debugger.h>
#include <print.h>

#define R0	0U
#define SP	29U
#define RA	31U

#define OP_SHIFT	26
#define RS_SHIFT	21
#define RT_SHIFT	16
#define RD_SHIFT	11

#define HINT_SHIFT	6
#define BASE_SHIFT	RS_SHIFT
#define IMM_SHIFT	0
#define OFFSET_SHIFT	IMM_SHIFT

#define RS_MASK		(0x1f << RS_SHIFT)
#define RT_MASK		(0x1f << RT_SHIFT)
#define RD_MASK		(0x1f << RD_SHIFT)
#define HINT_MASK	(0x1f << HINT_SHIFT)
#define BASE_MASK	RS_MASK
#define IMM_MASK	(0xffff << IMM_SHIFT)
#define OFFSET_MASK	IMM_MASK

#define RS_GET(inst)		(((inst) & RS_MASK) >> RS_SHIFT)
#define RD_GET(inst)		(((inst) & RD_MASK) >> RD_SHIFT)
#define IMM_GET(inst)		(int16_t)(((inst) & IMM_MASK) >> IMM_SHIFT)
#define BASE_GET(inst)		RS_GET(inst)
#define OFFSET_GET(inst)	IMM_GET(inst)

#define ADDU_R_SP_R0_TEMPL \
	((0x0 << OP_SHIFT) | (SP << RS_SHIFT) | (R0 << RT_SHIFT) | 0x21)
#define ADDU_SP_R_R0_TEMPL \
	((0x0 << OP_SHIFT) | (SP << RD_SHIFT) | (R0 << RT_SHIFT) | 0x21)
#define ADDI_SP_SP_IMM_TEMPL \
	((0x8 << OP_SHIFT) | (SP << RS_SHIFT) | (SP << RT_SHIFT))
#define ADDIU_SP_SP_IMM_TEMPL \
	((0x9 << OP_SHIFT) | (SP << RS_SHIFT) | (SP << RT_SHIFT))
#define JR_RA_TEMPL \
	((0x0 << OP_SHIFT) | (RA << RS_SHIFT) | (0x0 << HINT_SHIFT) | 0x8)
#define SW_RA_TEMPL \
	((0x2b << OP_SHIFT) | (RA << RT_SHIFT))

#define IS_ADDU_R_SP_R0(inst) \
	(((inst) & ~RD_MASK) == ADDU_R_SP_R0_TEMPL)
#define IS_ADDU_SP_R_R0(inst) \
	(((inst) & ~RS_MASK) == ADDU_SP_R_R0_TEMPL)
#define IS_ADDI_SP_SP_IMM(inst) \
	(((inst) & ~IMM_MASK) == ADDI_SP_SP_IMM_TEMPL)
#define IS_ADDIU_SP_SP_IMM(inst) \
	(((inst) & ~IMM_MASK) == ADDIU_SP_SP_IMM_TEMPL)
#define IS_JR_RA(inst) \
	(((inst) & ~HINT_MASK) == JR_RA_TEMPL)
#define IS_SW_RA(inst) \
	(((inst) & ~(BASE_MASK | OFFSET_MASK)) == SW_RA_TEMPL)

extern char ktext_start;
extern char ktext_end;

static bool bounds_check(uintptr_t pc)
{
	return (pc >= (uintptr_t) &ktext_start) &&
	    (pc < (uintptr_t) &ktext_end);
}

static bool
scan(stack_trace_context_t *ctx, uintptr_t *prev_fp, uintptr_t *prev_ra)
{
	uint32_t *inst = (void *) ctx->pc;
	bool has_fp = false;
	size_t frame_size;
	unsigned int fp = SP;

	do {
		inst--;
		if (!bounds_check((uintptr_t) inst))
			return false;
#if 0
		/*
		 * This is one of the situations in which the theory (ABI) does
		 * not meet the practice (GCC). GCC simply does not place the
		 * JR $ra instruction as dictated by the ABI, rendering the
		 * official stack tracing algorithm somewhat unapplicable.
		 */

		if (IS_ADDU_R_SP_R0(*inst)) {
			uint32_t *cur;
			fp = RD_GET(*inst);
			/*
			 * We have a candidate for frame pointer.
			 */

			/* Seek to the end of this function. */
			cur = inst + 1;
			while (!IS_JR_RA(*cur))
				cur++;

			/* Scan the last basic block */
			for (cur--; !is_jump(*(cur - 1)); cur--) {
				if (IS_ADDU_SP_R_R0(*cur) &&
				    (fp == RS_GET(*cur))) {
					has_fp = true;
				}
			}
			continue;
		}

		if (IS_JR_RA(*inst)) {
			if (!ctx->istate)
				return false;
			/*
			 * No stack frame has been allocated yet.
			 * Use the values stored in istate.
			 */
			if (prev_fp)
				*prev_fp = ctx->istate->sp;
			if (prev_ra)
				*prev_ra = ctx->istate->ra - 8;
			ctx->istate = NULL;
			return true;
		}
#endif

	} while ((!IS_ADDIU_SP_SP_IMM(*inst) && !IS_ADDI_SP_SP_IMM(*inst)) ||
	    (IMM_GET(*inst) >= 0));

	/*
	 * We are at the instruction which allocates the space for the current
	 * stack frame.
	 */
	frame_size = -IMM_GET(*inst);
	if (prev_fp)
		*prev_fp = ctx->fp + frame_size;

	/*
	 * Scan the first basic block for the occurrence of
	 * SW $ra, OFFSET($base).
	 */
	for (inst++; !is_jump(*(inst - 1)) && (uintptr_t) inst < ctx->pc;
	    inst++) {
		if (IS_SW_RA(*inst)) {
			unsigned int base = BASE_GET(*inst);
			int16_t offset = OFFSET_GET(*inst);

			if (base == SP || (has_fp && base == fp)) {
				uint32_t *addr = (void *) (ctx->fp + offset);

				if (offset % 4 != 0)
					return false;
				/* cannot store below current stack pointer */
				if (offset < 0)
					return false;
				/* too big offsets are suspicious */
				if ((size_t) offset > sizeof(istate_t))
					return false;

				if (prev_ra)
					*prev_ra = *addr;
				return true;
			}
		}
	}

	/*
	 * The first basic block does not save the return address or saves it
	 * after ctx->pc, which means that the correct value is in istate.
	 */
	if (prev_ra) {
		if (!ctx->istate)
			return false;
		*prev_ra = ctx->istate->ra - 8;
		ctx->istate = NULL;
	}
	return true;
}


bool kernel_stack_trace_context_validate(stack_trace_context_t *ctx)
{
	return !((ctx->fp == 0) || ((ctx->fp % 8) != 0) ||
	    (ctx->pc % 4 != 0) || !bounds_check(ctx->pc));
}

bool kernel_frame_pointer_prev(stack_trace_context_t *ctx, uintptr_t *prev)
{
	return scan(ctx, prev, NULL);
}

bool kernel_return_address_get(stack_trace_context_t *ctx, uintptr_t *ra)
{
	return scan(ctx, NULL, ra);
}

bool uspace_stack_trace_context_validate(stack_trace_context_t *ctx)
{
	return false;
}

bool uspace_frame_pointer_prev(stack_trace_context_t *ctx, uintptr_t *prev)
{
	return false;
}

bool uspace_return_address_get(stack_trace_context_t *ctx, uintptr_t *ra)
{
	return false;
}

/** @}
 */
