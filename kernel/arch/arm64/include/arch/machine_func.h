/*
 * SPDX-FileCopyrightText: 2016 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64
 * @{
 */
/** @file
 * @brief Declarations of machine specific functions.
 *
 * These functions enable to differentiate more kinds of ARM platforms.
 */

#ifndef KERN_arm64_MACHINE_FUNC_H_
#define KERN_arm64_MACHINE_FUNC_H_

#include <arch/istate.h>
#include <typedefs.h>

struct arm_machine_ops {
	void (*machine_init)(void);
	void (*machine_irq_exception)(unsigned int, istate_t *);
	void (*machine_output_init)(void);
	void (*machine_input_init)(void);
	inr_t (*machine_enable_vtimer_irq)(void);
	size_t (*machine_get_irq_count)(void);
	const char *(*machine_get_platform_name)(void);
	void (*machine_early_uart_output)(char32_t);
};

extern void machine_ops_init(void);
extern void machine_init(void);
void machine_irq_exception(unsigned int, istate_t *);
extern void machine_output_init(void);
extern void machine_input_init(void);
extern inr_t machine_enable_vtimer_irq(void);
extern size_t machine_get_irq_count(void);
extern const char *machine_get_platform_name(void);
extern void machine_early_uart_output(char32_t);

#endif

/** @}
 */
