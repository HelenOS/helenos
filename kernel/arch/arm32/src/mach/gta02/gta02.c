/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup arm32gxemul
 * @{
 */
/** @file
 *  @brief Openmoko GTA02 (Neo FreeRunner) platform driver.
 */

#include <arch/exception.h>
#include <arch/mach/gta02/gta02.h>

static void gta02_init(void);
static void gta02_timer_irq_start(void);
static void gta02_cpu_halt(void);
static uintptr_t gta02_get_memory_size(void);
static void gta02_irq_exception(unsigned int exc_no, istate_t *istate);
static void gta02_frame_init(void);
static void gta02_output_init(void);
static void gta02_input_init(void);

struct arm_machine_ops gta02_machine_ops = {
	gta02_init,
	gta02_timer_irq_start,
	gta02_cpu_halt,
	gta02_get_memory_size,
	gta02_irq_exception,
	gta02_frame_init,
	gta02_output_init,
	gta02_input_init
};

static void gta02_init(void)
{
}

static void gta02_timer_irq_start(void)
{
}

static void gta02_cpu_halt(void)
{
}

static uintptr_t gta02_get_memory_size(void)
{
	return 0;
}

static void gta02_irq_exception(unsigned int exc_no, istate_t *istate)
{
}

static void gta02_frame_init(void)
{
}

static void gta02_output_init(void)
{
}

static void gta02_input_init(void)
{
}

/** @}
 */
