/*
 * Copyright (c) 2026 Jakub Kodym
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

/** @addtogroup kernel_arm64_quartz64a
 * @{
 */
/** @file
 * @brief Quartz64 Model A platform driver.
 */

#include <arch/mach/quartz64a/quartz64a.h>
#include <arch/smp/aff.h>
#include <console/console.h>
#include <genarch/drivers/dwapb/dwapb.h>
#include <genarch/drivers/gicv3/gicv3.h>
#include <genarch/srln/srln.h>
#include <mm/km.h>
#include <sysinfo/sysinfo.h>

#define BIT(n)                      (1U << (n))

#define QUARTZ64A_VTIMER_IRQ         27     		/* Virtual timer interrupt */
#define QUARTZ64A_UART_IRQ           150         	/* Interrupt for UART2 */
#define QUARTZ64A_UART_ADDRESS       0xFE660000  	/* UART2 start address */

#define PLATFORM_CORE_COUNT 1

#define GIC600_BASE			(0xfd400000)
#define PLAT_GICD_BASE		(GIC600_BASE)
#define PLAT_GICR_BASE		(GIC600_BASE + 0x60000)

#define GICV3_DISTR_AREA_SIZE 0x10000
#define GICV3_REDIST_AREA_SIZE 0x80000

struct {
	gicv3_t gicv3;
	dwapb_instance_t *uart;
} quartz64a;

uintptr_t gicr_base_addrs[PLATFORM_CORE_COUNT];

affinity_t quartz64a_pe_affinities[] = {
	/* Value to be changed at runtime to the actual affinity. */
	{ .aff3 = 0, .aff2 = 0, .aff1 = 0, .aff0 = 0 },
};

/**
 * Convert affinity to PE index.
 *
 * @param affinity Affinity value.
 * @param pe_idx_out Pointer to store the PE index.
 * @return EOK on success, ENOENT if the affinity is not supported.
 */
static errno_t aff_to_idx(uint32_t affinity, uint32_t *pe_idx_out)
{
	for (unsigned int i = 0; i < PLATFORM_CORE_COUNT; i++) {
		if (affinity == quartz64a_pe_affinities[i].val) {
			*pe_idx_out = i;
			return EOK;
		}
	}

	return ENOENT;
}

/** Initialize the Quartz64A platform. */
static void quartz64a_init(void)
{
	quartz64a_pe_affinities[0] = pe_affinity();

	/* Initialize interrupt controller. */
	unsigned int flags = PAGE_NOT_CACHEABLE | PAGE_READ | PAGE_WRITE |
	    PAGE_KERNEL;
	size_t gicd_size = ALIGN_UP(GICV3_DISTR_AREA_SIZE, PAGE_SIZE);
	size_t gicr_size = ALIGN_UP(GICV3_REDIST_AREA_SIZE, PAGE_SIZE);

	uintptr_t gicd_base =
	    km_map(PLAT_GICD_BASE, gicd_size, KM_NATURAL_ALIGNMENT, flags);
	uintptr_t gicr_base =
	    km_map(PLAT_GICR_BASE, gicr_size, KM_NATURAL_ALIGNMENT, flags);

	gicv3_init(&quartz64a.gicv3, gicd_base, gicr_base, gicr_base_addrs,
	    PLATFORM_CORE_COUNT, aff_to_idx);
	gicv3_pe_init(&quartz64a.gicv3);
}

/** Handle interrupts for the Quartz64A platform. */
static void quartz64a_irq_exception(unsigned int exc_no, istate_t *istate)
{
	unsigned int irqid;

	gicv3_irq_get_id(&irqid);

	if (gicv3_is_spurious(irqid)) {
		/* Spurious interrupt. */
		return;
	}

	bool ack = false;

	/* Dispatch the interrupt. */
	irq_t *irq = irq_dispatch_and_lock(irqid);
	if (irq) {
		/* The IRQ handler was found. */
		if (irq->preack) {
			/* Signal end of interrupt before processing it. */
			gicv3_end(irqid);
			ack = true;
		}

		irq->handler(irq);
		irq_spinlock_unlock(&irq->lock, false);
	} else {
		/* Spurious interrupt. */
		printf("cpu%d: spurious interrupt (id=%u)\n", CPU->id, irqid);
	}

	if (!ack) {
		/* Signal end of interrupt to the controller. */
		gicv3_end(irqid);
	}
}

/** Initialize the output device for the Quartz64A platform. */
static void quartz64a_output_init(void)
{
	outdev_t *outdev = NULL;

	quartz64a.uart = dwapb_init((ioport32_t *)QUARTZ64A_UART_ADDRESS,
	    QUARTZ64A_UART_IRQ, NULL, NULL, &outdev);
	if (!quartz64a.uart)
		return;

	stdout_wire(outdev);
}

/** Initialize the input device for the Quartz64A platform. */
static void quartz64a_input_init(void)
{
	srln_instance_t *srln_instance = srln_init();
	if (srln_instance == NULL)
		return;

	indev_t *sink = stdin_wire();
	indev_t *srln = srln_wire(srln_instance, sink);
	dwapb_wire(quartz64a.uart, srln);
	gicv3_enable(&quartz64a.gicv3, QUARTZ64A_UART_IRQ);
}

/** Enable the virtual timer interrupt for the Quartz64A platform. */
static inr_t quartz64a_enable_vtimer_irq(void)
{
	gicv3_enable(&quartz64a.gicv3, QUARTZ64A_VTIMER_IRQ);
	return QUARTZ64A_VTIMER_IRQ;
}

/** Get the number of interrupts for the Quartz64A platform. */
static size_t quartz64a_get_irq_count(void)
{
	return gicv3_irq_get_total(&quartz64a.gicv3);
}

/** Get the platform name for the Quartz64A platform. */
static const char *quartz64a_get_platform_name(void)
{
	return "quartz64a";
}

/** Output a character to the early UART for the Quartz64A platform. */
static void quartz64a_early_uart_output(char32_t c)
{
	volatile uint32_t *uart_THR =
	    (volatile uint32_t *)PA2KA(QUARTZ64A_UART_ADDRESS);

	volatile uint32_t *uart_LSR =
	    (volatile uint32_t *)PA2KA(QUARTZ64A_UART_ADDRESS + 0x14);

	/* Wait until THR is empty. */
	while ((*uart_LSR & BIT(5)) == 0) {
	}

	*uart_THR = c;
}

struct arm_machine_ops quartz64a_machine_ops = {
	quartz64a_init,
	quartz64a_irq_exception,
	quartz64a_output_init,
	quartz64a_input_init,
	quartz64a_enable_vtimer_irq,
	quartz64a_get_irq_count,
	quartz64a_get_platform_name,
	quartz64a_early_uart_output,
};

/** @}
 */
