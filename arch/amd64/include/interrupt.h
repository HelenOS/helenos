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

 /** @addtogroup amd64interrupt amd64
 * @ingroup interrupt
 * @{
 */
/** @file
 */

#ifndef __ia32_INTERRUPT_H__
#define __ia32_INTERRUPT_H__

#include <arch/types.h>
#include <arch/pm.h>

#define IVT_ITEMS		IDT_ITEMS

#define EXC_COUNT	32
#define IRQ_COUNT	16

#define IVT_EXCBASE		0
#define IVT_IRQBASE		(IVT_EXCBASE+EXC_COUNT)
#define IVT_FREEBASE		(IVT_IRQBASE+IRQ_COUNT)

#define IRQ_CLK		0
#define IRQ_KBD		1
#define IRQ_PIC1	2
#define IRQ_PIC_SPUR	7

/* this one must have four least significant bits set to ones */
#define VECTOR_APIC_SPUR	(IVT_ITEMS-1)

#if (((VECTOR_APIC_SPUR + 1)%16) || VECTOR_APIC_SPUR >= IVT_ITEMS)
#error Wrong definition of VECTOR_APIC_SPUR
#endif

#define VECTOR_DEBUG            1
#define VECTOR_PIC_SPUR		(IVT_IRQBASE+IRQ_PIC_SPUR)
#define VECTOR_CLK		(IVT_IRQBASE+IRQ_CLK)
#define VECTOR_KBD		(IVT_IRQBASE+IRQ_KBD)

#define VECTOR_TLB_SHOOTDOWN_IPI	(IVT_FREEBASE+0)
#define VECTOR_WAKEUP_IPI		(IVT_FREEBASE+1)
#define VECTOR_DEBUG_IPI                (IVT_FREEBASE+2)

/** This is passed to interrupt handlers */
struct istate {
	__u64 rax;
	__u64 rbx;
	__u64 rcx;
	__u64 rdx;
	__u64 rsi;
	__u64 rdi;
	__u64 r8;
	__u64 r9;
	__u64 r10;
	__u64 r11;
	__u64 r12;
	__u64 r13;
	__u64 r14;
	__u64 r15;
	__u64 rbp;
	__u64 error_word;
	__u64 rip;
	__u64 cs;
	__u64 rflags;
	__u64 stack[]; /* Additional data on stack */
};

/** Return true if exception happened while in userspace */
static inline int istate_from_uspace(istate_t *istate)
{
	return !(istate->rip & 0x8000000000000000);
}

static inline void istate_set_retaddr(istate_t *istate, __address retaddr)
{
	istate->rip = retaddr;
}
static inline __native istate_get_pc(istate_t *istate)
{
	return istate->rip;
}

extern void (* disable_irqs_function)(__u16 irqmask);
extern void (* enable_irqs_function)(__u16 irqmask);
extern void (* eoi_function)(void);

extern void print_info_errcode(int n, istate_t *istate);
extern void null_interrupt(int n, istate_t *istate);
extern void gp_fault(int n, istate_t *istate);
extern void nm_fault(int n, istate_t *istate);
extern void ss_fault(int n, istate_t *istate);
extern void page_fault(int n, istate_t *istate);
extern void syscall(int n, istate_t *istate);
extern void tlb_shootdown_ipi(int n, istate_t *istate);

extern void trap_virtual_enable_irqs(__u16 irqmask);
extern void trap_virtual_disable_irqs(__u16 irqmask);
extern void trap_virtual_eoi(void);
/* AMD64 - specific page handler */
extern void ident_page_fault(int n, istate_t *istate);

#endif

 /** @}
 */

