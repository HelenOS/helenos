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
#include <arch/mm/page.h>
#include <mm/page.h>
#include <genarch/drivers/s3c24xx_uart/s3c24xx_uart.h>

#define GTA02_MEMORY_START	0x30000000	/* physical */
#define GTA02_MEMORY_SIZE	0x08000000	/* 128 MB */
#define GTA02_MEMORY_SKIP	0x8000

/** GTA02 serial console UART address (UART S3C24XX CPU UART channel 2). */
#define GTA02_SCONS_BASE	0x50008000

static void gta02_init(void);
static void gta02_timer_irq_start(void);
static void gta02_cpu_halt(void);
static void gta02_get_memory_extents(uintptr_t *start, uintptr_t *size);
static void gta02_irq_exception(unsigned int exc_no, istate_t *istate);
static void gta02_frame_init(void);
static void gta02_output_init(void);
static void gta02_input_init(void);

static void *gta02_scons_out;

struct arm_machine_ops gta02_machine_ops = {
	gta02_init,
	gta02_timer_irq_start,
	gta02_cpu_halt,
	gta02_get_memory_extents,
	gta02_irq_exception,
	gta02_frame_init,
	gta02_output_init,
	gta02_input_init
};

static void gta02_init(void)
{
	gta02_scons_out = (void *) hw_map(GTA02_SCONS_BASE, PAGE_SIZE);
}

static void gta02_timer_irq_start(void)
{
}

static void gta02_cpu_halt(void)
{
}

/** Get extents of available memory.
 *
 * @param start		Place to store memory start address (physical).
 * @param size		Place to store memory size.
 */
static void gta02_get_memory_extents(uintptr_t *start, uintptr_t *size)
{
	*start = GTA02_MEMORY_START + GTA02_MEMORY_SKIP;
	*size  = GTA02_MEMORY_SIZE - GTA02_MEMORY_SKIP;
}

static void gta02_irq_exception(unsigned int exc_no, istate_t *istate)
{
}

static void gta02_frame_init(void)
{
}

static void gta02_output_init(void)
{
	outdev_t *scons_dev;

	scons_dev = s3c24xx_uart_init((ioport8_t *) gta02_scons_out);
	if (scons_dev)
		stdout_wire(scons_dev);
}

static void gta02_input_init(void)
{
}

/** @}
 */
