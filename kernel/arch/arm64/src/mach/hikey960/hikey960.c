/*
 * SPDX-FileCopyrightText: 2021 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_hikey960
 * @{
 */
/** @file
 * @brief HiKey 960 platform driver.
 */

#include <arch/mach/hikey960/hikey960.h>
#include <console/console.h>
#include <genarch/drivers/gicv2/gicv2.h>
#include <genarch/drivers/pl011/pl011.h>
#include <genarch/srln/srln.h>
#include <mm/km.h>
#include <sysinfo/sysinfo.h>

#define HIKEY960_VTIMER_IRQ         27
#define HIKEY960_UART_IRQ           111
#define HIKEY960_GIC_DISTR_ADDRESS  0xE82B1000
#define HIKEY960_GIC_CPUI_ADDRESS   0xE82B2000
#define HIKEY960_UART_ADDRESS       0xFFF32000

struct {
	gicv2_t gicv2;
	pl011_uart_t uart;
} hikey960;

static void hikey960_init(void)
{
	/* Initialize interrupt controller. */
	gicv2_distr_regs_t *distr = (void *) km_map(HIKEY960_GIC_DISTR_ADDRESS,
	    ALIGN_UP(sizeof(*distr), PAGE_SIZE), KM_NATURAL_ALIGNMENT,
	    PAGE_NOT_CACHEABLE | PAGE_READ | PAGE_WRITE | PAGE_KERNEL);
	gicv2_cpui_regs_t *cpui = (void *) km_map(HIKEY960_GIC_CPUI_ADDRESS,
	    ALIGN_UP(sizeof(*cpui), PAGE_SIZE), KM_NATURAL_ALIGNMENT,
	    PAGE_NOT_CACHEABLE | PAGE_READ | PAGE_WRITE | PAGE_KERNEL);
	gicv2_init(&hikey960.gicv2, distr, cpui);
}

static void hikey960_irq_exception(unsigned int exc_no, istate_t *istate)
{
	unsigned int inum, cpuid;
	gicv2_inum_get(&hikey960.gicv2, &inum, &cpuid);

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
	gicv2_end(&hikey960.gicv2, inum, cpuid);
}

static void hikey960_output_init(void)
{
	if (!pl011_uart_init(&hikey960.uart, HIKEY960_UART_IRQ,
	    HIKEY960_UART_ADDRESS))
		return;

	stdout_wire(&hikey960.uart.outdev);
}

static void hikey960_input_init(void)
{
	srln_instance_t *srln_instance = srln_init();
	if (srln_instance == NULL)
		return;

	indev_t *sink = stdin_wire();
	indev_t *srln = srln_wire(srln_instance, sink);
	pl011_uart_input_wire(&hikey960.uart, srln);
	gicv2_enable(&hikey960.gicv2, HIKEY960_UART_IRQ);
}

static inr_t hikey960_enable_vtimer_irq(void)
{
	gicv2_enable(&hikey960.gicv2, HIKEY960_VTIMER_IRQ);
	return HIKEY960_VTIMER_IRQ;
}

static size_t hikey960_get_irq_count(void)
{
	return gicv2_inum_get_total(&hikey960.gicv2);
}

static const char *hikey960_get_platform_name(void)
{
	return "hikey960";
}

static void hikey960_early_uart_output(char32_t c)
{
	volatile uint32_t *uartdr = (volatile uint32_t *)
	    PA2KA(HIKEY960_UART_ADDRESS);
	volatile uint32_t *uartfr = (volatile uint32_t *)
	    PA2KA(HIKEY960_UART_ADDRESS + 24);

	while (*uartfr & 0x20U) {
	}

	*uartdr = c;
}

struct arm_machine_ops hikey960_machine_ops = {
	hikey960_init,
	hikey960_irq_exception,
	hikey960_output_init,
	hikey960_input_init,
	hikey960_enable_vtimer_irq,
	hikey960_get_irq_count,
	hikey960_get_platform_name,
	hikey960_early_uart_output
};

/** @}
 */
