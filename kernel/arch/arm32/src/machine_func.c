/*
 * SPDX-FileCopyrightText: 2009 Vineeth Pillai
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Definitions of machine specific functions.
 *
 *  These functions enable to differentiate more kinds of ARM emulators
 *  or CPUs. It's the same concept as "arch" functions on the architecture
 *  level.
 */

#include <arch/machine_func.h>
#include <arch/mach/gta02/gta02.h>
#include <arch/mach/integratorcp/integratorcp.h>
#include <arch/mach/beagleboardxm/beagleboardxm.h>
#include <arch/mach/beaglebone/beaglebone.h>
#include <arch/mach/raspberrypi/raspberrypi.h>

/** Pointer to machine_ops structure being used. */
struct arm_machine_ops *machine_ops;

/** Initialize machine_ops pointer. */
void machine_ops_init(void)
{
#if defined(MACHINE_gta02)
	machine_ops = &gta02_machine_ops;
#elif defined(MACHINE_integratorcp)
	machine_ops = &icp_machine_ops;
#elif defined(MACHINE_beagleboardxm)
	machine_ops = &bbxm_machine_ops;
#elif defined(MACHINE_beaglebone)
	machine_ops = &bbone_machine_ops;
#elif defined(MACHINE_raspberrypi)
	machine_ops = &raspberrypi_machine_ops;
#else
#error Machine type not defined.
#endif
}

/** Maps HW devices to the kernel address space using #hw_map. */
void machine_init(void)
{
	(machine_ops->machine_init)();
}

/** Starts timer. */
void machine_timer_irq_start(void)
{
	(machine_ops->machine_timer_irq_start)();
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

/** Interrupt exception handler.
 *
 * @param exc_no Interrupt exception number.
 * @param istate Saved processor state.
 */
void machine_irq_exception(unsigned int exc_no, istate_t *istate)
{
	(machine_ops->machine_irq_exception)(exc_no, istate);
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

/** Get IRQ number range used by machine. */
size_t machine_get_irq_count(void)
{
	return (machine_ops->machine_get_irq_count)();
}

const char *machine_get_platform_name(void)
{
	if (machine_ops->machine_get_platform_name)
		return machine_ops->machine_get_platform_name();
	return NULL;
}
/** @}
 */
