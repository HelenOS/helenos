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

#ifndef __PM_H__
#define __PM_H__

#define IDT_ITEMS 64
#define GDT_ITEMS 6

#define NULL_DES	0
#define KTEXT_DES	1
#define	KDATA_DES	2
#define UTEXT_DES	3
#define UDATA_DES	4
#define TSS_DES		5

#define selector(des)	((des)<<3)

#define PL_KERNEL	0
#define PL_USER		3

#define AR_PRESENT	(1<<7)
#define AR_DATA		(2<<3)
#define AR_CODE		(3<<3)
#define AR_WRITABLE	(1<<1)
#define AR_INTERRUPT	(0xe)
#define AR_TSS		(0x9)

#define DPL_KERNEL	(PL_KERNEL<<5)
#define DPL_USER	(PL_USER<<5)

#define IO_MAP_BASE	(104)

#ifndef __ASM__

#include <arch/types.h>
#include <typedefs.h>
#include <arch/context.h>

struct ptr_16_32 {
	__u16 limit;
	__u32 base;
} __attribute__ ((packed));

struct descriptor {
	unsigned limit_0_15: 16;
	unsigned base_0_15: 16;
	unsigned base_16_23: 8;
	unsigned access: 8;
	unsigned limit_16_19: 4;
	unsigned available: 1;
	unsigned unused: 1;
	unsigned special: 1;
	unsigned granularity : 1;
	unsigned base_24_31: 8;
} __attribute__ ((packed));

struct idescriptor {
	unsigned offset_0_15: 16;
	unsigned selector: 16;
	unsigned unused: 8;
	unsigned access: 8;
	unsigned offset_16_31: 16;
} __attribute__ ((packed));


struct tss {
	__u16 link;
	unsigned : 16;
	__u32 esp0;
	__u16 ss0;
	unsigned : 16;
	__u32 esp1;
	__u16 ss1;
	unsigned : 16;
	__u32 esp2;
	__u16 ss2;
	unsigned : 16;
	__u32 cr3;
	__u32 eip;
	__u32 eflags;
	__u32 eax;
	__u32 ecx;
	__u32 edx;
	__u32 ebx;
	__u32 esp;
	__u32 ebp;
	__u32 esi;
	__u32 edi;
	__u16 es;
	unsigned : 16;
	__u16 cs;
	unsigned : 16;
	__u16 ss;
	unsigned : 16;
	__u16 ds;
	unsigned : 16;
	__u16 fs;
	unsigned : 16;
	__u16 gs;
	unsigned : 16;
	__u16 ldtr;
	unsigned : 16;
	unsigned : 16;
	__u16 io_map_base;
} __attribute__ ((packed));

extern struct ptr_16_32 gdtr;
extern struct ptr_16_32 real_bootstrap_gdtr;
extern struct ptr_16_32 protected_bootstrap_gdtr;
extern struct tss *tss_p;

extern struct descriptor gdt[];

extern void pm_init(void);

extern void gdt_setbase(struct descriptor *d, __address base);
extern void gdt_setlimit(struct descriptor *d, __u32 limit);

extern void idt_init(void);
extern void idt_setoffset(struct idescriptor *d, __address offset);

extern void tss_initialize(struct tss *t);

#endif /* __ASM__ */

#endif
