/*
 * SPDX-FileCopyrightText: 2007 Pavel Jancik, Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32_mm
 * @{
 */
/** @file
 *  @brief Page fault related declarations.
 */

#ifndef KERN_arm32_PAGE_FAULT_H_
#define KERN_arm32_PAGE_FAULT_H_

#include <stdint.h>

/** Decribes CP15 "fault status register" (FSR).
 *
 * "VMSAv6 added a fifth fault status bit (bit[10]) to both the IFSR and DFSR.
 * It is IMPLEMENTATION DEFINED how this bit is encoded in earlier versions of
 * the architecture. A write flag (bit[11] of the DFSR) has also been
 * introduced."
 * ARM Architecture Reference Manual version i ch. B4.6 (PDF p. 719)
 *
 * See ARM Architecture Reference Manual ch. B4.9.6 (pdf p.743). for FSR info
 */
typedef union {
	struct {
		unsigned status : 4;
		unsigned domain : 4;
		unsigned zero : 1;
		unsigned lpae : 1; /**< Needs LPAE support implemented */
		unsigned fs : 1; /**< armv6+ mandated, earlier IPLM. DEFINED */
		unsigned wr : 1; /**< armv6+ only */
		unsigned ext : 1; /**< external abort */
		unsigned cm : 1; /**< Cache maintenance, needs LPAE support */
		unsigned should_be_zero : 18;
	} data;
	struct {
		unsigned status : 4;
		unsigned sbz0 : 6;
		unsigned fs : 1;
		unsigned should_be_zero : 21;
	} inst;
	uint32_t raw;
} fault_status_t;

/** Simplified description of instruction code.
 *
 * @note Used for recognizing memory access instructions.
 * @see ARM architecture reference (chapter 3.1)
 */
typedef struct {
	unsigned dummy1 : 4;
	unsigned bit4 : 1;
	unsigned bits567 : 3;
	unsigned dummy : 12;
	unsigned access : 1;
	unsigned opcode : 4;
	unsigned type : 3;
	unsigned condition : 4;
} ATTRIBUTE_PACKED instruction_t;

/** Help union used for casting pc register (uint_32_t) value into
 *  #instruction_t pointer.
 */
typedef union {
	instruction_t *instr;
	uint32_t pc;
} instruction_union_t;

extern void prefetch_abort(unsigned int, istate_t *);
extern void data_abort(unsigned int, istate_t *);

#endif

/** @}
 */
