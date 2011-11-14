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

#include "beep.h"
#include "ddf_log.h"
#include "dsp_commands.h"
#include "dsp.h"
#include "sb16.h"

/* ISA interrupts should be edge-triggered so there should be no need for
 * irq code magic */
static const irq_cmd_t irq_cmds[] = {{ .cmd = CMD_ACCEPT }};
static const irq_code_t irq_code =
    { .cmdcount = 1, .cmds = (irq_cmd_t*)irq_cmds }; // FIXME: Remove cast

static inline sb_mixer_type_t sb_mixer_type_by_dsp_version(
    unsigned major, unsigned minor)
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
	// FIXME: Remove this cast
	return (irq_code_t*)&irq_code;
}
/*----------------------------------------------------------------------------*/
int sb16_init_sb16(sb16_t *sb, void *regs, size_t size,
    ddf_dev_t *dev, int dma8, int dma16)
{
	assert(sb);
	/* Setup registers */
	int ret = pio_enable(regs, size, (void**)&sb->regs);
	if (ret != EOK)
		return ret;
	ddf_log_debug("PIO registers at %p accessible.\n", sb->regs);

	/* Initialize DSP */
	ret = sb_dsp_init(&sb->dsp, sb->regs, dev, dma8, dma16);
	if (ret != EOK) {
		ddf_log_error("Failed to initialize SB DSP: %s.\n",
		    str_error(ret));
		return ret;
	}
	ddf_log_note("Sound blaster DSP (%x.%x) initialized.\n",
	    sb->dsp.version.major, sb->dsp.version.minor);

	/* Initialize mixer */
	const sb_mixer_type_t mixer_type = sb_mixer_type_by_dsp_version(
	    sb->dsp.version.major, sb->dsp.version.minor);

	ret = sb_mixer_init(&sb->mixer, sb->regs, mixer_type);
	if (ret != EOK) {
		ddf_log_error("Failed to initialize SB mixer: %s.\n",
		    str_error(ret));
		return ret;
	}
	ddf_log_note("Initialized mixer: %s.\n",
	    sb_mixer_type_str(sb->mixer.type));

	ddf_log_note("Playing startup sound.\n");
	sb_dsp_play(&sb->dsp, beep, beep_size, 44100, 1, 8);

	return EOK;
}
/*----------------------------------------------------------------------------*/
int sb16_init_mpu(sb16_t *sb, void *regs, size_t size)
{
	sb->mpu_regs = NULL;
	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
void sb16_interrupt(sb16_t *sb)
{
	assert(sb);
	/* The acknowledgment of interrupts on DSP version 4.xx is different;
	 * It can contain MPU-401 indicator and DMA16 transfers are acked
	 * differently */
	if (sb->dsp.version.major >= 4) {
		pio_write_8(&sb->regs->mixer_address, MIXER_IRQ_STATUS_ADDRESS);
		const uint8_t irq_mask = pio_read_8(&sb->regs->mixer_data);
		/* Third bit is MPU-401 interrupt */
		if (irq_mask & 0x4) {
			return;
		}
	} else {
		ddf_log_debug("SB16 interrupt.\n");
	}
	sb_dsp_interrupt(&sb->dsp);
}
