/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <arch/types.h>
#include <arch/pm.h>

#define IVT_ITEMS		IDT_ITEMS

#define IVT_EXCBASE		0
#define EXCLAST			31

#define IVT_IRQBASE		(IVT_EXCBASE+EXCLAST+1)
#define IRQLAST			15

#define IVT_FREEBASE		(IVT_IRQBASE+IRQLAST+1)

#define IRQ_CLK		0
#define IRQ_KBD		1
#define IRQ_PIC1	2
#define IRQ_PIC_SPUR	7

/* this one must have four least significant bits set to ones */
#define VECTOR_APIC_SPUR	(IVT_ITEMS-1)

#if (((VECTOR_APIC_SPUR + 1)%16) || VECTOR_APIC_SPUR >= IVT_ITEMS)
#error Wrong definition of VECTOR_APIC_SPUR
#endif

#define VECTOR_PIC_SPUR		(IVT_IRQBASE+IRQ_PIC_SPUR)
#define VECTOR_CLK		(IVT_IRQBASE+IRQ_CLK)
#define VECTOR_KBD		(IVT_IRQBASE+IRQ_KBD)

#define VECTOR_SYSCALL			(IVT_FREEBASE+0)
#define VECTOR_TLB_SHOOTDOWN_IPI	(IVT_FREEBASE+1)
#define VECTOR_WAKEUP_IPI		(IVT_FREEBASE+2)

typedef void (* iroutine)(__u8 n, __u32 stack[]);

extern iroutine ivt[IVT_ITEMS];

extern void (* disable_irqs_function)(__u16 irqmask);
extern void (* enable_irqs_function)(__u16 irqmask);
extern void (* eoi_function)(void);

extern iroutine trap_register(__u8 n, iroutine f);

extern void trap_dispatcher(__u8 n, __u32 stack[]);

extern void null_interrupt(__u8 n, __u32 stack[]);
extern void gp_fault(__u8 n, __u32 stack[]);
extern void nm_fault(__u8 n, __u32 stack[]);
extern void page_fault(__u8 n, __u32 stack[]);
extern void syscall(__u8 n, __u32 stack[]);
extern void tlb_shootdown_ipi(__u8 n, __u32 stack[]);
extern void wakeup_ipi(__u8 n, __u32 stack[]);

extern void trap_virtual_enable_irqs(__u16 irqmask);
extern void trap_virtual_disable_irqs(__u16 irqmask);
extern void trap_virtual_eoi(void);

#endif
