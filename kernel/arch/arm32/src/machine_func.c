/*
 * Copyright (c) 2009 Vineeth Pillai
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

/** @addtogroup arm32
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
