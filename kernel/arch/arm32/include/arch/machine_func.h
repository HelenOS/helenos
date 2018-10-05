/*
 * Copyright (c) 2007 Michal Kebrt
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
