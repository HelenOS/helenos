/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 * SPDX-FileCopyrightText: 2009 Vineeth Pillai
 * SPDX-FileCopyrightText: 2012 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 *  @brief Declarations of machine specific functions.
 *
 *  These functions allow us to support various kinds of mips32 machines
 *  in a unified way.
 */

#ifndef KERN_mips32_MACHINE_FUNC_H_
#define KERN_mips32_MACHINE_FUNC_H_

#include <typedefs.h>
#include <stdbool.h>

struct mips32_machine_ops {
	void (*machine_init)(void);
	void (*machine_cpu_halt)(void);
	void (*machine_get_memory_extents)(uintptr_t *, size_t *);
	void (*machine_frame_init)(void);
	void (*machine_output_init)(void);
	void (*machine_input_init)(void);
	const char *(*machine_get_platform_name)(void);
};

/** Pointer to mips32_machine_ops structure being used. */
extern struct mips32_machine_ops *machine_ops;

extern void machine_ops_init(void);

extern void machine_init(void);
extern void machine_cpu_halt(void);
extern void machine_get_memory_extents(uintptr_t *, size_t *);
extern void machine_frame_init(void);
extern void machine_output_init(void);
extern void machine_input_init(void);
extern const char *machine_get_platform_name(void);

#endif

/** @}
 */
