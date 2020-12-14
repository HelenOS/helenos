/*
 * Copyright (c) 2016 Petr Pavlu
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

/** @addtogroup kernel_arm64_virt
 * @{
 */
/** @file
 * @brief QEMU virt platform driver.
 */

#include <arch/mach/virt/virt.h>
#include <console/console.h>
#include <genarch/drivers/gicv2/gicv2.h>
#include <genarch/drivers/pl011/pl011.h>
#include <genarch/srln/srln.h>
#include <mm/km.h>
#include <sysinfo/sysinfo.h>

#define VIRT_VTIMER_IRQ         27
#define VIRT_UART_IRQ           33
#define VIRT_GIC_DISTR_ADDRESS  0x08000000
#define VIRT_GIC_CPUI_ADDRESS   0x08010000
#define VIRT_UART_ADDRESS       0x09000000

static void virt_init(void);
static void virt_irq_exception(unsigned int exc_no, istate_t *istate);
static void virt_output_init(void);
static void virt_input_init(void);
inr_t virt_enable_vtimer_irq(void);
size_t virt_get_irq_count(void);
static const char *virt_get_platform_name(void);

struct {
	gicv2_t gicv2;
	pl011_uart_t uart;
} virt;

struct arm_machine_ops virt_machine_ops = {
	virt_init,
	virt_irq_exception,
	virt_output_init,
	virt_input_init,
	virt_enable_vtimer_irq,
	virt_get_irq_count,
	virt_get_platform_name
};

static void virt_init(void)
{
	/* Initialize interrupt controller. */
	gicv2_distr_regs_t *distr = (void *) km_map(VIRT_GIC_DISTR_ADDRESS,
	    ALIGN_UP(sizeof(*distr), PAGE_SIZE), KM_NATURAL_ALIGNMENT,
	    PAGE_NOT_CACHEABLE | PAGE_READ | PAGE_WRITE | PAGE_KERNEL);
	gicv2_cpui_regs_t *cpui = (void *) km_map(VIRT_GIC_CPUI_ADDRESS,
	    ALIGN_UP(sizeof(*cpui), PAGE_SIZE), KM_NATURAL_ALIGNMENT,
	    PAGE_NOT_CACHEABLE | PAGE_READ | PAGE_WRITE | PAGE_KERNEL);
	gicv2_init(&virt.gicv2, distr, cpui);
}

static void virt_irq_exception(unsigned int exc_no, istate_t *istate)
{
	unsigned inum, cpuid;
	gicv2_inum_get(&virt.gicv2, &inum, &cpuid);

	/* Dispatch the interrupt. */
	irq_t *irq = irq_dispatch_and_lock(inum);
	if (irq) {
		/* The IRQ handler was found. */
		irq->handler(irq);
		irq_spinlock_unlock(&irq->lock, false);
	} else {
		/* Spurious interrupt. */
		printf("cpu%d: spurious interrupt (inum=%u)\n", CPU->id, inum);
	}

	/* Signal end of interrupt to the controller. */
	gicv2_end(&virt.gicv2, inum, cpuid);
}

static void virt_output_init(void)
{
	if (!pl011_uart_init(&virt.uart, VIRT_UART_IRQ, VIRT_UART_ADDRESS))
		return;

	stdout_wire(&virt.uart.outdev);
}

static void virt_input_init(void)
{
	srln_instance_t *srln_instance = srln_init();
	if (srln_instance == NULL)
		return;

	indev_t *sink = stdin_wire();
	indev_t *srln = srln_wire(srln_instance, sink);
	pl011_uart_input_wire(&virt.uart, srln);
	gicv2_enable(&virt.gicv2, VIRT_UART_IRQ);
}

inr_t virt_enable_vtimer_irq(void)
{
	gicv2_enable(&virt.gicv2, VIRT_VTIMER_IRQ);
	return VIRT_VTIMER_IRQ;
}

size_t virt_get_irq_count(void)
{
	return gicv2_inum_get_total(&virt.gicv2);
}

const char *virt_get_platform_name(void)
{
	return "arm64virt";
}

/** @}
 */
