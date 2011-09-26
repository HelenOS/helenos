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

#define CT_MIXER_RESET_ADDRESS 0x00

typedef struct volume_item {
	uint8_t address;
	uint8_t channels;
	const char *description;
	unsigned volume_levels;
	unsigned shift;
	bool same_reg;
} volume_item_t;

static const volume_item_t volume_ct1335[] = {
	{ 0x02, 1, "Master", 8, 1, true },
	{ 0x06, 1, "MIDI", 8, 1, true },
	{ 0x08, 1, "CD", 8, 1, true },
	{ 0x0a, 1, "Voice", 4, 1, true },
};

static const volume_item_t volume_ct1345[] = {
	{ 0x22, 2, "Master", 8, 1, true },
	{ 0x04, 2, "Voice", 8, 1, true },
	{ 0x0a, 1, "Mic", 4, 1, true },
	{ 0x26, 2, "MIDI", 8, 1, true },
	{ 0x28, 2, "CD", 8, 1, true },
	{ 0x2e, 2, "Line", 8, 1, true },
};

static const volume_item_t volume_ct1745[] = {
	{ 0x30, 2, "Master", 32, 3, false },
	{ 0x32, 2, "Voice", 32, 3, false },
	{ 0x34, 2, "MIDI", 32, 3, false },
	{ 0x36, 2, "CD", 32, 3, false },
	{ 0x38, 2, "Line", 32, 3, false },
	{ 0x3a, 1, "Mic", 32, 3, false },
	{ 0x3b, 1, "PC Speaker", 4, 6, false },
	{ 0x3f, 2, "Input Gain", 4, 6, false },
	{ 0x41, 2, "Output Gain", 4, 6, false },
	{ 0x44, 2, "Treble", 16, 4, false },
	{ 0x46, 2, "Bass", 16, 4, false },
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

const char * mixer_type_to_str(mixer_type_t type)
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
int mixer_init(sb16_regs_t *regs, mixer_type_t type)
{
	if (type == SB_MIXER_UNKNOWN)
		return ENOTSUP;

	if (type != SB_MIXER_NONE) {
		pio_write_8(&regs->mixer_address, CT_MIXER_RESET_ADDRESS);
		pio_write_8(&regs->mixer_data, 1);
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
void mixer_load_volume_levels(sb16_regs_t *regs, mixer_type_t type)
{
	/* Set Master to maximum */
	if (!mixer_get_control_item_count(type))
		return;
	unsigned levels = 0, channels = 0, current_level;
	const char *name = NULL;
	mixer_get_control_item_info(type, 0, &name, &channels, &levels);
	unsigned channel = 0;
	for (;channel < channels; ++channel) {
		current_level =
		    mixer_get_volume_level(regs, type, 0, channel);
		ddf_log_note("Setting %s channel %d to %d (%d).\n",
		    name, channel, levels - 1, current_level);

		mixer_set_volume_level(regs, type, 0, channel, levels - 1);

		current_level =
		    mixer_get_volume_level(regs, type, 0, channel);
		ddf_log_note("%s channel %d set to %d.\n",
		    name, channel, current_level);
	}

}
/*----------------------------------------------------------------------------*/
void mixer_store_volume_levels(sb16_regs_t *regs, mixer_type_t type)
{
	/* No place to store the values. */
}
/*----------------------------------------------------------------------------*/
int mixer_get_control_item_count(mixer_type_t type)
{
	return volume_table[type].count;
}
/*----------------------------------------------------------------------------*/
int mixer_get_control_item_info(mixer_type_t type, unsigned index,
    const char** name, unsigned *channels, unsigned *levels)
{
	if (index > volume_table[type].count)
		return ENOENT;

	if (name)
		*name = volume_table[type].table[index].description;
	if (channels)
		*channels = volume_table[type].table[index].channels;
	if (levels)
		*levels = volume_table[type].table[index].volume_levels;
	return EOK;
}
/*----------------------------------------------------------------------------*/
int mixer_set_volume_level(sb16_regs_t *regs, mixer_type_t type,
    unsigned index, unsigned channel, unsigned level)
{
	if (type == SB_MIXER_UNKNOWN || type == SB_MIXER_NONE)
		return ENOTSUP;
	if (index >= volume_table[type].count)
		return ENOENT;
	if (level >= volume_table[type].table[index].volume_levels)
		return ENOTSUP;
	if (channel >= volume_table[type].table[index].channels)
		return ENOENT;

	const volume_item_t item = volume_table[type].table[index];
	const uint8_t address = item.address + (item.same_reg ? 0 : channel);
	pio_write_8(&regs->mixer_address, address);
	if (!item.same_reg) {
		const uint8_t value = level << item.shift;
		pio_write_8(&regs->mixer_data, value);
	} else {
		/* Nasty stuff */
		uint8_t value = pio_read_8(&regs->mixer_data);
		/* Remove value that is to be replaced register format is L:R*/
		value &= (channel ? 0xf0 : 0x0f);
		/* Add the new value */
		value |= (level << item.shift) << (channel ? 0 : 4);
		pio_write_8(&regs->mixer_data, value);
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
unsigned mixer_get_volume_level(sb16_regs_t *regs, mixer_type_t type,
    unsigned index, unsigned channel)
{
	if (type == SB_MIXER_UNKNOWN
	    || type == SB_MIXER_NONE
	    || (index >= volume_table[type].count)
	    || (channel >= volume_table[type].table[index].channels))
		return 0;

	const volume_item_t item = volume_table[type].table[index];
	const uint8_t address = item.address + (item.same_reg ? 0 : channel);
	pio_write_8(&regs->mixer_address, address);
	if (!item.same_reg) {
		return pio_read_8(&regs->mixer_data) >> item.shift;
	} else {
		const uint8_t value =
		    pio_read_8(&regs->mixer_data) >> (channel ? 0 : 4);
		return value >> item.shift;
	}
	return 0;
}
