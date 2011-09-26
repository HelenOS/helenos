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
#include <str_error.h>

#include "ddf_log.h"
#include "dsp_commands.h"
#include "dsp.h"
#include "sb16.h"

/* ISA interrupts should be edge-triggered so there should be no need for
 * irq code magic */
static const irq_cmd_t irq_cmds[] = {{ .cmd = CMD_ACCEPT }};
static const irq_code_t irq_code =
    { .cmdcount = 1, .cmds = (irq_cmd_t*)irq_cmds };

static mixer_type_t mixer_type_by_dsp_version(unsigned major, unsigned minor)
{
	switch (major)
	{
	case 1: return SB_MIXER_NONE; /* SB 1.5 and early 2.0 = no mixer chip */
	case 2: return (minor == 0) ? SB_MIXER_NONE : SB_MIXER_CT1335;
	case 3: return SB_MIXER_CT1345; /* SB Pro */
	case 4: return SB_MIXER_CT1745; /* SB 16  */
	default: return SB_MIXER_UNKNOWN;
	}
}
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

	dsp_reset(drv->regs);
	/* "DSP takes about 100 microseconds to initialize itself" */
	udelay(100);

	uint8_t response;
	ret = dsp_read(drv->regs, &response);
	if (ret != EOK) {
		ddf_log_error("Failed to read DSP reset response value.\n");
		return ret;
	}

	if (response != DSP_RESET_RESPONSE) {
		ddf_log_error("Invalid DSP reset response: %x.\n", response);
		return EIO;
	}

	/* Get DSP version number */
	dsp_write(drv->regs, DSP_VERSION);
	dsp_read(drv->regs, &drv->dsp_version.major);
	dsp_read(drv->regs, &drv->dsp_version.minor);
	ddf_log_note("Sound blaster DSP (%x.%x) initialized.\n",
	    drv->dsp_version.major, drv->dsp_version.minor);

	/* Initialize mixer */
	drv->mixer = mixer_type_by_dsp_version(
	    drv->dsp_version.major, drv->dsp_version.minor);

	ret = mixer_init(drv->regs, drv->mixer);
	if (ret != EOK) {
		ddf_log_error("Failed to initialize SB mixer: %s.\n",
		    str_error(ret));
		return ret;
	}
	mixer_load_volume_levels(drv->regs, drv->mixer);
	ddf_log_note("Initialized mixer: %s.\n", mixer_type_to_str(drv->mixer));

	return EOK;
}
/*----------------------------------------------------------------------------*/
int sb16_init_mpu(sb16_drv_t *drv, void *regs, size_t size)
{
	return ENOTSUP;
}
