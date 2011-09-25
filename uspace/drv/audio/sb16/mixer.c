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

#include "mixer.h"

#define CT_MIXER_RESET_ADDRESS 0x00

typedef struct volume_item {
	uint8_t address;
	uint8_t channels;
	const char *description;
	unsigned min_value;
	unsigned max_value;
	unsigned shift;
	bool same_reg;
} volume_item_t;

static const volume_item_t volume_ct1335[] = {
	{ 0x02, 1, "Master", 0, 7, 1, true },
	{ 0x06, 1, "MIDI", 0, 7, 1, true },
	{ 0x08, 1, "CD", 0, 7, 1, true },
	{ 0x0a, 1, "Voice", 0, 3, 1, true },
};

static const volume_item_t volume_ct1345[] = {
	{ 0x04, 2, "Voice", 0, 7, 1, true },
	{ 0x0a, 1, "Mic", 0, 3, 1, true },
	{ 0x22, 2, "Master", 0, 7, 1, true },
	{ 0x26, 2, "MIDI", 0, 7, 1, true },
	{ 0x28, 2, "CD", 0, 7, 1, true },
	{ 0x2e, 2, "Line", 0, 7, 1, true },
};

static const volume_item_t volume_ct1745[] = {
	{ 0x30, 2, "Master", 0, 31, 3, false },
	{ 0x32, 2, "Voice", 0, 31, 3, false },
	{ 0x34, 2, "MIDI", 0, 31, 3, false },
	{ 0x36, 2, "CD", 0, 31, 3, false },
	{ 0x38, 2, "Line", 0, 31, 3, false },
	{ 0x3a, 1, "Mic", 0, 31, 3, false },
	{ 0x3b, 1, "PC Speaker", 0, 3, 6, false },
	{ 0x3f, 2, "Input Gain", 0, 3, 6, false },
	{ 0x41, 2, "Output Gain", 0, 3, 6, false },
	{ 0x44, 2, "Treble", 0, 15, 4, false },
	{ 0x46, 2, "Bass", 0, 15, 4, false },
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
	/* Default values are ok for now */
}
/*----------------------------------------------------------------------------*/
void mixer_store_volume_levels(sb16_regs_t *regs, mixer_type_t type)
{
	/* Default values are ok for now */
}
/*----------------------------------------------------------------------------*/
int mixer_get_control_item_count(mixer_type_t type)
{
	return volume_table[type].count;
}
/*----------------------------------------------------------------------------*/
int mixer_get_control_item_info(mixer_type_t type, unsigned index,
    const char** name, unsigned *channels)
{
	if (index > volume_table[type].count)
		return ENOENT;

	if (name)
	    *name = volume_table[type].table[index].description;
	if (channels)
	    *channels = volume_table[type].table[index].channels;
	return EOK;
}
/*----------------------------------------------------------------------------*/
int mixer_set_volume_level(sb16_regs_t *regs, mixer_type_t type,
    unsigned item, unsigned channel, unsigned level)
{
	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
unsigned mixer_get_volume_level(sb16_regs_t *regs, mixer_type_t type,
    unsigned item, unsigned channel)
{
	return 0;
}
