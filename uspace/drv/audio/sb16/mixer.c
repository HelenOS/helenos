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

#include <bool.h>
#include <errno.h>
#include <libarch/ddi.h>

#include "ddf_log.h"
#include "mixer.h"

typedef struct channel {
	uint8_t address;
	unsigned shift;
	unsigned volume_levels;
	bool preserve_bits;
} channel_t;

typedef struct volume_item {
	const char *description;
	uint8_t channels;
	const channel_t *channel_table;
} volume_item_t;

/* CT1335 channels */
static const channel_t channels_table_ct1335[] = {
	{ 0x02, 1, 8, false }, /* Master, Mono, 3bit volume level */
	{ 0x06, 1, 8, false }, /* Midi, Mono, 3bit volume level */
	{ 0x08, 1, 8, false }, /* CD, Mono, 3bit volume level */
	{ 0x0a, 1, 4, false }, /* Voice, Mono, 2bit volume level */
};

/* CT1345 channels */
static const channel_t channels_table_ct1345[] = {
	{ 0x22, 5, 8, true }, /* Master, Left, 3bit volume level */
	{ 0x22, 1, 8, true }, /* Master, Right, 3bit volume level */
	{ 0x26, 5, 8, true }, /* Midi, Left, 3bit volume level */
	{ 0x26, 1, 8, true }, /* Midi, Right, 3bit volume level */
	{ 0x28, 5, 8, true }, /* CD, Left, 3bit volume level */
	{ 0x28, 1, 8, true }, /* CD, Right, 3bit volume level */
	{ 0x2e, 5, 8, true }, /* Line, Left, 3bit volume level */
	{ 0x2e, 1, 8, true }, /* Line, Right, 3bit volume level */
	{ 0x04, 5, 8, true }, /* Voice, Left, 3bit volume level */
	{ 0x04, 1, 8, true }, /* Voice, Right, 3bit volume level */
	{ 0x0a, 1, 4, false }, /* Mic, Mono, 2bit volume level */
};

/* CT1745 channels */
static const channel_t channels_table_ct1745[] = {
	{ 0x30, 3, 32, false }, /* Master, Left, 5bit volume level */
	{ 0x31, 3, 32, false }, /* Master, Right, 5bit volume level */
	{ 0x32, 3, 32, false }, /* Voice, Left, 5bit volume level */
	{ 0x33, 3, 32, false }, /* Voice, Right, 5bit volume level */
	{ 0x34, 3, 32, false }, /* MIDI, Left, 5bit volume level */
	{ 0x35, 3, 32, false }, /* MIDI, Right, 5bit volume level */
	{ 0x36, 3, 32, false }, /* CD, Left, 5bit volume level */
	{ 0x37, 3, 32, false }, /* CD, Right, 5bit volume level */
	{ 0x38, 3, 32, false }, /* Line, Left, 5bit volume level */
	{ 0x39, 3, 32, false }, /* Line, Right, 5bit volume level */
	{ 0x3a, 3, 32, false }, /* Mic, Mono, 5bit volume level */
	{ 0x3b, 6, 4, false }, /* PC speaker, Mono, 2bit volume level */
	{ 0x3f, 6, 4, false }, /* Input Gain, Left, 2bit volume level */
	{ 0x40, 6, 4, false }, /* Input Gain, Right, 2bit volume level */
	{ 0x41, 6, 4, false }, /* Output Gain, Left, 2bit volume level */
	{ 0x42, 6, 4, false }, /* Output Gain, Right, 2bit volume level */
	{ 0x44, 4, 16, false }, /* Treble, Left, 4bit volume level */
	{ 0x45, 4, 16, false }, /* Treble, Right, 4bit volume level */
	{ 0x46, 4, 16, false }, /* Bass, Left, 4bit volume level */
	{ 0x47, 4, 16, false }, /* Bass, Right, 4bit volume level */
};

static const volume_item_t volume_ct1335[] = {
	{ "Master", 1, &channels_table_ct1335[0] },
	{ "MIDI", 1, &channels_table_ct1335[1] },
	{ "CD", 1, &channels_table_ct1335[2] },
	{ "Voice", 1, &channels_table_ct1335[3] },
};

static const volume_item_t volume_ct1345[] = {
	{ "Master", 2, &channels_table_ct1345[0] },
	{ "Voice", 2, &channels_table_ct1345[8] },
	{ "Mic", 1, &channels_table_ct1345[10] },
	{ "MIDI", 2, &channels_table_ct1345[2] },
	{ "CD", 2, &channels_table_ct1345[4] },
	{ "Line", 2, &channels_table_ct1345[6] },
};

static const volume_item_t volume_ct1745[] = {
	{ "Master", 2, &channels_table_ct1745[0] },
	{ "Voice", 2, &channels_table_ct1745[2] },
	{ "MIDI", 2, &channels_table_ct1745[4] },
	{ "CD", 2, &channels_table_ct1745[6] },
	{ "Line", 2, &channels_table_ct1745[8] },
	{ "Mic", 1, &channels_table_ct1745[10] },
	{ "PC Speaker", 1, &channels_table_ct1745[11] },
	{ "Input Gain", 2, &channels_table_ct1745[12] },
	{ "Output Gain", 2, &channels_table_ct1745[14] },
	{ "Treble", 2, &channels_table_ct1745[16] },
	{ "Bass", 2, &channels_table_ct1745[0] },
};

