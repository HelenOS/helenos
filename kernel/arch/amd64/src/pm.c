/*
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2005-2006 Ondrej Palkovsky
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

/** @addtogroup amd64
 * @{
 */
/** @file
 */

#include <arch.h>
#include <arch/pm.h>
#include <arch/asm.h>
#include <mm/as.h>
#include <mm/frame.h>
#include <mem.h>
#include <mm/slab.h>

/*
 * There is no segmentation in long mode so we set up flat mode. In this
 * mode, we use, for each privilege level, two segments spanning the
 * whole memory. One is for code and one is for data.
 */

descriptor_t gdt[GDT_ITEMS] = {
	[NULL_DES] = {
		0
	},
	[KTEXT_DES] = {
		.limit_0_15 = 0xffffU,
		.limit_16_19 = 0xfU,
	  	.access = AR_PRESENT | AR_CODE | DPL_KERNEL | AR_READABLE,
		.longmode = 1,
		.granularity = 1
	},
	[KDATA_DES] = {
		.limit_0_15 = 0xffffU,
		.limit_16_19 = 0xfU,
		.access = AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_KERNEL,
		.granularity = 1
	},
	[UDATA_DES] = {
		.limit_0_15 = 0xffffU,
		.limit_16_19 = 0xfU,
		.access = AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_USER,
		.special = 1,
		.granularity = 1
	},
	[UTEXT_DES] = {
		.limit_0_15 = 0xffffU,
		.limit_16_19 = 0xfU,
		.access = AR_PRESENT | AR_CODE | DPL_USER,
		.longmode = 1,
		.granularity = 1
	},
	[KTEXT32_DES] = {
		.limit_0_15 = 0xffffU,
		.limit_16_19 = 0xfU,
		.access = AR_PRESENT | AR_CODE | DPL_KERNEL | AR_READABLE,
		.special = 1,
		.granularity = 1
	},
	/*
	 * TSS descriptor - set up will be completed later,
	 * on AMD64 it is 64-bit - 2 items in the table
	 */
	[TSS_DES] = {
		0
	},
	[TSS_DES + 1] = {
		0
	},
	/* VESA Init descriptor */
#ifdef CONFIG_FB
	[VESA_INIT_CODE_DES] = {
		.limit_0_15 = 0xffff,
		.limit_16_19 = 0xf,
		.base_16_23 = VESA_INIT_SEGMENT >> 12,
		.access = AR_PRESENT | AR_CODE | AR_READABLE | DPL_KERNEL
	},
	[VESA_INIT_DATA_DES] = {
		.limit_0_15 = 0xffff,
		.limit_16_19 = 0xf,
		.base_16_23 = VESA_INIT_SEGMENT >> 12,
		.access = AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_KERNEL
	}
#endif
};

idescriptor_t idt[IDT_ITEMS];

ptr_16_64_t gdtr = {
	.limit = sizeof(gdt),
	.base = (uint64_t) gdt
};
ptr_16_64_t idtr = {
	.limit = sizeof(idt),
	.base = (uint64_t) idt
};

static tss_t tss;
tss_t *tss_p = NULL;

void gdt_tss_setbase(descriptor_t *d, uintptr_t base)
{
	tss_descriptor_t *td = (tss_descriptor_t *) d;

	td->base_0_15 = base & 0xffffU;
	td->base_16_23 = ((base) >> 16) & 0xffU;
	td->base_24_31 = ((base) >> 24) & 0xffU;
	td->base_32_63 = ((base) >> 32);
}

void gdt_tss_setlimit(descriptor_t *d, uint32_t limit)
{
	tss_descriptor_t *td = (tss_descriptor_t *) d;

	td->limit_0_15 = limit & 0xffffU;
	td->limit_16_19 = (limit >> 16) & 0x0fU;
}

void idt_setoffset(idescriptor_t *d, uintptr_t offset)
{
	/*
	 * Offset is a linear address.
	 */
	d->offset_0_15 = offset & 0xffffU;
	d->offset_16_31 = (offset >> 16) & 0xffffU;
	d->offset_32_63 = offset >> 32;
}

void tss_initialize(tss_t *t)
{
	memsetb(t, sizeof(tss_t), 0);
}

/*
 * This function takes care of proper setup of IDT and IDTR.
 */
