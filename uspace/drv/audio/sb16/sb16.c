/*
 * Copyright (c) 2011 Jan Vesely
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

#include <errno.h>
#include <libarch/ddi.h>

#include "ddf_log.h"
#include "dsp_commands.h"
#include "sb16.h"


static inline void sb16_dsp_command(sb16_drv_t *drv, dsp_command_t command)
{
	assert(drv);
	uint8_t status;
	do {
		status = pio_read_8(&drv->regs->dsp_write);
	} while ((status & DSP_WRITE_READY) == 0);

	pio_write_8(&drv->regs->dsp_write, command);
}
/*----------------------------------------------------------------------------*/
static inline uint8_t sb16_dsp_read(sb16_drv_t *drv)
{
	assert(drv);
	uint8_t status;
	do {
		status = pio_read_8(&drv->regs->dsp_read_status);
	} while ((status & DSP_READ_READY) == 0);
	return pio_read_8(&drv->regs->dsp_data_read);
}
/*----------------------------------------------------------------------------*/
/* ISA interrupts should be edge-triggered so there should be no need for
 * irq code magic */
static const irq_cmd_t irq_cmds[] = {{ .cmd = CMD_ACCEPT }};
static const irq_code_t irq_code =
    { .cmdcount = 1, .cmds = (irq_cmd_t*)irq_cmds };

/*----------------------------------------------------------------------------*/
irq_code_t * sb16_irq_code(void)
{
	return (irq_code_t*)&irq_code;
}
/*----------------------------------------------------------------------------*/
int sb16_init_sb16(sb16_drv_t *drv, void *regs, size_t size)
{
	assert(drv);
	/* Setup registers */
	int ret = pio_enable(regs, size, (void**)&drv->regs);
	if (ret != EOK)
		return ret;
	ddf_log_debug("PIO registers at %p accessible.\n", drv->regs);

	/* Reset DSP, see Chapter 2 of Sound Blaster HW programming guide */
	pio_write_8(&drv->regs->dsp_reset, 1);
	udelay(3);
	pio_write_8(&drv->regs->dsp_reset, 0);
	udelay(100);

	unsigned attempts = 100;
	uint8_t status;
	do {
		status = pio_read_8(&drv->regs->dsp_read_status);
		udelay(10);
	} while (--attempts && ((status & DSP_READ_READY) == 0));

	if (status & DSP_READ_READY) {
		const uint8_t response = sb16_dsp_read(drv);
		if (response != 0xaa) {
			ddf_log_error("Invalid DSP reset response: %x.\n",
			    response);
			return EIO;
		}
	} else {
		ddf_log_error("Failed to reset Sound Blaster DSP.\n");
		return EIO;
	}
	ddf_log_note("Sound blaster reset!\n");


	/* Get DSP version number */
	sb16_dsp_command(drv, DSP_VERSION);
	const uint8_t major = sb16_dsp_read(drv);
	const uint8_t minor = sb16_dsp_read(drv);
	ddf_log_note("Sound blaster DSP version: %x.%x.\n", major, minor);


	// TODO Initialize mixer
	return EOK;
}
/*----------------------------------------------------------------------------*/
int sb16_init_mpu(sb16_drv_t *drv, void *regs, size_t size)
{
	return ENOTSUP;
}