static const struct {
	size_t count;
	const volume_item_t *table;
} volume_table[] = {
	[SB_MIXER_NONE] = { 0, NULL },
	[SB_MIXER_UNKNOWN] = { 0, NULL },
	[SB_MIXER_CT1335] = {
	    sizeof(volume_ct1335) / sizeof(volume_item_t), volume_ct1335 },
	[SB_MIXER_CT1345] = {
	    sizeof(volume_ct1345) / sizeof(volume_item_t), volume_ct1345 },
	[SB_MIXER_CT1745] = {
	    sizeof(volume_ct1745) / sizeof(volume_item_t), volume_ct1745 },
};

static void sb_mixer_max_master_levels(sb_mixer_t *mixer)
{
	assert(mixer);
	/* Set Master to maximum */
	if (!sb_mixer_get_control_item_count(mixer))
		return;
	const unsigned item = 0; /* 0 is Master. */
	unsigned levels = 0, channels = 0, level;
	const char *name = NULL;

	sb_mixer_get_control_item_info(mixer, item, &name, &channels, &levels);
	for (unsigned channel = 0; channel < channels; ++channel) {
		level = sb_mixer_get_volume_level(mixer, item, channel);
		ddf_log_note("Setting %s channel %d to %d (%d).\n",
		    name, channel, levels - 1, level);

		sb_mixer_set_volume_level(mixer, item, channel, levels - 1);

		level = sb_mixer_get_volume_level(mixer, item, channel);
		ddf_log_note("%s channel %d set to %d.\n",
		    name, channel, level);
	}
}
/*----------------------------------------------------------------------------*/
const char * sb_mixer_type_str(sb_mixer_type_t type)
{
	static const char * names[] = {
		[SB_MIXER_CT1335] = "CT 1335",
		[SB_MIXER_CT1345] = "CT 1345",
		[SB_MIXER_CT1745] = "CT 1745",
		[SB_MIXER_UNKNOWN] = "Unknown mixer",
	};
	return names[type];
}
/*----------------------------------------------------------------------------*/
int sb_mixer_init(sb_mixer_t *mixer, sb16_regs_t *regs, sb_mixer_type_t type)
{
	assert(mixer);
	mixer->regs = regs;
	mixer->type = type;
	if (type == SB_MIXER_UNKNOWN)
		return ENOTSUP;

	if (type != SB_MIXER_NONE) {
		pio_write_8(&regs->mixer_address, MIXER_RESET_ADDRESS);
		pio_write_8(&regs->mixer_data, 1);
		sb_mixer_max_master_levels(mixer);
	}
	pio_write_8(&regs->mixer_address, MIXER_PNP_IRQ_ADDRESS);
	const uint8_t irq = pio_read_8(&regs->mixer_data);
	pio_write_8(&regs->mixer_address, MIXER_PNP_DMA_ADDRESS);
	const uint8_t dma = pio_read_8(&regs->mixer_data);
	ddf_log_debug("SB16 setup with IRQ 0x%hhx and DMA 0x%hhx.\n", irq, dma);
	return EOK;
}
/*----------------------------------------------------------------------------*/
int sb_mixer_get_control_item_count(const sb_mixer_t *mixer)
{
	assert(mixer);
	return volume_table[mixer->type].count;
}
/*----------------------------------------------------------------------------*/
int sb_mixer_get_control_item_info(const sb_mixer_t *mixer, unsigned index,
    const char** name, unsigned *channels, unsigned *levels)
{
	assert(mixer);
	if (index > volume_table[mixer->type].count)
		return ENOENT;

	const volume_item_t item = volume_table[mixer->type].table[index];
	if (name)
		*name = item.description;
	if (channels)
		*channels = item.channels;
	if (levels)
		*levels = item.channel_table[0].volume_levels;
	return EOK;
}
/*----------------------------------------------------------------------------*/
int sb_mixer_set_volume_level(const sb_mixer_t *mixer,
    unsigned index, unsigned channel, unsigned level)
{
	if (mixer->type == SB_MIXER_UNKNOWN || mixer->type == SB_MIXER_NONE)
		return ENOTSUP;
	if (index >= volume_table[mixer->type].count)
		return ENOENT;
	if (channel >= volume_table[mixer->type].table[index].channels)
		return ENOENT;
	const channel_t chan =
	    volume_table[mixer->type].table[index].channel_table[channel];

	if (level > chan.volume_levels)
		level = chan.volume_levels;

	pio_write_8(&mixer->regs->mixer_address, chan.address);
	uint8_t value = 0;

	if (chan.preserve_bits) {
		value = pio_read_8(&mixer->regs->mixer_data);
		value &= ~(uint8_t)((chan.volume_levels - 1) << chan.shift);
	}

	value |= level << chan.shift;
	pio_write_8(&mixer->regs->mixer_data, value);
	return EOK;
}
/*----------------------------------------------------------------------------*/
unsigned sb_mixer_get_volume_level(const sb_mixer_t *mixer, unsigned index,
    unsigned channel)
{
	assert(mixer);
	if (mixer->type == SB_MIXER_UNKNOWN
	    || mixer->type == SB_MIXER_NONE
	    || (index >= volume_table[mixer->type].count)
	    || (channel >= volume_table[mixer->type].table[index].channels))
		return 0;

	const channel_t chan =
	    volume_table[mixer->type].table[index].channel_table[channel];
	pio_write_8(&mixer->regs->mixer_address, chan.address);
	return (pio_read_8(&mixer->regs->mixer_data) >> chan.shift)
	    & (chan.volume_levels - 1);
}
