/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio controller
 */

#ifndef CODEC_H
#define CODEC_H

#include "hdaudio.h"
#include "stream.h"

typedef struct hda_codec {
	hda_t *hda;
	uint8_t address;
	int out_aw;
	uint32_t out_aw_rates;
	uint32_t out_aw_formats;
	int out_aw_num;
	int out_aw_sel;
	int in_aw;
	uint32_t in_aw_rates;
	uint32_t in_aw_formats;
} hda_codec_t;

extern hda_codec_t *hda_codec_init(hda_t *, uint8_t);
extern void hda_codec_fini(hda_codec_t *);
extern errno_t hda_out_converter_setup(hda_codec_t *, hda_stream_t *);
extern errno_t hda_in_converter_setup(hda_codec_t *, hda_stream_t *);

#endif

/** @}
 */
