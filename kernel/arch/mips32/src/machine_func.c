/*
 * SPDX-FileCopyrightText: 2009 Vineeth Pillai
 * SPDX-FileCopyrightText: 2012 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 *  @brief Definitions of machine specific functions.
 *
 *  These functions allow us to support various kinds of mips32 machines
 *  in a unified way.
 */

#include <stddef.h>
#include <arch/machine_func.h>
#if defined(MACHINE_msim)
#include <arch/mach/msim/msim.h>
#elif defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
#include <arch/mach/malta/malta.h>
#endif

/** Pointer to machine_ops structure being used. */
struct mips32_machine_ops *machine_ops;

/** Initialize machine_ops pointer. */
void machine_ops_init(void)
{
#if defined(MACHINE_msim)
	machine_ops = &msim_machine_ops;
#elif defined(MACHINE_lmalta) || defined(MACHINE_bmalta)
	machine_ops = &malta_machine_ops;
#else
#error Machine type not defined.
#endif
}

void machine_init(void)
{
	(machine_ops->machine_init)();
}

/** Halts CPU. */
void machine_cpu_halt(void)
{
	(machine_ops->machine_cpu_halt)();
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address.
 * @param size		Place to store memory size.
 */
void machine_get_memory_extents(uintptr_t *start, size_t *size)
{
	(machine_ops->machine_get_memory_extents)(start, size);
}

/*
 * Machine specific frame initialization
 */
void machine_frame_init(void)
{
	(machine_ops->machine_frame_init)();
}

/*
 * configure the output device.
 */
void machine_output_init(void)
{
	(machine_ops->machine_output_init)();
}

/*
 * configure the input device.
 */
void machine_input_init(void)
{
	(machine_ops->machine_input_init)();
}

const char *machine_get_platform_name(void)
{
	if (machine_ops->machine_get_platform_name)
		return machine_ops->machine_get_platform_name();
	return NULL;
}

/** @}
 */
