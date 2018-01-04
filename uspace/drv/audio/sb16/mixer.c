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

#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#include <libarch/ddi.h>
#include <macros.h>

#include "ddf_log.h"
#include "mixer.h"

typedef struct channel {
	const char *name;
	uint8_t address;
	unsigned shift;
	unsigned volume_levels;
	bool preserve_bits;
} channel_t;

/* CT1335 channels */
static const channel_t channels_table_ct1335[] = {
	{ "Master", 0x02, 1, 8, false }, /* Master, Mono, 3bit volume level */
	{ "Midi", 0x06, 1, 8, false }, /* Midi, Mono, 3bit volume level */
	{ "CD", 0x08, 1, 8, false }, /* CD, Mono, 3bit volume level */
	{ "Voice", 0x0a, 1, 4, false }, /* Voice, Mono, 2bit volume level */
};

/* CT1345 channels */
static const channel_t channels_table_ct1345[] = {
	{ "Master Left", 0x22, 5, 8, true }, /* Master, Left, 3bit volume level */
	{ "Master Right", 0x22, 1, 8, true }, /* Master, Right, 3bit volume level */
	{ "MIDI Left", 0x26, 5, 8, true }, /* Midi, Left, 3bit volume level */
	{ "MIDI Right", 0x26, 1, 8, true }, /* Midi, Right, 3bit volume level */
	{ "CD Left", 0x28, 5, 8, true }, /* CD, Left, 3bit volume level */
	{ "CD Right", 0x28, 1, 8, true }, /* CD, Right, 3bit volume level */
	{ "Line In Left", 0x2e, 5, 8, true }, /* Line, Left, 3bit volume level */
	{ "Line In Right", 0x2e, 1, 8, true }, /* Line, Right, 3bit volume level */
	{ "Voice Left", 0x04, 5, 8, true }, /* Voice, Left, 3bit volume level */
	{ "Voice Right", 0x04, 1, 8, true }, /* Voice, Right, 3bit volume level */
	{ "Mic", 0x0a, 1, 4, false }, /* Mic, Mono, 2bit volume level */
};

/* CT1745 channels */
static const channel_t channels_table_ct1745[] = {
	{ "Master Left", 0x30, 3, 32, false },  /* Master, Left, 5bit volume level */
	{ "Master Right", 0x31, 3, 32, false }, /* Master, Right, 5bit volume level */
	{ "Voice Left", 0x32, 3, 32, false },  /* Voice, Left, 5bit volume level */
	{ "Voice Right", 0x33, 3, 32, false }, /* Voice, Right, 5bit volume level */
	{ "MIDI Left", 0x34, 3, 32, false }, /* MIDI, Left, 5bit volume level */
	{ "MIDI Right", 0x35, 3, 32, false }, /* MIDI, Right, 5bit volume level */
	{ "CD Left", 0x36, 3, 32, false }, /* CD, Left, 5bit volume level */
	{ "CD Right", 0x37, 3, 32, false }, /* CD, Right, 5bit volume level */
	{ "Line In Left", 0x38, 3, 32, false }, /* Line, Left, 5bit volume level */
	{ "Line In Right", 0x39, 3, 32, false }, /* Line, Right, 5bit volume level */
	{ "Mic", 0x3a, 3, 32, false }, /* Mic, Mono, 5bit volume level */
	{ "PC Speaker", 0x3b, 6, 4, false }, /* PC speaker, Mono, 2bit level */
	{ "Input Gain Left", 0x3f, 6, 4, false }, /* Input Gain, Left, 2bit level */
	{ "Input Gain Right", 0x40, 6, 4, false }, /* Input Gain, Right, 2bit level */
	{ "Output Gain Left", 0x41, 6, 4, false }, /* Output Gain, Left, 2bit level */
	{ "Output Gain Right", 0x42, 6, 4, false }, /* Output Gain, Right, 2bit level */
	{ "Treble Left", 0x44, 4, 16, false }, /* Treble, Left, 4bit volume level */
	{ "Treble Right", 0x45, 4, 16, false }, /* Treble, Right, 4bit volume level */
	{ "Bass Left", 0x46, 4, 16, false }, /* Bass, Left, 4bit volume level */
	{ "Bass Right", 0x47, 4, 16, false }, /* Bass, Right, 4bit volume level */
};

