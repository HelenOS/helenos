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

#include <arch/pm.h>
#include <config.h>
#include <arch/types.h>
#include <typedefs.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <arch/context.h>
#include <panic.h>
#include <arch/mm/page.h>
#include <mm/heap.h>
#include <memstr.h>
#include <arch/boot/boot.h>

/*
 * Early ia32 configuration functions and data structures.
 */

/*
 * We have no use for segmentation so we set up flat mode. In this
 * mode, we use, for each privilege level, two segments spanning the
 * whole memory. One is for code and one is for data.
 */
struct descriptor gdt[GDT_ITEMS] = {
	/* NULL descriptor */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	/* KTEXT descriptor */
	{ 0xffff, 0, 0, AR_PRESENT | AR_CODE | DPL_KERNEL, 0xf, 0, 0, 1, 1, 0 },
	/* KDATA descriptor */
	{ 0xffff, 0, 0, AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_KERNEL, 0xf, 0, 0, 1, 1, 0 },
	/* UTEXT descriptor */
	{ 0xffff, 0, 0, AR_PRESENT | AR_CODE | DPL_USER, 0xf, 0, 0, 1, 1, 0 },
	/* UDATA descriptor */
	{ 0xffff, 0, 0, AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_USER, 0xf, 0, 0, 1, 1, 0 },
	/* TSS descriptor - set up will be completed later */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

static struct idescriptor idt[IDT_ITEMS];

static struct tss tss;

struct tss *tss_p = NULL;

/* gdtr is changed by kmp before next CPU is initialized */
struct ptr_16_32 bsp_bootstrap_gdtr __attribute__ ((section ("K_DATA_START"))) = { .limit = sizeof(gdt), .base = KA2PA((__address) gdt - BOOT_OFFSET) };
struct ptr_16_32 ap_bootstrap_gdtr __attribute__ ((section ("K_DATA_START"))) = { .limit = sizeof(gdt), .base = KA2PA((__address) gdt) };
struct ptr_16_32 gdtr = { .limit = sizeof(gdt), .base = KA2PA((__address) gdt) };
struct ptr_16_32 idtr __attribute__ ((section ("K_DATA_START"))) = { .limit = sizeof(idt), .base = KA2PA((__address) idt) };

void gdt_setbase(struct descriptor *d, __address base)
{
	d->base_0_15 = base & 0xffff;
	d->base_16_23 = ((base) >> 16) & 0xff;
	d->base_24_31 = ((base) >> 24) & 0xff;
}

void gdt_setlimit(struct descriptor *d, __u32 limit)
{
	d->limit_0_15 = limit & 0xffff;
	d->limit_16_19 = (limit >> 16) & 0xf;
}

void idt_setoffset(struct idescriptor *d, __address offset)
{
	/*
	 * Offset is a linear address.
	 */
	d->offset_0_15 = offset & 0xffff;
	d->offset_16_31 = offset >> 16;
}

void tss_initialize(struct tss *t)
{
	memsetb((__address) t, sizeof(struct tss), 0);
}

/*
 * This function takes care of proper setup of IDT and IDTR.
 */
void idt_init(void)
{
	struct idescriptor *d;
	int i;

	for (i = 0; i < IDT_ITEMS; i++) {
		d = &idt[i];

		d->unused = 0;
		d->selector = selector(KTEXT_DES);

		d->access = AR_PRESENT | AR_INTERRUPT;	/* masking interrupt */

		if (i == VECTOR_SYSCALL) {
			/*
			 * The syscall interrupt gate must be calleable from userland.
			 */
			d->access |= DPL_USER;
		}
		
		idt_setoffset(d, ((__address) interrupt_handlers) + i*interrupt_handler_size);
		trap_register(i, null_interrupt);
	}
	trap_register(13, gp_fault);
	trap_register( 7, nm_fault);
	trap_register(12, ss_fault);
}


/* Clean IOPL(12,13) and NT(14) flags in EFLAGS register */
static void clean_IOPL_NT_flags(void)
{
	asm
	(
		"pushfl;"
		"pop %%eax;"
		"and $0xffff8fff,%%eax;"
		"push %%eax;"
		"popfl;"
		:
		:
		:"%eax"
	);
}

/* Clean AM(18) flag in CR0 register */
static void clean_AM_flag(void)
{
	asm
	(
		"mov %%cr0,%%eax;"
		"and $0xFFFBFFFF,%%eax;"
		"mov %%eax,%%cr0;"
		:
		:
		:"%eax"
	);
}

void pm_init(void)
{
	struct descriptor *gdt_p = (struct descriptor *) PA2KA(gdtr.base);


	/*
	 * Update addresses in GDT and IDT to their virtual counterparts.
	 */
	if (config.cpu_active == 1)
		gdtr.base = (__address) gdt;
	idtr.base = (__address) idt;
	__asm__ volatile ("lgdt %0\n" : : "m" (gdtr));
	__asm__ volatile ("lidt %0\n" : : "m" (idtr));	
	
	/*
	 * Each CPU has its private GDT and TSS.
	 * All CPUs share one IDT.
	 */

	if (config.cpu_active == 1) {
		idt_init();
		/*
		 * NOTE: bootstrap CPU has statically allocated TSS, because
		 * the heap hasn't been initialized so far.
		 */
		tss_p = &tss;
	}
	else {
		tss_p = (struct tss *) malloc(sizeof(struct tss));
		if (!tss_p)
			panic("could not allocate TSS\n");
	}

	tss_initialize(tss_p);
	
	gdt_p[TSS_DES].access = AR_PRESENT | AR_TSS | DPL_KERNEL;
	gdt_p[TSS_DES].special = 1;
	gdt_p[TSS_DES].granularity = 1;
	
	gdt_setbase(&gdt_p[TSS_DES], (__address) tss_p);
	gdt_setlimit(&gdt_p[TSS_DES], sizeof(struct tss) - 1);

	/*
	 * As of this moment, the current CPU has its own GDT pointing
	 * to its own TSS. We just need to load the TR register.
	 */
	__asm__ volatile ("ltr %0" : : "r" ((__u16) selector(TSS_DES)));
	
	clean_IOPL_NT_flags();    /* Disable I/O on nonprivileged levels */
	clean_AM_flag();          /* Disable alignment check */
}
