/*
 * Copyright (c) 2013 Jakub Jermar
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

/** @addtogroup mips32
 * @{
 */
/** @file
 *  @brief msim platform driver.
 */

#include <arch/mach/msim/msim.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <arch/drivers/msim.h>
#include <genarch/drivers/dsrln/dsrlnin.h>
#include <genarch/drivers/dsrln/dsrlnout.h>
#include <genarch/srln/srln.h>

static void msim_init(void);
static void msim_cpu_halt(void);
static void msim_get_memory_extents(uintptr_t *, size_t *);
static void msim_frame_init(void);
static void msim_output_init(void);
static void msim_input_init(void);
static const char *msim_get_platform_name(void);

struct mips32_machine_ops msim_machine_ops = {
	.machine_init = msim_init,
	.machine_cpu_halt = msim_cpu_halt,
	.machine_get_memory_extents = msim_get_memory_extents,
	.machine_frame_init = msim_frame_init,
	.machine_output_init = msim_output_init,
	.machine_input_init = msim_input_init,
	.machine_get_platform_name = msim_get_platform_name
};

void msim_init(void)
{
	cp0_unmask_int(MSIM_DDISK_IRQ);
}

void msim_cpu_halt(void)
{
}

void msim_get_memory_extents(uintptr_t *start, size_t *size)
{
}

void msim_frame_init(void)
{
}

void msim_output_init(void)
{
#ifdef CONFIG_MSIM_PRN
	outdev_t *dsrlndev = dsrlnout_init((ioport8_t *) MSIM_KBD_ADDRESS);
	if (dsrlndev)
		stdout_wire(dsrlndev);
#endif
}

void msim_input_init(void)
{
#ifdef CONFIG_MSIM_KBD
	/*
	 * Initialize the msim keyboard port. Then initialize the serial line
	 * module and connect it to the msim keyboard. Enable keyboard
	 * interrupts.
	 */
	dsrlnin_instance_t *dsrlnin_instance
	    = dsrlnin_init((dsrlnin_t *) MSIM_KBD_ADDRESS, MSIM_KBD_IRQ);
	if (dsrlnin_instance) {
		srln_instance_t *srln_instance = srln_init();
		if (srln_instance) {
			indev_t *sink = stdin_wire();
			indev_t *srln = srln_wire(srln_instance, sink);
			dsrlnin_wire(dsrlnin_instance, srln);
			cp0_unmask_int(MSIM_KBD_IRQ);
		}
	}
#endif
}

const char *msim_get_platform_name(void)
{
	return "msim";
}

/** @}
 */
