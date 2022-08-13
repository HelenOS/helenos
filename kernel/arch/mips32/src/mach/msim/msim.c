/*
 * SPDX-FileCopyrightText: 2013 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 *  @brief msim platform driver.
 */

#include <arch/mach/msim/msim.h>
#include <arch/mach/msim/dorder.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <genarch/drivers/dsrln/dsrlnin.h>
#include <genarch/drivers/dsrln/dsrlnout.h>
#include <genarch/srln/srln.h>
#include <stdbool.h>

static void msim_init(void);
static void msim_cpu_halt(void);
static void msim_get_memory_extents(uintptr_t *, size_t *);
static void msim_frame_init(void);
static void msim_output_init(void);
static void msim_input_init(void);
static const char *msim_get_platform_name(void);

struct mips32_machine_ops msim_machine_ops = {
	.machine_init = msim_init,
	.machine_cpu_halt = msim_cpu_halt,
	.machine_get_memory_extents = msim_get_memory_extents,
	.machine_frame_init = msim_frame_init,
	.machine_output_init = msim_output_init,
	.machine_input_init = msim_input_init,
	.machine_get_platform_name = msim_get_platform_name
};

static void msim_irq_handler(unsigned int i)
{
	irq_t *irq = irq_dispatch_and_lock(i);
	if (irq) {
		irq->handler(irq);
		irq_spinlock_unlock(&irq->lock, false);
	} else {
#ifdef CONFIG_DEBUG
		log(LF_ARCH, LVL_DEBUG, "cpu%u: spurious IRQ (irq=%u)",
		    CPU->id, i);
#endif
	}
}

void msim_init(void)
{
	irq_init(HW_INTERRUPTS, HW_INTERRUPTS);

	int_handler[INT_HW0] = msim_irq_handler;
	int_handler[INT_HW1] = msim_irq_handler;
	int_handler[INT_HW2] = msim_irq_handler;
	int_handler[INT_HW3] = msim_irq_handler;
	int_handler[INT_HW4] = msim_irq_handler;

	dorder_init();
	cp0_unmask_int(MSIM_DDISK_IRQ);
}

void msim_cpu_halt(void)
{
}

void msim_get_memory_extents(uintptr_t *start, size_t *size)
{
}

void msim_frame_init(void)
{
}

void msim_output_init(void)
{
#ifdef CONFIG_MSIM_PRN
	outdev_t *dsrlndev = dsrlnout_init((ioport8_t *) MSIM_KBD_ADDRESS,
	    KSEG12PA(MSIM_KBD_ADDRESS));
	if (dsrlndev)
		stdout_wire(dsrlndev);
#endif
}

void msim_input_init(void)
{
#ifdef CONFIG_MSIM_KBD
	/*
	 * Initialize the msim keyboard port. Then initialize the serial line
	 * module and connect it to the msim keyboard. Enable keyboard
	 * interrupts.
	 */
	dsrlnin_instance_t *dsrlnin_instance =
	    dsrlnin_init((dsrlnin_t *) MSIM_KBD_ADDRESS, MSIM_KBD_IRQ);
	if (dsrlnin_instance) {
		srln_instance_t *srln_instance = srln_init();
		if (srln_instance) {
			indev_t *sink = stdin_wire();
			indev_t *srln = srln_wire(srln_instance, sink);
			dsrlnin_wire(dsrlnin_instance, srln);
			cp0_unmask_int(MSIM_KBD_IRQ);
		}
	}
#endif
}

const char *msim_get_platform_name(void)
{
	return "msim";
}

/** @}
 */
