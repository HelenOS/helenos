/*
 * Copyright (C) 2005 Jakub Jermar
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

#include <smp/smp.h>
#include <arch/smp/smp.h>
#include <arch/smp/mps.h>
#include <arch/smp/ap.h>
#include <arch/acpi/acpi.h>
#include <arch/acpi/madt.h>
#include <config.h>
#include <synch/waitq.h>
#include <arch/pm.h>
#include <func.h>
#include <panic.h>
#include <debug.h>
#include <arch/asm.h>
#include <mm/frame.h>
#include <mm/page.h>
#include <mm/heap.h>
#include <print.h>
#include <memstr.h>
#include <arch/i8259.h>

#ifdef __SMP__

static struct smp_config_operations *ops = NULL;

void smp_init(void)
{
	if (acpi_madt) {
		acpi_madt_parse();
		ops = &madt_config_operations;
	}
	if (config.cpu_count == 1) {
		mps_init();
		ops = &mps_config_operations;
	}

	if (config.cpu_count > 1) {		
		map_page_to_frame((__address)l_apic, (__address)l_apic, 
				  PAGE_NOT_CACHEABLE, 0);
		map_page_to_frame((__address) io_apic,
				  (__address) io_apic,
				  PAGE_NOT_CACHEABLE, 0);
        }

        /* 
         * Must be initialized outside the kmp thread, since it is waited
         * on before the kmp thread is created.
         */
        waitq_initialize(&kmp_completion_wq);

}

/*
 * Kernel thread for bringing up application processors. It becomes clear
 * that we need an arrangement like this (AP's being initialized by a kernel
 * thread), for a thread has its dedicated stack. (The stack used during the
 * BSP initialization (prior the very first call to scheduler()) will be used
 * as an initialization stack for each AP.)
 */
void kmp(void *arg)
{
	__address src, dst;
	int i;
	
	ASSERT(ops != NULL);

	waitq_initialize(&ap_completion_wq);

	/*
	 * We need to access data in frame 0.
	 * We boldly make use of kernel address space mapping.
	 */

	/*
	 * Set the warm-reset vector to the real-mode address of 4K-aligned ap_boot()
	 */
	*((__u16 *) (PA2KA(0x467+0))) =  ((__address) ap_boot) >> 4;	/* segment */
	*((__u16 *) (PA2KA(0x467+2))) =  0;				/* offset */
	
	/*
	 * Save 0xa to address 0xf of the CMOS RAM.
	 * BIOS will not do the POST after the INIT signal.
	 */
	outb(0x70,0xf);
	outb(0x71,0xa);

	cpu_priority_high();

	pic_disable_irqs(0xffff);
	apic_init();

	for (i = 0; i < ops->cpu_count(); i++) {
		struct descriptor *gdt_new;
	
		/*
		 * Skip processors marked unusable.
		 */
		if (!ops->cpu_enabled(i))
			continue;

		/*
		 * The bootstrap processor is already up.
		 */
		if (ops->cpu_bootstrap(i))
			continue;

		if (ops->cpu_apic_id(i) == l_apic_id()) {
			printf("kmp: bad processor entry #%d, will not send IPI to myself\n", i);
			continue;
		}
		
		/*
		 * Prepare new GDT for CPU in question.
		 */
		if (!(gdt_new = (struct descriptor *) malloc(GDT_ITEMS*sizeof(struct descriptor))))
			panic("couldn't allocate memory for GDT\n");

		memcpy(gdt_new, gdt, GDT_ITEMS*sizeof(struct descriptor));
		memsetb((__address)(&gdt_new[TSS_DES]), sizeof(struct descriptor), 0);
		real_bootstrap_gdtr.base = KA2PA((__address) gdt_new);
		gdtr.base = (__address) gdt_new;

		if (l_apic_send_init_ipi(ops->cpu_apic_id(i))) {
			/*
		         * There may be just one AP being initialized at
			 * the time. After it comes completely up, it is
			 * supposed to wake us up.
		         */
			waitq_sleep(&ap_completion_wq);
		}
		else {
			printf("INIT IPI for l_apic%d failed\n", ops->cpu_apic_id(i));
		}
	}

	/*
	 * Wakeup the kinit thread so that
	 * system initialization can go on.
	 */
	waitq_wakeup(&kmp_completion_wq, WAKEUP_FIRST);
}

#endif /* __SMP__ */
