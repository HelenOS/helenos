/*
 * Copyright (C) 2001-2004 Jakub Jermar
 * Copyright (C) 2005-2006 Ondrej Palkovsky
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
#include <arch/mm/page.h>
#include <arch/types.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <interrupt.h>

#include <config.h>

#include <memstr.h>
#include <mm/slab.h>
#include <debug.h>

/*
 * There is no segmentation in long mode so we set up flat mode. In this
 * mode, we use, for each privilege level, two segments spanning the
 * whole memory. One is for code and one is for data.
 */

struct descriptor gdt[GDT_ITEMS] = {
	/* NULL descriptor */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	/* KTEXT descriptor */
	{ .limit_0_15  = 0xffff, 
	  .base_0_15   = 0, 
	  .base_16_23  = 0, 
	  .access      = AR_PRESENT | AR_CODE | DPL_KERNEL | AR_READABLE , 
	  .limit_16_19 = 0xf, 
	  .available   = 0, 
	  .longmode    = 1, 
	  .special     = 0,
	  .granularity = 1, 
	  .base_24_31  = 0 },
	/* KDATA descriptor */
	{ .limit_0_15  = 0xffff, 
	  .base_0_15   = 0, 
	  .base_16_23  = 0, 
	  .access      = AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_KERNEL, 
	  .limit_16_19 = 0xf, 
	  .available   = 0, 
	  .longmode    = 0, 
	  .special     = 0, 
	  .granularity = 1, 
	  .base_24_31  = 0 },
	/* UDATA descriptor */
	{ .limit_0_15  = 0xffff, 
	  .base_0_15   = 0, 
	  .base_16_23  = 0, 
	  .access      = AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_USER, 
	  .limit_16_19 = 0xf, 
	  .available   = 0, 
	  .longmode    = 0, 
	  .special     = 1, 
	  .granularity = 1, 
	  .base_24_31  = 0 },
	/* UTEXT descriptor */
	{ .limit_0_15  = 0xffff, 
	  .base_0_15   = 0, 
	  .base_16_23  = 0, 
	  .access      = AR_PRESENT | AR_CODE | DPL_USER, 
	  .limit_16_19 = 0xf, 
	  .available   = 0, 
	  .longmode    = 1, 
	  .special     = 0, 
	  .granularity = 1, 
	  .base_24_31  = 0 },
	/* KTEXT 32-bit protected, for protected mode before long mode */
	{ .limit_0_15  = 0xffff, 
	  .base_0_15   = 0, 
	  .base_16_23  = 0, 
	  .access      = AR_PRESENT | AR_CODE | DPL_KERNEL | AR_READABLE, 
	  .limit_16_19 = 0xf, 
	  .available   = 0, 
	  .longmode    = 0, 
	  .special     = 1,
	  .granularity = 1, 
	  .base_24_31  = 0 },
	/* TSS descriptor - set up will be completed later,
	 * on AMD64 it is 64-bit - 2 items in table */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

struct idescriptor idt[IDT_ITEMS];

struct ptr_16_64 gdtr = {.limit = sizeof(gdt), .base= (__u64) gdt };
struct ptr_16_64 idtr = {.limit = sizeof(idt), .base= (__u64) idt };

static struct tss tss;
struct tss *tss_p = NULL;

void gdt_tss_setbase(struct descriptor *d, __address base)
{
	struct tss_descriptor *td = (struct tss_descriptor *) d;

	td->base_0_15 = base & 0xffff;
	td->base_16_23 = ((base) >> 16) & 0xff;
	td->base_24_31 = ((base) >> 24) & 0xff;
	td->base_32_63 = ((base) >> 32);
}

void gdt_tss_setlimit(struct descriptor *d, __u32 limit)
{
	struct tss_descriptor *td = (struct tss_descriptor *) d;

	td->limit_0_15 = limit & 0xffff;
	td->limit_16_19 = (limit >> 16) & 0xf;
}

void idt_setoffset(struct idescriptor *d, __address offset)
{
	/*
	 * Offset is a linear address.
	 */
	d->offset_0_15 = offset & 0xffff;
	d->offset_16_31 = offset >> 16 & 0xffff;
	d->offset_32_63 = offset >> 32;
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
		d->selector = gdtselector(KTEXT_DES);

		d->present = 1;
		d->type = AR_INTERRUPT;	/* masking interrupt */

		idt_setoffset(d, ((__address) interrupt_handlers) + i*interrupt_handler_size);
		exc_register(i, "undef", (iroutine)null_interrupt);
	}

	exc_register( 7, "nm_fault", nm_fault);
	exc_register(12, "ss_fault", ss_fault);
	exc_register(13, "gp_fault", gp_fault);
	exc_register(14, "ident_mapper", ident_page_fault);
}

/** Initialize segmentation - code/data/idt tables
 *
 */
void pm_init(void)
{
	struct descriptor *gdt_p = (struct descriptor *) gdtr.base;
	struct tss_descriptor *tss_desc;

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
		tss_p = (struct tss *) malloc(sizeof(struct tss),FRAME_ATOMIC);
		if (!tss_p)
			panic("could not allocate TSS\n");
	}

	tss_initialize(tss_p);

	tss_desc = (struct tss_descriptor *) (&gdt_p[TSS_DES]);
	tss_desc->present = 1;
	tss_desc->type = AR_TSS;
	tss_desc->dpl = PL_KERNEL;
	
	gdt_tss_setbase(&gdt_p[TSS_DES], (__address) tss_p);
	gdt_tss_setlimit(&gdt_p[TSS_DES], sizeof(struct tss) - 1);

	gdtr_load(&gdtr);
	idtr_load(&idtr);
	/*
	 * As of this moment, the current CPU has its own GDT pointing
	 * to its own TSS. We just need to load the TR register.
	 */
	tr_load(gdtselector(TSS_DES));
}
