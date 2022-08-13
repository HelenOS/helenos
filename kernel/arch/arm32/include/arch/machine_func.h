/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 * SPDX-FileCopyrightText: 2009 Vineeth Pillai
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Declarations of machine specific functions.
 *
 *  These functions enable to differentiate more kinds of ARM emulators
 *  or CPUs. It's the same concept as "arch" functions on the architecture
 *  level.
 */

#ifndef KERN_arm32_MACHINE_FUNC_H_
#define KERN_arm32_MACHINE_FUNC_H_

#include <console/console.h>
#include <typedefs.h>
#include <arch/exception.h>

struct arm_machine_ops {
	void (*machine_init)(void);
	void (*machine_timer_irq_start)(void);
	void (*machine_cpu_halt)(void);
	void (*machine_get_memory_extents)(uintptr_t *, size_t *);
	void (*machine_irq_exception)(unsigned int, istate_t *);
	void (*machine_frame_init)(void);
	void (*machine_output_init)(void);
	void (*machine_input_init)(void);
	size_t (*machine_get_irq_count)(void);
	const char *(*machine_get_platform_name)(void);
};

/** Pointer to arm_machine_ops structure being used. */
extern struct arm_machine_ops *machine_ops;

/** Initialize machine_ops pointer. */
extern void machine_ops_init(void);

/** Maps HW devices to the kernel address space using #hw_map. */
extern void machine_init(void);

/** Starts timer. */
extern void machine_timer_irq_start(void);

/** Halts CPU. */
extern void machine_cpu_halt(void);

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address.
 * @param size		Place to store memory size.
 */
extern void machine_get_memory_extents(uintptr_t *start, size_t *size);

/** Interrupt exception handler.
 *
 * @param exc_no Interrupt exception number.
 * @param istate Saved processor state.
 */
extern void machine_irq_exception(unsigned int exc_no, istate_t *istate);

/*
 * Machine specific frame initialization
 */
extern void machine_frame_init(void);

/*
 * configure the serial line output device.
 */
extern void machine_output_init(void);

/*
 * configure the serial line input device.
 */
extern void machine_input_init(void);

extern size_t machine_get_irq_count(void);

extern const char *machine_get_platform_name(void);

#endif

/** @}
 */
