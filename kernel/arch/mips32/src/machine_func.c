/*
 * Copyright (c) 2009 Vineeth Pillai
 * Copyright (c) 2012 Jakub Jermar
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
