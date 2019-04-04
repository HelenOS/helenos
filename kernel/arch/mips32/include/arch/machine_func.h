/*
 * Copyright (c) 2007 Michal Kebrt
 * Copyright (c) 2009 Vineeth Pillai
 * Copyrithg (c) 2012 Jakub Jermar
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