static const struct {
	const channel_t *table;
	size_t count;
} volume_table[] = {
	[SB_MIXER_NONE] = { NULL, 0 },
	[SB_MIXER_UNKNOWN] = { NULL, 0 },
	[SB_MIXER_CT1335] = {
	    channels_table_ct1335,
	    ARRAY_SIZE(channels_table_ct1335),
	},
	[SB_MIXER_CT1345] = {
	    channels_table_ct1345,
	    ARRAY_SIZE(channels_table_ct1345),
	},
	[SB_MIXER_CT1745] = {
	    channels_table_ct1745,
	    ARRAY_SIZE(channels_table_ct1745),
	},
};

const char * sb_mixer_type_str(sb_mixer_type_t type)
{
	static const char *names[] = {
		[SB_MIXER_NONE] = "No mixer",
		[SB_MIXER_CT1335] = "CT 1335",
		[SB_MIXER_CT1345] = "CT 1345",
		[SB_MIXER_CT1745] = "CT 1745",
		[SB_MIXER_UNKNOWN] = "Unknown mixer",
	};
	return names[type];
}

errno_t sb_mixer_init(sb_mixer_t *mixer, sb16_regs_t *regs, sb_mixer_type_t type)
{
	assert(mixer);
	mixer->regs = regs;
	mixer->type = type;
	if (type == SB_MIXER_UNKNOWN)
		return ENOTSUP;

	if (type != SB_MIXER_NONE) {
		pio_write_8(&regs->mixer_address, MIXER_RESET_ADDRESS);
		pio_write_8(&regs->mixer_data, 1);
	}
	pio_write_8(&regs->mixer_address, MIXER_PNP_IRQ_ADDRESS);
	const uint8_t irq = pio_read_8(&regs->mixer_data);
	pio_write_8(&regs->mixer_address, MIXER_PNP_DMA_ADDRESS);
	const uint8_t dma = pio_read_8(&regs->mixer_data);
	ddf_log_debug("SB16 setup with IRQ 0x%hhx and DMA 0x%hhx.", irq, dma);
	return EOK;
}

int sb_mixer_get_control_item_count(const sb_mixer_t *mixer)
{
	assert(mixer);
	return volume_table[mixer->type].count;
}

errno_t sb_mixer_get_control_item_info(const sb_mixer_t *mixer, unsigned item,
    const char** name, unsigned *levels)
{
	assert(mixer);
	if (item > volume_table[mixer->type].count)
		return ENOENT;

	const channel_t *ch = &volume_table[mixer->type].table[item];
	if (name)
		*name = ch->name;
	if (levels)
		*levels = ch->volume_levels;
	return EOK;
}

/**
 * Write new volume level from mixer registers.
 * @param mixer SB Mixer to use.
 * @param index Control item(channel) index.
 * @param value New volume level.
 * @return Error code.
 */
errno_t sb_mixer_get_control_item_value(const sb_mixer_t *mixer, unsigned item,
    unsigned *value)
{
	assert(mixer);
	if (!value)
		return EBADMEM;
	if (item > volume_table[mixer->type].count)
		return ENOENT;

	const channel_t *chan = &volume_table[mixer->type].table[item];
	pio_write_8(&mixer->regs->mixer_address, chan->address);
	*value = (pio_read_8(&mixer->regs->mixer_data) >> chan->shift)
	    & (chan->volume_levels - 1);
	return EOK;
}

/**
 * Write new volume level to mixer registers.
 * @param mixer SB Mixer to use.
 * @param index Control item(channel) index.
 * @param value New volume level.
 * @return Error code.
 */
errno_t sb_mixer_set_control_item_value(const sb_mixer_t *mixer, unsigned item,
    unsigned value)
{
	assert(mixer);
	if (item > volume_table[mixer->type].count)
		return ENOENT;

	const channel_t *chan = &volume_table[mixer->type].table[item];
	if (value >= chan->volume_levels)
		value = chan->volume_levels - 1;

	pio_write_8(&mixer->regs->mixer_address, chan->address);

	uint8_t regv = 0;
	if (chan->preserve_bits) {
		regv = pio_read_8(&mixer->regs->mixer_data);
		regv &= ~(uint8_t)((chan->volume_levels - 1) << chan->shift);
	}

	regv |= value << chan->shift;
	pio_write_8(&mixer->regs->mixer_data, regv);
	ddf_log_note("Item %s new value is: %u.",
	    volume_table[mixer->type].table[item].name, value);
	return EOK;
}
