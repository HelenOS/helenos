/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64_interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_INTERRUPT_H_
#define KERN_ia64_INTERRUPT_H_

#ifndef __ASSEMBLER__
#include <arch/istate.h>
#include <stdint.h>
#endif

#define EXC_ALT_ITLB_FAULT	0xc
#define EXC_ALT_DTLB_FAULT	0x10
#define EXC_NESTED_TLB_FAULT	0x14
#define EXC_DATA_D_BIT_FAULT	0x20
#define EXC_INST_A_BIT_FAULT	0x24
#define EXC_DATA_A_BIT_FAULT	0x28
#define EXC_BREAK_INSTRUCTION	0x2c
#define EXC_EXT_INTERRUPT	0x30
#define EXC_PAGE_NOT_PRESENT	0x50
#define EXC_DATA_AR_FAULT	0x53
#define EXC_GENERAL_EXCEPTION	0x54
#define EXC_DISABLED_FP_REG	0x55
#define EXC_SPECULATION		0x57

/** ia64 has 256 INRs. */
#define INR_COUNT  256

#define IVT_ITEMS  128
#define IVT_FIRST  0

/** External Interrupt vectors. */

#define VECTOR_TLB_SHOOTDOWN_IPI  0xf0

#define INTERRUPT_SPURIOUS  15
#define INTERRUPT_TIMER     255

#define LEGACY_INTERRUPT_BASE  0x20

#define IRQ_KBD    (0x01 + LEGACY_INTERRUPT_BASE)
#define IRQ_MOUSE  (0x0c + LEGACY_INTERRUPT_BASE)

/** General Exception codes. */
#define GE_ILLEGALOP     0
#define GE_PRIVOP        1
#define GE_PRIVREG       2
#define GE_RESREGFLD     3
#define GE_DISBLDISTRAN  4
#define GE_ILLEGALDEP    8

#define EOI  0  /**< The actual value doesn't matter. */

#ifndef __ASSEMBLER__
extern void *ivt;

extern void general_exception(unsigned int, istate_t *);
extern sysarg_t break_instruction(unsigned int, istate_t *);
extern void universal_handler(unsigned int, istate_t *);
extern void external_interrupt(unsigned int, istate_t *);
extern void disabled_fp_register(unsigned int, istate_t *);

void exception_init(void);
#endif

#endif

/** @}
 */
