/*
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio codec
 */

#include <async.h>
#include <bitops.h>
#include <ddf/log.h>
#include <errno.h>
#include <stdlib.h>

#include "codec.h"
#include "hdactl.h"
#include "spec/codec.h"
#include "spec/fmt.h"
#include "stream.h"

static int hda_ccmd(hda_codec_t *codec, int node, uint32_t vid, uint32_t payload,
    uint32_t *resp)
{
	uint32_t verb;

	verb = (codec->address << 28) | (node << 20) | (vid << 8) | payload;
	return hda_cmd(codec->hda, verb, resp);
}

static int hda_get_parameter(hda_codec_t *codec, int node, hda_param_id_t param,
    uint32_t *resp)
{
	return hda_ccmd(codec, node, hda_param_get, param, resp);
}

static int hda_get_subnc(hda_codec_t *codec, int node, int *startnode,
    int *nodecount)
{
	int rc;
	uint32_t resp;

	rc = hda_get_parameter(codec, node, hda_sub_nc, &resp);
	if (rc != EOK)
		return rc;

	*startnode = BIT_RANGE_EXTRACT(uint32_t, subnc_startnode_h,
	    subnc_startnode_l, resp);
	*nodecount = BIT_RANGE_EXTRACT(uint32_t, subnc_nodecount_h,
	    subnc_nodecount_l, resp);

	return EOK;
}

/** Get Function Group Type */
static int hda_get_fgrp_type(hda_codec_t *codec, int node, bool *unsol,
    hda_fgrp_type_t *type)
{
	int rc;
	uint32_t resp;

	rc = hda_get_parameter(codec, node, hda_fgrp_type, &resp);
	if (rc != EOK)
		return rc;

	*unsol = (resp & BIT_V(uint32_t, fgrpt_unsol)) != 0;
	*type = BIT_RANGE_EXTRACT(uint32_t, fgrpt_type_h, fgrpt_type_l, resp);

	return EOK;
}

/** Get Suppported PCM Size, Rates */
static int hda_get_supp_rates(hda_codec_t *codec, int node, uint32_t *rates)
{
	return hda_get_parameter(codec, node, hda_supp_rates, rates);
}

/** Get Suppported Stream Formats */
static int hda_get_supp_formats(hda_codec_t *codec, int node, uint32_t *fmts)
{
	return hda_get_parameter(codec, node, hda_supp_formats, fmts);
}

static int hda_set_converter_fmt(hda_codec_t *codec, int node, uint16_t fmt)
{
	return hda_ccmd(codec, node, hda_converter_fmt_set, fmt, NULL);
}

static int hda_set_converter_ctl(hda_codec_t *codec, int node, uint8_t stream,
    uint8_t channel)
{
	uint32_t ctl;

	ctl = (stream << cctl_stream_l) | (channel << cctl_channel_l);
	return hda_ccmd(codec, node, hda_converter_ctl_set, ctl, NULL);
}

/** Get Audio Widget Capabilities */
static int hda_get_aw_caps(hda_codec_t *codec, int node,
    hda_awidget_type_t *type, uint32_t *caps)
{
	int rc;
	uint32_t resp;

	rc = hda_get_parameter(codec, node, hda_aw_caps, &resp);
	if (rc != EOK)
		return rc;

	*type = BIT_RANGE_EXTRACT(uint32_t, awc_type_h, awc_type_l, resp);
	*caps = resp;

	return EOK;
}

/** Get Configuration Default */
static int hda_get_cfg_def(hda_codec_t *codec, int node, uint32_t *cfgdef)
{
	return hda_ccmd(codec, node, hda_cfg_def_get, 0, cfgdef);
}

/** Get Amplifier Gain / Mute  */
static int hda_get_amp_gain_mute(hda_codec_t *codec, int node, uint16_t payload,
    uint32_t *resp)
{
	return hda_ccmd(codec, node, hda_amp_gain_mute_get, payload, resp);
}

static int hda_set_amp_gain_mute(hda_codec_t *codec, int node, uint16_t payload)
{
	return hda_ccmd(codec, node, hda_amp_gain_mute_set, payload, NULL);
}