void idt_init(void)
{
	idescriptor_t *d;
	unsigned int i;

	for (i = 0; i < IDT_ITEMS; i++) {
		d = &idt[i];

		d->unused = 0;
		d->selector = GDT_SELECTOR(KTEXT_DES);

		d->present = 1;
		d->type = AR_INTERRUPT;  /* masking interrupt */
	}

	d = &idt[0];
	idt_setoffset(d++, (uintptr_t) &int_0);
	idt_setoffset(d++, (uintptr_t) &int_1);
	idt_setoffset(d++, (uintptr_t) &int_2);
	idt_setoffset(d++, (uintptr_t) &int_3);
	idt_setoffset(d++, (uintptr_t) &int_4);
	idt_setoffset(d++, (uintptr_t) &int_5);
	idt_setoffset(d++, (uintptr_t) &int_6);
	idt_setoffset(d++, (uintptr_t) &int_7);
	idt_setoffset(d++, (uintptr_t) &int_8);
	idt_setoffset(d++, (uintptr_t) &int_9);
	idt_setoffset(d++, (uintptr_t) &int_10);
	idt_setoffset(d++, (uintptr_t) &int_11);
	idt_setoffset(d++, (uintptr_t) &int_12);
	idt_setoffset(d++, (uintptr_t) &int_13);
	idt_setoffset(d++, (uintptr_t) &int_14);
	idt_setoffset(d++, (uintptr_t) &int_15);
	idt_setoffset(d++, (uintptr_t) &int_16);
	idt_setoffset(d++, (uintptr_t) &int_17);
	idt_setoffset(d++, (uintptr_t) &int_18);
	idt_setoffset(d++, (uintptr_t) &int_19);
	idt_setoffset(d++, (uintptr_t) &int_20);
	idt_setoffset(d++, (uintptr_t) &int_21);
	idt_setoffset(d++, (uintptr_t) &int_22);
	idt_setoffset(d++, (uintptr_t) &int_23);
	idt_setoffset(d++, (uintptr_t) &int_24);
	idt_setoffset(d++, (uintptr_t) &int_25);
	idt_setoffset(d++, (uintptr_t) &int_26);
	idt_setoffset(d++, (uintptr_t) &int_27);
	idt_setoffset(d++, (uintptr_t) &int_28);
	idt_setoffset(d++, (uintptr_t) &int_29);
	idt_setoffset(d++, (uintptr_t) &int_30);
	idt_setoffset(d++, (uintptr_t) &int_31);
	idt_setoffset(d++, (uintptr_t) &int_32);
	idt_setoffset(d++, (uintptr_t) &int_33);
	idt_setoffset(d++, (uintptr_t) &int_34);
	idt_setoffset(d++, (uintptr_t) &int_35);
	idt_setoffset(d++, (uintptr_t) &int_36);
	idt_setoffset(d++, (uintptr_t) &int_37);
	idt_setoffset(d++, (uintptr_t) &int_38);
	idt_setoffset(d++, (uintptr_t) &int_39);
	idt_setoffset(d++, (uintptr_t) &int_40);
	idt_setoffset(d++, (uintptr_t) &int_41);
	idt_setoffset(d++, (uintptr_t) &int_42);
	idt_setoffset(d++, (uintptr_t) &int_43);
	idt_setoffset(d++, (uintptr_t) &int_44);
	idt_setoffset(d++, (uintptr_t) &int_45);
	idt_setoffset(d++, (uintptr_t) &int_46);
	idt_setoffset(d++, (uintptr_t) &int_47);
	idt_setoffset(d++, (uintptr_t) &int_48);
	idt_setoffset(d++, (uintptr_t) &int_49);
	idt_setoffset(d++, (uintptr_t) &int_50);
	idt_setoffset(d++, (uintptr_t) &int_51);
	idt_setoffset(d++, (uintptr_t) &int_52);
	idt_setoffset(d++, (uintptr_t) &int_53);
	idt_setoffset(d++, (uintptr_t) &int_54);
	idt_setoffset(d++, (uintptr_t) &int_55);
	idt_setoffset(d++, (uintptr_t) &int_56);
	idt_setoffset(d++, (uintptr_t) &int_57);
	idt_setoffset(d++, (uintptr_t) &int_58);
	idt_setoffset(d++, (uintptr_t) &int_59);
	idt_setoffset(d++, (uintptr_t) &int_60);
	idt_setoffset(d++, (uintptr_t) &int_61);
	idt_setoffset(d++, (uintptr_t) &int_62);
	idt_setoffset(d++, (uintptr_t) &int_63);
}

/** Initialize segmentation - code/data/idt tables
 *
 */
void pm_init(void)
{
	descriptor_t *gdt_p = (descriptor_t *) gdtr.base;
	tss_descriptor_t *tss_desc;

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
	} else {
		/* We are going to use malloc, which may return
		 * non boot-mapped pointer, initialize the CR3 register
		 * ahead of page_init */
		write_cr3((uintptr_t) AS_KERNEL->genarch.page_table);

		tss_p = (tss_t *) malloc(sizeof(tss_t), FRAME_ATOMIC);
		if (!tss_p)
			panic("Cannot allocate TSS.");
	}

	tss_initialize(tss_p);

	tss_desc = (tss_descriptor_t *) (&gdt_p[TSS_DES]);
	tss_desc->present = 1;
	tss_desc->type = AR_TSS;
	tss_desc->dpl = PL_KERNEL;

	gdt_tss_setbase(&gdt_p[TSS_DES], (uintptr_t) tss_p);
	gdt_tss_setlimit(&gdt_p[TSS_DES], TSS_BASIC_SIZE - 1);

	gdtr_load(&gdtr);
	idtr_load(&idtr);
	/*
	 * As of this moment, the current CPU has its own GDT pointing
	 * to its own TSS. We just need to load the TR register.
	 */
	tr_load(GDT_SELECTOR(TSS_DES));
}

/** @}
 */
