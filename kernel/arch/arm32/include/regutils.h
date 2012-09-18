/*
 * Copyright (c) 2007 Petr Stepan
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
/**
 * @file
 * @brief Utilities for convenient manipulation with ARM registers.
 */

#ifndef KERN_arm32_REGUTILS_H_
#define KERN_arm32_REGUTILS_H_

#define STATUS_REG_IRQ_DISABLED_BIT  (1 << 7)
#define STATUS_REG_MODE_MASK         0x1f

/* COntrol register bit values see ch. B4.1.130 of ARM Architecture Reference
 * Manual ARMv7-A and ARMv7-R edition, page 1687 */
#define CP15_R1_MMU_EN            (1 << 0)
#define CP15_R1_ALIGN_CHECK_EN    (1 << 1)  /* Allow alignemnt check */
#define CP15_R1_CACHE_EN          (1 << 2)
#define CP15_R1_CP15_BARRIER_EN   (1 << 5)
#define CP15_R1_B_EN              (1 << 7)  /* ARMv6- only big endian switch */
#define CP15_R1_SWAP_EN           (1 << 10)
#define CP15_R1_BRANCH_PREDICT_EN (1 << 11)
#define CP15_R1_INST_CACHE_EN     (1 << 12)
#define CP15_R1_HIGH_VECTORS_EN   (1 << 13)
#define CP15_R1_ROUND_ROBIN_EN    (1 << 14)
#define CP15_R1_HW_ACCESS_FLAG_EN (1 << 17)
#define CP15_R1_WRITE_XN_EN       (1 << 19) /* Only if virt. supported */
#define CP15_R1_USPCE_WRITE_XN_EN (1 << 20) /* Only if virt. supported */
#define CP15_R1_FAST_IRQ_EN       (1 << 21) /* Disbale impl.specific features */
#define CP15_R1_UNALIGNED_EN      (1 << 22) /* Must be 1 on armv7 */
#define CP15_R1_IRQ_VECTORS_EN    (1 << 24)
#define CP15_R1_BIG_ENDIAN_EXC    (1 << 25)
#define CP15_R1_NMFI_EN           (1 << 27)
#define CP15_R1_TEX_REMAP_EN      (1 << 28)
#define CP15_R1_ACCESS_FLAG_EN    (1 << 29)
#define CP15_r1_THUMB_EXC_EN      (1 << 30)

/* ARM Processor Operation Modes */
#define USER_MODE        0x10
#define FIQ_MODE         0x11
#define IRQ_MODE         0x12
#define SUPERVISOR_MODE  0x13
#define ABORT_MODE       0x17
#define UNDEFINED_MODE   0x1b
#define SYSTEM_MODE      0x1f

/* [CS]PRS manipulation macros */
#define GEN_STATUS_READ(nm, reg) \
	static inline uint32_t nm## _status_reg_read(void) \
	{ \
		uint32_t retval; \
		\
		asm volatile ( \
			"mrs %[retval], " #reg \
			: [retval] "=r" (retval) \
		); \
		\
		return retval; \
	}

#define GEN_STATUS_WRITE(nm, reg, fieldname, field) \
	static inline void nm## _status_reg_ ##fieldname## _write(uint32_t value) \
	{ \
		asm volatile ( \
			"msr " #reg "_" #field ", %[value]" \
			:: [value] "r" (value) \
		); \
	}

/** Return the value of CPSR (Current Program Status Register). */
GEN_STATUS_READ(current, cpsr);

/** Set control bits of CPSR. */
GEN_STATUS_WRITE(current, cpsr, control, c);

/** Return the value of SPSR (Saved Program Status Register). */
GEN_STATUS_READ(saved, spsr);

#endif

/** @}
 */
