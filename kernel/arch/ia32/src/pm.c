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

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#include <arch/pm.h>
#include <config.h>
#include <typedefs.h>
#include <arch/interrupt.h>
#include <arch/asm.h>
#include <arch/context.h>
#include <panic.h>
#include <arch/mm/page.h>
#include <mm/km.h>
#include <mm/frame.h>
#include <stdlib.h>
#include <memw.h>
#include <arch/boot/boot.h>
#include <interrupt.h>
#include <arch/cpu.h>

/*
 * Early ia32 configuration functions and data structures.
 */

/*
 * We don't have much use for segmentation so we set up flat mode.
 * In this mode, we use, for each privilege level, two segments spanning the
 * whole memory. One is for code and one is for data.
 *
 * One special segment apart of that is for the GS register which holds
 * a pointer to the VREG page in its base.
 */
descriptor_t gdt[GDT_ITEMS] = {
	[NULL_DES] = {
		0
	},
	[KTEXT_DES] = {
		.limit_0_15 = 0xffff,
		.limit_16_19 = 0xf,
		.access = AR_PRESENT | AR_CODE | DPL_KERNEL,
		.special = 1,
		.granularity = 1
	},
	[KDATA_DES] = {
		.limit_0_15 = 0xffff,
		.limit_16_19 = 0xf,
		.access = AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_KERNEL,
		.special = 1,
		.granularity = 1
	},
	[UTEXT_DES] = {
		.limit_0_15 = 0xffff,
		.limit_16_19 = 0xf,
		.access = AR_PRESENT | AR_CODE | DPL_USER,
		.special = 1,
		.granularity = 1
	},
	[UDATA_DES] = {
		.limit_0_15 = 0xffff,
		.limit_16_19 = 0xf,
		.access = AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_USER,
		.special = 1,
		.granularity = 1
	},
	[TSS_DES] = {		/* set up will be completed later */
		0,
	},
	[VREG_DES] = {		/* will be reinitialized later */
		.limit_0_15 = 0xffff,
		.limit_16_19 = 0xf,
		.access = AR_PRESENT | AR_DATA | AR_WRITABLE | DPL_USER,
		.special = 1,
		.granularity = 1
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

static idescriptor_t idt[IDT_ITEMS];

static tss_t tss0;

tss_t *tss_p = NULL;

/* gdtr is changed by kmp before next CPU is initialized */
ptr_16_32_t gdtr = {
	.limit = sizeof(gdt),
	.base = (uintptr_t) gdt
};

void gdt_setbase(descriptor_t *d, uintptr_t base)
{
	d->base_0_15 = base & 0xffff;
	d->base_16_23 = (base >> 16) & 0xff;
	d->base_24_31 = (base >> 24) & 0xff;
}

void gdt_setlimit(descriptor_t *d, uint32_t limit)
{
	d->limit_0_15 = limit & 0xffff;
	d->limit_16_19 = (limit >> 16) & 0xf;
}

void idt_setoffset(idescriptor_t *d, uintptr_t offset)
{
	/*
	 * Offset is a linear address.
	 */
	d->offset_0_15 = offset & 0xffff;
	d->offset_16_31 = offset >> 16;
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

		if (i == VECTOR_SYSCALL) {
			/*
			 * The syscall trap gate must be callable from
			 * userland. Interrupts will remain enabled.
			 */
			d->access = AR_PRESENT | AR_TRAP | DPL_USER;
		} else {
			/*
			 * Other interrupts use interrupt gates which
			 * disable interrupts.
			 */
			d->access = AR_PRESENT | AR_INTERRUPT;
		}
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

	idt_setoffset(&idt[VECTOR_SYSCALL], (uintptr_t) &int_syscall);
}

void pm_init(void)
{
	descriptor_t *gdt_p = (descriptor_t *) gdtr.base;
	ptr_16_32_t idtr;

	/*
	 * Update addresses in GDT and IDT to their virtual counterparts.
	 */
	idtr.limit = sizeof(idt);
	idtr.base = (uintptr_t) idt;
	gdtr_load(&gdtr);
	idtr_load(&idtr);

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
		tss_p = &tss0;
	} else {
		tss_p = (tss_t *) malloc(sizeof(tss_t));
		if (!tss_p)
			panic("Cannot allocate TSS.");
	}

	tss_initialize(tss_p);

	gdt_p[TSS_DES].access = AR_PRESENT | AR_TSS | DPL_KERNEL;
	gdt_p[TSS_DES].special = 1;
	gdt_p[TSS_DES].granularity = 0;

	gdt_setbase(&gdt_p[TSS_DES], (uintptr_t) tss_p);
	gdt_setlimit(&gdt_p[TSS_DES], TSS_BASIC_SIZE - 1);

	/*
	 * As of this moment, the current CPU has its own GDT pointing
	 * to its own TSS. We just need to load the TR register.
	 */
	tr_load(GDT_SELECTOR(TSS_DES));

	/* Disable I/O on nonprivileged levels and clear NT flag. */
	write_eflags(read_eflags() & ~(EFLAGS_IOPL | EFLAGS_NT));

	/* Disable alignment check */
	write_cr0(read_cr0() & ~CR0_AM);
}

/** @}
 */
