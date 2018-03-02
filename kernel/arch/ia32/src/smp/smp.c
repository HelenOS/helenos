/*
 * Copyright (c) 2008 Jakub Jermar
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

/** @addtogroup ia32
 * @{
 */
/** @file
 */

#include <smp/smp.h>
#include <arch/smp/smp.h>
#include <arch/smp/mps.h>
#include <arch/smp/ap.h>
#include <arch/boot/boot.h>
#include <assert.h>
#include <errno.h>
#include <genarch/acpi/acpi.h>
#include <genarch/acpi/madt.h>
#include <config.h>
#include <synch/waitq.h>
#include <arch/pm.h>
#include <halt.h>
#include <panic.h>
#include <arch/asm.h>
#include <mm/page.h>
#include <mm/frame.h>
#include <mm/km.h>
#include <mm/slab.h>
#include <mm/as.h>
#include <log.h>
#include <mem.h>
#include <arch/drivers/i8259.h>
#include <cpu.h>

#ifdef CONFIG_SMP

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
		l_apic = (uint32_t *) km_map((uintptr_t) l_apic, PAGE_SIZE,
		    PAGE_WRITE | PAGE_NOT_CACHEABLE);
		io_apic = (uint32_t *) km_map((uintptr_t) io_apic, PAGE_SIZE,
		    PAGE_WRITE | PAGE_NOT_CACHEABLE);
	}
}

static void cpu_arch_id_init(void)
{
	assert(ops != NULL);
	assert(cpus != NULL);

	for (unsigned int i = 0; i < config.cpu_count; ++i) {
		cpus[i].arch.id = ops->cpu_apic_id(i);
	}
}

/*
 * Kernel thread for bringing up application processors. It becomes clear
 * that we need an arrangement like this (AP's being initialized by a kernel
 * thread), for a thread has its dedicated stack. (The stack used during the
 * BSP initialization (prior the very first call to scheduler()) will be used
 * as an initialization stack for each AP.)
 */
void kmp(void *arg __attribute__((unused)))
{
	unsigned int i;

	assert(ops != NULL);

	/*
	 * SMP initialized, cpus array allocated. Assign each CPU its
	 * physical APIC ID.
	 */
	cpu_arch_id_init();

	/*
	 * We need to access data in frame 0.
	 * We boldly make use of kernel address space mapping.
	 */

	/*
	 * Set the warm-reset vector to the real-mode address of 4K-aligned ap_boot()
	 */
	*((uint16_t *) (PA2KA(0x467 + 0))) =
	    (uint16_t) (((uintptr_t) ap_boot) >> 4);  /* segment */
	*((uint16_t *) (PA2KA(0x467 + 2))) = 0;       /* offset */

	/*
	 * Save 0xa to address 0xf of the CMOS RAM.
	 * BIOS will not do the POST after the INIT signal.
	 */
	pio_write_8((ioport8_t *) 0x70, 0xf);
	pio_write_8((ioport8_t *) 0x71, 0xa);

	pic_disable_irqs(0xffff);
	apic_init();

	for (i = 0; i < config.cpu_count; i++) {
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

		if (ops->cpu_apic_id(i) == bsp_l_apic) {
			log(LF_ARCH, LVL_ERROR, "kmp: bad processor entry #%u, "
			    "will not send IPI to myself", i);
			continue;
		}

		/*
		 * Prepare new GDT for CPU in question.
		 */

		/* XXX Flag FRAME_LOW_4_GiB was removed temporarily,
		 * it needs to be replaced by a generic fuctionality of
		 * the memory subsystem
		 */
		descriptor_t *gdt_new =
		    (descriptor_t *) malloc(GDT_ITEMS * sizeof(descriptor_t),
		    FRAME_ATOMIC);
		if (!gdt_new)
			panic("Cannot allocate memory for GDT.");

		memcpy(gdt_new, gdt, GDT_ITEMS * sizeof(descriptor_t));
		memsetb(&gdt_new[TSS_DES], sizeof(descriptor_t), 0);
		protected_ap_gdtr.limit = GDT_ITEMS * sizeof(descriptor_t);
		protected_ap_gdtr.base = KA2PA((uintptr_t) gdt_new);
		gdtr.base = (uintptr_t) gdt_new;

		if (l_apic_send_init_ipi(ops->cpu_apic_id(i))) {
			/*
			 * There may be just one AP being initialized at
			 * the time. After it comes completely up, it is
			 * supposed to wake us up.
			 */
			if (waitq_sleep_timeout(&ap_completion_wq, 1000000,
			    SYNCH_FLAGS_NONE, NULL) == ETIMEOUT) {
				log(LF_ARCH, LVL_NOTE, "%s: waiting for cpu%u "
				    "(APIC ID = %d) timed out", __FUNCTION__,
				    i, ops->cpu_apic_id(i));
			}
		} else
			log(LF_ARCH, LVL_ERROR, "INIT IPI for l_apic%d failed",
			    ops->cpu_apic_id(i));
	}
}

int smp_irq_to_pin(unsigned int irq)
{
	assert(ops != NULL);
	return ops->irq_to_pin(irq);
}

#endif /* CONFIG_SMP */

/** @}
 */
