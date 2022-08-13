/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_SMP_H_
#define KERN_ia32_SMP_H_

#include <stdbool.h>
#include <stddef.h>

/** SMP config opertaions interface. */
struct smp_config_operations {
	/** Check whether a processor is enabled. */
	bool (*cpu_enabled)(size_t);

	/** Check whether a processor is BSP. */
	bool (*cpu_bootstrap)(size_t);

	/** Return APIC ID of a processor. */
	uint8_t (*cpu_apic_id)(size_t);

	/** Return mapping between IRQ and APIC pin. */
	int (*irq_to_pin)(unsigned int);
};

extern int smp_irq_to_pin(unsigned int);

#endif

/** @}
 */
