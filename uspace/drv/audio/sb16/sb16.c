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
#include <macros.h>
#include <audio_mixer_iface.h>
#include <audio_pcm_iface.h>

#include "ddf_log.h"
#include "dsp_commands.h"
#include "dsp.h"
#include "sb16.h"

extern audio_mixer_iface_t sb_mixer_iface;
extern audio_pcm_iface_t sb_pcm_iface;

static ddf_dev_ops_t sb_mixer_ops = {
	.interfaces[AUDIO_MIXER_IFACE] = &sb_mixer_iface,
};

static ddf_dev_ops_t sb_pcm_ops = {
	.interfaces[AUDIO_PCM_BUFFER_IFACE] = &sb_pcm_iface,
};

/*
 * ISA interrupts should be edge-triggered so there should be no need for
 * irq code magic, but we still need to ack those interrupts ASAP.
 */
static const irq_cmd_t irq_cmds[] = {
	{ .cmd = CMD_PIO_READ_8, .dstarg = 1 }, /* Address patched at runtime */
	{ .cmd = CMD_PIO_READ_8, .dstarg = 1 }, /* Address patched at runtime */
	{ .cmd = CMD_ACCEPT },
};

static inline sb_mixer_type_t sb_mixer_type_by_dsp_version(
    unsigned major, unsigned minor)
{
	switch (major) {
	case 1:
		return SB_MIXER_NONE; /* SB 1.5 and early 2.0 = no mixer chip */
	case 2:
		return (minor == 0) ? SB_MIXER_NONE : SB_MIXER_CT1335;
	case 3:
		return SB_MIXER_CT1345; /* SB Pro */
	case 4:
		return SB_MIXER_CT1745; /* SB 16  */
	default:
		return SB_MIXER_UNKNOWN;
	}
}

size_t sb16_irq_code_size(void)
{
	return ARRAY_SIZE(irq_cmds);
}

void sb16_irq_code(addr_range_t *regs, int dma8, int dma16, irq_cmd_t cmds[],
    irq_pio_range_t ranges[])
{
	assert(regs);
	assert(dma8 > 0 && dma8 < 4);

	sb16_regs_t *registers = RNGABSPTR(*regs);
	memcpy(cmds, irq_cmds, sizeof(irq_cmds));
	cmds[0].addr = (void *) &registers->dsp_read_status;
	ranges[0].base = (uintptr_t) registers;
	ranges[0].size = sizeof(*registers);
	if (dma16 > 4 && dma16 < 8) {
		/* Valid dma16 */
		cmds[1].addr = (void *) &registers->dma16_ack;
	} else {
		cmds[1].cmd = CMD_ACCEPT;
	}
}

errno_t sb16_init_sb16(sb16_t *sb, addr_range_t *regs, ddf_dev_t *dev, int dma8,
    int dma16)
{
	assert(sb);

	/* Setup registers */
	errno_t ret = pio_enable_range(regs, (void **) &sb->regs);
	if (ret != EOK)
		return ret;
	ddf_log_note("PIO registers at %p accessible.", sb->regs);

	/* Initialize DSP */
	ddf_fun_t *dsp_fun = ddf_fun_create(dev, fun_exposed, "pcm");
	if (!dsp_fun) {
		ddf_log_error("Failed to create dsp function.");
		return ENOMEM;
	}

	ret = sb_dsp_init(&sb->dsp, sb->regs, dev, dma8, dma16);
	if (ret != EOK) {
		ddf_log_error("Failed to initialize SB DSP: %s.",
		    str_error(ret));
		ddf_fun_destroy(dsp_fun);
		return ret;
	}

	ddf_fun_set_ops(dsp_fun, &sb_pcm_ops);
	ddf_log_note("Sound blaster DSP (%x.%x) initialized.",
	    sb->dsp.version.major, sb->dsp.version.minor);

	ret = ddf_fun_bind(dsp_fun);
	if (ret != EOK) {
		ddf_log_error(
		    "Failed to bind PCM function: %s.", str_error(ret));
		ddf_fun_destroy(dsp_fun);
		return ret;
	}

	ret = ddf_fun_add_to_category(dsp_fun, "audio-pcm");
	if (ret != EOK) {
		ddf_log_error("Failed register PCM function in category: %s.",
		    str_error(ret));
		ddf_fun_unbind(dsp_fun);
		ddf_fun_destroy(dsp_fun);
		return ret;
	}

	/* Initialize mixer */
	const sb_mixer_type_t mixer_type = sb_mixer_type_by_dsp_version(
	    sb->dsp.version.major, sb->dsp.version.minor);

	ddf_fun_t *mixer_fun = ddf_fun_create(dev, fun_exposed, "control");
	if (!mixer_fun) {
		ddf_log_error("Failed to create mixer function.");
		ddf_fun_unbind(dsp_fun);
		ddf_fun_destroy(dsp_fun);
		return ENOMEM;
	}
	ret = sb_mixer_init(&sb->mixer, sb->regs, mixer_type);
	if (ret != EOK) {
		ddf_log_error("Failed to initialize SB mixer: %s.",
		    str_error(ret));
		ddf_fun_unbind(dsp_fun);
		ddf_fun_destroy(dsp_fun);
		ddf_fun_destroy(mixer_fun);
		return ret;
	}

	ddf_log_note("Initialized mixer: %s.",
	    sb_mixer_type_str(sb->mixer.type));
	ddf_fun_set_ops(mixer_fun, &sb_mixer_ops);

	ret = ddf_fun_bind(mixer_fun);
	if (ret != EOK) {
		ddf_log_error(
		    "Failed to bind mixer function: %s.", str_error(ret));
		ddf_fun_destroy(mixer_fun);
		ddf_fun_unbind(dsp_fun);
		ddf_fun_destroy(dsp_fun);
		return ret;
	}

	return EOK;
}

errno_t sb16_init_mpu(sb16_t *sb, addr_range_t *regs)
{
	sb->mpu_regs = NULL;
	return ENOTSUP;
}

void sb16_interrupt(sb16_t *sb)
{
	assert(sb);
	/*
	 * The acknowledgment of interrupts on DSP version 4.xx is different;
	 * It can contain MPU-401 indicator and DMA16 transfers are acked
	 * differently
	 */
	if (sb->dsp.version.major >= 4) {
		pio_write_8(&sb->regs->mixer_address, MIXER_IRQ_STATUS_ADDRESS);
		const uint8_t irq_mask = pio_read_8(&sb->regs->mixer_data);
		/* Third bit is MPU-401 interrupt */
		if (irq_mask & 0x4) {
			return;
		}
	} else {
		ddf_log_debug("SB16 interrupt.");
	}
	sb_dsp_interrupt(&sb->dsp);
}
