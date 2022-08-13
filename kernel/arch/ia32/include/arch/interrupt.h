/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32_interrupt
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_INTERRUPT_H_
#define KERN_ia32_INTERRUPT_H_

#include <arch/istate.h>
#include <arch/pm.h>
#include <stdint.h>

#define IVT_ITEMS  IDT_ITEMS
#define IVT_FIRST  0

#define EXC_COUNT  32
#define IRQ_COUNT  16

#define IVT_EXCBASE   0
#define IVT_IRQBASE   (IVT_EXCBASE + EXC_COUNT)
#define IVT_FREEBASE  (IVT_IRQBASE + IRQ_COUNT)

#define EXC_DE 0
#define EXC_DB 1
#define EXC_NM 7
#define EXC_SS 12
#define EXC_GP 13
#define EXC_PF 14
#define EXC_XM 19

#define IRQ_CLK       0
#define IRQ_KBD       1
#define IRQ_PIC1      2
/* NS16550 at COM1 */
#define IRQ_NS16550   4
#define IRQ_PIC0_SPUR 7
#define IRQ_MOUSE     12
#define IRQ_PIC1_SPUR 15

/* This one must have four least significant bits set to ones */
#define VECTOR_APIC_SPUR  (IVT_ITEMS - 1)

#if (((VECTOR_APIC_SPUR + 1) % 16) || VECTOR_APIC_SPUR >= IVT_ITEMS)
#error Wrong definition of VECTOR_APIC_SPUR
#endif

#define VECTOR_DE                 (IVT_EXCBASE + EXC_DE)
#define VECTOR_DB                 (IVT_EXCBASE + EXC_DB)
#define VECTOR_NM                 (IVT_EXCBASE + EXC_NM)
#define VECTOR_SS                 (IVT_EXCBASE + EXC_SS)
#define VECTOR_GP                 (IVT_EXCBASE + EXC_GP)
#define VECTOR_PF                 (IVT_EXCBASE + EXC_PF)
#define VECTOR_XM                 (IVT_EXCBASE + EXC_XM)
#define VECTOR_CLK                (IVT_IRQBASE + IRQ_CLK)
#define VECTOR_PIC0_SPUR          (IVT_IRQBASE + IRQ_PIC0_SPUR)
#define VECTOR_PIC1_SPUR          (IVT_IRQBASE + IRQ_PIC1_SPUR)
#define VECTOR_SYSCALL            IVT_FREEBASE
#define VECTOR_TLB_SHOOTDOWN_IPI  (IVT_FREEBASE + 1)
#define VECTOR_DEBUG_IPI          (IVT_FREEBASE + 2)

extern void interrupt_init(void);

#endif

/** @}
 */
