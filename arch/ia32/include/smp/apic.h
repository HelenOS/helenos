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

#ifndef __APIC_H__
#define __APIC_H__

#include <arch/types.h>
#include <cpu.h>

#define FIXED		(0<<0)
#define LOPRI		(1<<0)

/* local APIC macros */
#define IPI_INIT 	0
#define IPI_STARTUP	0

#define DLVRMODE_FIXED	(0<<8)
#define DLVRMODE_INIT	(5<<8)
#define DLVRMODE_STUP	(6<<8)
#define DESTMODE_PHYS	(0<<11)
#define DESTMODE_LOGIC	(1<<11)
#define LEVEL_ASSERT	(1<<14)
#define LEVEL_DEASSERT	(0<<14)
#define TRGRMODE_LEVEL	(1<<15)
#define TRGRMODE_EDGE	(0<<15)
#define SHORTHAND_DEST	(0<<18)
#define SHORTHAND_INCL	(2<<18)
#define SHORTHAND_EXCL	(3<<18)

#define SEND_PENDING	(1<<12)

/* Interrupt Command Register */
#define ICRlo		(0x300/sizeof(__u32))
#define ICRhi		(0x310/sizeof(__u32))
#define ICRloClear	((1<<13)|(3<<16)|(0xfff<<20))
#define ICRhiClear	(0xffffff<<0)

/* End Of Interrupt */
#define EOI		(0x0b0/sizeof(__u32))

/* Error Status Register */
#define ESR		(0x280/sizeof(__u32))
#define ESRClear	((0xffffff<<8)|(1<<4))

/* Task Priority Register */
#define TPR		(0x080/sizeof(__u32))
#define TPRClear	0xffffff00

/* Spurious Vector Register */
#define SVR		(0x0f0/sizeof(__u32))
#define SVRClear	(~0x3f0)

/* Time Divide Configuratio Register */
#define TDCR		(0x3e0/sizeof(__u32))
#define TDCRClear	(~0xb)

/* Initial Count Register for Timer */
#define ICRT		(0x380/sizeof(__u32))

/* Current Count Register for Timer */
#define CCRT		(0x390/sizeof(__u32))

/* LVT */
#define LVT_Tm		(0x320/sizeof(__u32))
#define LVT_LINT0	(0x350/sizeof(__u32))
#define LVT_LINT1	(0x360/sizeof(__u32))
#define LVT_Err		(0x370/sizeof(__u32))
#define LVT_PCINT	(0x340/sizeof(__u32))

/* Local APIC ID Register */
#define L_APIC_ID	(0x020/sizeof(__u32))
#define L_APIC_IDClear	(~(0xf<<24))
#define L_APIC_IDShift	24
#define L_APIC_IDMask	0xf

/* Local APIC Version Register */
#define LAVR		(0x030/sizeof(__u32))
#define LAVR_Mask	0xff
#define is_local_apic(x)	(((x)&LAVR_Mask&0xf0)==0x1)
#define is_82489DX_apic(x)	((((x)&LAVR_Mask&0xf0)==0x0))
#define is_local_xapic(x)	(((x)&LAVR_Mask)==0x14)

/* IO APIC */
#define IOREGSEL	(0x00/sizeof(__u32))
#define IOWIN		(0x10/sizeof(__u32))

#define IOAPICID	0x00
#define IOAPICVER	0x01
#define IOAPICARB	0x02
#define IOREDTBL	0x10


extern volatile __u32 *l_apic;
extern volatile __u32 *io_apic;

extern __u32 apic_id_mask;

extern void apic_init(void);
extern void apic_spurious(__u8 n, __u32 stack[]);

extern void l_apic_init(void);
extern void l_apic_eoi(void);
extern int l_apic_broadcast_custom_ipi(__u8 vector);
extern int l_apic_send_init_ipi(__u8 apicid);
extern void l_apic_debug(void);
extern void l_apic_timer_interrupt(__u8 n, __u32 stack[]);
extern __u8 l_apic_id(void);

extern __u32 io_apic_read(__u8 address);
extern void io_apic_write(__u8 address , __u32 x);
extern void io_apic_change_ioredtbl(int signal, int dest, __u8 v, int flags);
extern void io_apic_disable_irqs(__u16 irqmask);
extern void io_apic_enable_irqs(__u16 irqmask);

#endif
