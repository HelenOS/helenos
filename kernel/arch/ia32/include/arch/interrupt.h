/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup ia32interrupt
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
#define IRQ_PIC_SPUR  7
#define IRQ_MOUSE     12

/* This one must have four least significant bits set to ones */
#define VECTOR_APIC_SPUR  (IVT_ITEMS - 1)

#if (((VECTOR_APIC_SPUR + 1) % 16) || VECTOR_APIC_SPUR >= IVT_ITEMS)
#error Wrong definition of VECTOR_APIC_SPUR
#endif

#define VECTOR_DE                 (IVT_EXCBASE + EXC_DE)
#define VECTOR_NM                 (IVT_EXCBASE + EXC_NM)
#define VECTOR_SS                 (IVT_EXCBASE + EXC_SS)
#define VECTOR_GP                 (IVT_EXCBASE + EXC_GP)
#define VECTOR_PF                 (IVT_EXCBASE + EXC_PF)
#define VECTOR_XM                 (IVT_EXCBASE + EXC_XM)
#define VECTOR_CLK                (IVT_IRQBASE + IRQ_CLK)
#define VECTOR_PIC_SPUR           (IVT_IRQBASE + IRQ_PIC_SPUR)
#define VECTOR_SYSCALL            IVT_FREEBASE
#define VECTOR_TLB_SHOOTDOWN_IPI  (IVT_FREEBASE + 1)
#define VECTOR_DEBUG_IPI          (IVT_FREEBASE + 2)
#define VECTOR_SMP_CALL_IPI       (IVT_FREEBASE + 3)

extern void (*disable_irqs_function)(uint16_t);
extern void (*enable_irqs_function)(uint16_t);
extern void (*eoi_function)(void);
extern const char *irqs_info;

extern void interrupt_init(void);
extern void trap_virtual_enable_irqs(uint16_t);
extern void trap_virtual_disable_irqs(uint16_t);

#endif

/** @}
 */
