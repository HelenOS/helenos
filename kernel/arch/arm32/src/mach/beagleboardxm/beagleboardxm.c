/*
 * Copyright (c) 2012 Jan Vesely
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
/** @addtogroup arm32beagleboardxm
 * @{
 */
/** @file
 *  @brief BeagleBoard platform driver.
 */

#include <arch/exception.h>
#include <arch/mach/beagleboardxm/beagleboardxm.h>
#include <interrupt.h>
#include <ddi/ddi.h>
#include <ddi/device.h>

static void bbxm_init(void);
static void bbxm_timer_irq_start(void);
static void bbxm_cpu_halt(void);
static void bbxm_get_memory_extents(uintptr_t *start, size_t *size);
static void bbxm_irq_exception(unsigned int exc_no, istate_t *istate);
static void bbxm_frame_init(void);
static void bbxm_output_init(void);
static void bbxm_input_init(void);
static size_t bbxm_get_irq_count(void);
static const char *bbxm_get_platform_name(void);

struct arm_machine_ops bbxm_machine_ops = {
	bbxm_init,
	bbxm_timer_irq_start,
	bbxm_cpu_halt,
	bbxm_get_memory_extents,
	bbxm_irq_exception,
	bbxm_frame_init,
	bbxm_output_init,
	bbxm_input_init,
	bbxm_get_irq_count,
	bbxm_get_platform_name
};

static void bbxm_init(void)
{
}

static void bbxm_timer_irq_start(void)
{
}

static void bbxm_cpu_halt(void)
{
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address (physical).
 * @param size		Place to store memory size.
 */
static void bbxm_get_memory_extents(uintptr_t *start, size_t *size)
{
}

static void bbxm_irq_exception(unsigned int exc_no, istate_t *istate)
{
}

static void bbxm_frame_init(void)
{
}

static void bbxm_output_init(void)
{
}

static void bbxm_input_init(void)
{
}

size_t bbxm_get_irq_count(void)
{
	return 0;
}

const char *bbxm_get_platform_name(void)
{
	return "beagleboardxm";
}

/**
 * @}
 */
