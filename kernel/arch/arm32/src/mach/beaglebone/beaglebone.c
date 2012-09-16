/*
 * Copyright (c) 2012 Matteo Facchinetti
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
/** @addtogroup arm32beaglebone
 * @{
 */
/** @file
 *  @brief BeagleBone platform driver.
 */

#include <arch/exception.h>
#include <arch/mach/beaglebone/beaglebone.h>
#include <interrupt.h>
#include <ddi/ddi.h>
#include <ddi/device.h>

static void bbone_init(void);
static void bbone_timer_irq_start(void);
static void bbone_cpu_halt(void);
static void bbone_get_memory_extents(uintptr_t *start, size_t *size);
static void bbone_irq_exception(unsigned int exc_no, istate_t *istate);
static void bbone_frame_init(void);
static void bbone_output_init(void);
static void bbone_input_init(void);
static size_t bbone_get_irq_count(void);
static const char *bbone_get_platform_name(void);

struct arm_machine_ops bbone_machine_ops = {
	bbone_init,
	bbone_timer_irq_start,
	bbone_cpu_halt,
	bbone_get_memory_extents,
	bbone_irq_exception,
	bbone_frame_init,
	bbone_output_init,
	bbone_input_init,
	bbone_get_irq_count,
	bbone_get_platform_name
};

static void bbone_init(void)
{
}

static void bbone_timer_irq_start(void)
{
}

static void bbone_cpu_halt(void)
{
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address (physical).
 * @param size		Place to store memory size.
 */
static void bbone_get_memory_extents(uintptr_t *start, size_t *size)
{
}

static void bbone_irq_exception(unsigned int exc_no, istate_t *istate)
{
}

static void bbone_frame_init(void)
{
}

static void bbone_output_init(void)
{
}

static void bbone_input_init(void)
{
}

size_t bbone_get_irq_count(void)
{
	return 0;
}

const char *bbone_get_platform_name(void)
{
	return "beaglebone";
}

/**
 * @}
 */

