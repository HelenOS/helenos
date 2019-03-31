/*
 * Copyright (c) 2015 Petr Pavlu
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

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief ARM64 FPU context.
 */

#include <arch/regutils.h>
#include <fpu_context.h>

/** Initialize FPU functionality. */
void fpu_init(void)
{
	/*
	 * Set initial FPU state:
	 * o Registers v0-v31 are cleared.
	 * o FPCR value:
	 *   [31:27] - Reserved 0.
	 *   [26]    - AHP=0, IEEE half-precision format selected.
	 *   [25]    - DN=0, NaN operands propagate through to the output of a
	 *             floating-point operation.
	 *   [24]    - FZ=0, flush-to-zero mode disabled.
	 *   [23:22] - RMode=00, round to nearest mode.
	 *   [21:20] - Stride=00, this field has no function in AArch64 state.
	 *   [19]    - FZ16=0, flush-to-zero mode disabled.
	 *   [18:16] - Len=000, this field has no function in AArch64 state.
	 *   [15]    - IDE=0, input denormal FP exception is untrapped.
	 *   [14:13] - Reserved 0.
	 *   [12]    - IXE=0, inexact FP exception is untrapped.
	 *   [11]    - UFE=0, underflow FP exception is untrapped.
	 *   [10]    - OFE=0, overflow FP exception is untrapped.
	 *   [9]     - DZE=0, divide by zero FP exception is untrapped.
	 *   [8]     - IOE=0, invalid operation FP exception is untrapped.
	 *   [7:0]   - Reserved 0.
	 * o FPSR value:
	 *   [31]    - N=0, negative condition flag for AArch32.
	 *   [30]    - Z=0, zero condition flag for AArch32.
	 *   [29]    - C=0, carry condition flag for AArch32.
	 *   [28]    - V=0, overflow condition flag for AArch32.
	 *   [27]    - QC=0, cumulative saturation bit.
	 *   [26:8]  - Reserved 0.
	 *   [7]     - IDC=0, input denormal cumulative FP exception bit.
	 *   [6:5]   - Reserved 0.
	 *   [4]     - IXC=0, inexact cumulative FP exception bit.
	 *   [3]     - UFC=0, underflow cumulative FP exception bit.
	 *   [2]     - OFC=0, overflow cumulative FP exception bit.
	 *   [1]     - DZC=0, divide by zero cumulative FP exception bit.
	 *   [0]     - IOC=0, invalid operation cumulative FP exception bit.
	 */
	static fpu_context_t init = { .vregs = { 0 }, .fpcr = 0, .fpsr = 0 };
	fpu_context_restore(&init);
}

/** Enable FPU instructions. */
void fpu_enable(void)
{
	CPACR_EL1_write((CPACR_EL1_read() & ~CPACR_FPEN_MASK) |
	    (CPACR_FPEN_TRAP_NONE << CPACR_FPEN_SHIFT));
}

/** Disable FPU instructions. */
void fpu_disable(void)
{
	CPACR_EL1_write((CPACR_EL1_read() & ~CPACR_FPEN_MASK) |
	    (CPACR_FPEN_TRAP_ALL << CPACR_FPEN_SHIFT));
}

/** @}
 */