hda_codec_t *hda_codec_init(hda_t *hda, uint8_t address)
{
	hda_codec_t *codec;
	int rc;
	int sfg, nfg;
	int saw, naw;
	int fg, aw;
	bool unsol;
	hda_fgrp_type_t grptype;
	hda_awidget_type_t awtype;
	uint32_t awcaps;
	uint32_t cfgdef;
	uint32_t rates;
	uint32_t formats;

	codec = calloc(1, sizeof(hda_codec_t));
	if (codec == NULL)
		return NULL;

	codec->hda = hda;
	codec->address = address;

	rc = hda_get_subnc(codec, 0, &sfg, &nfg);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_NOTE, "hda_get_subnc -> %d", rc);
	ddf_msg(LVL_NOTE, "sfg=%d nfg=%d", sfg, nfg);

	for (fg = sfg; fg < sfg + nfg; fg++) {
		ddf_msg(LVL_NOTE, "Enumerate FG %d", fg);

		rc = hda_get_fgrp_type(codec, fg, &unsol, &grptype);
		if (rc != EOK)
			goto error;

		ddf_msg(LVL_NOTE, "hda_get_fgrp_type -> %d", rc);
		ddf_msg(LVL_NOTE, "unsol: %d, grptype: %d", unsol, grptype);

		rc = hda_get_subnc(codec, fg, &saw, &naw);
		if (rc != EOK)
			goto error;

		ddf_msg(LVL_NOTE, "hda_get_subnc -> %d", rc);
		ddf_msg(LVL_NOTE, "saw=%d baw=%d", saw, naw);

		for (aw = saw; aw < saw + naw; aw++) {
			rc = hda_get_aw_caps(codec, aw, &awtype, &awcaps);
			if (rc != EOK)
				goto error;
			ddf_msg(LVL_NOTE, "aw %d: type=0x%x caps=0x%x",
			    aw, awtype, awcaps);

			if (awtype == awt_pin_complex) {
				rc = hda_get_cfg_def(codec, aw, &cfgdef);
				if (rc != EOK)
					goto error;
				ddf_msg(LVL_NOTE, "aw %d: PIN cdfgef=0x%x",
				    aw, cfgdef);

			} else if (awtype == awt_audio_output) {
				codec->out_aw = aw;
			}

			if ((awcaps & BIT_V(uint32_t, awc_out_amp_present)) != 0) {
				uint32_t ampcaps;
				uint32_t gmleft, gmright;

				rc = hda_get_parameter(codec, aw,
				    hda_out_amp_caps, &ampcaps);
				if (rc != EOK)
					goto error;

				rc = hda_get_amp_gain_mute(codec, aw, 0x8000, &gmleft);
				if (rc != EOK)
					goto error;

				rc = hda_get_amp_gain_mute(codec, aw, 0xc000, &gmright);
				if (rc != EOK)
					goto error;

				ddf_msg(LVL_NOTE, "out amp caps 0x%x "
				    "gain/mute: L:0x%x R:0x%x",
				    ampcaps, gmleft, gmright);

				rc = hda_set_amp_gain_mute(codec, aw, 0xb04a);
				if (rc != EOK)
					goto error;
			}
		}
	}

	rc = hda_get_supp_rates(codec, codec->out_aw, &rates);
	if (rc != EOK)
		goto error;

	rc = hda_get_supp_formats(codec, codec->out_aw, &formats);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_NOTE, "Output widget %d: rates=0x%x formats=0x%x",
	    codec->out_aw, rates, formats);

	/* XXX Choose appropriate parameters */
	uint32_t fmt;
	/* 48 kHz, 16-bits, 1 channel */
	fmt = fmt_bits_16 << fmt_bits_l;

	/* Create stream */
	ddf_msg(LVL_NOTE, "Create stream");
	hda_stream_t *stream;
	stream = hda_stream_create(hda, sdir_output, fmt);
	if (stream == NULL)
		goto error;

	/* Configure converter */

	ddf_msg(LVL_NOTE, "Configure converter format");
	rc = hda_set_converter_fmt(codec, codec->out_aw, fmt);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_NOTE, "Configure converter stream, channel");
	rc = hda_set_converter_ctl(codec, codec->out_aw, stream->sid, 0);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_NOTE, "Start stream");
	hda_stream_start(stream);

	ddf_msg(LVL_NOTE, "Codec OK");
	return codec;
error:
	free(codec);
	return NULL;
}

void hda_codec_fini(hda_codec_t *codec)
{
	ddf_msg(LVL_NOTE, "hda_codec_fini()");
	free(codec);
}

/** @}
 */
