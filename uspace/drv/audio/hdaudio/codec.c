/*
 * Copyright (c) 2022 Jiri Svoboda
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
#include <str_error.h>
#include <stdlib.h>

#include "codec.h"
#include "hdactl.h"
#include "spec/codec.h"
#include "spec/fmt.h"
#include "stream.h"

static errno_t hda_ccmd(hda_codec_t *codec, int node, uint32_t vid, uint32_t payload,
    uint32_t *resp)
{
	uint32_t verb;
	uint32_t myresp;

	if (resp == NULL)
		resp = &myresp;

	if ((vid & 0x700) != 0) {
		verb = (codec->address << 28) |
		    ((node & 0x1ff) << 20) |
		    ((vid & 0xfff) << 8) |
		    (payload & 0xff);
	} else {
		verb = (codec->address << 28) |
		    ((node & 0x1ff) << 20) |
		    ((vid & 0xf) << 16) |
		    (payload & 0xffff);
	}
	errno_t rc = hda_cmd(codec->hda, verb, resp);

#if 0
	if (resp != NULL) {
		ddf_msg(LVL_DEBUG, "verb 0x%" PRIx32 " -> 0x%" PRIx32, verb,
		    *resp);
	} else {
		ddf_msg(LVL_DEBUG, "verb 0x%" PRIx32, verb);
	}
#endif
	return rc;
}

static errno_t hda_get_parameter(hda_codec_t *codec, int node, hda_param_id_t param,
    uint32_t *resp)
{
	return hda_ccmd(codec, node, hda_param_get, param, resp);
}

static errno_t hda_get_subnc(hda_codec_t *codec, int node, int *startnode,
    int *nodecount)
{
	errno_t rc;
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
static errno_t hda_get_fgrp_type(hda_codec_t *codec, int node, bool *unsol,
    hda_fgrp_type_t *type)
{
	errno_t rc;
	uint32_t resp;

	rc = hda_get_parameter(codec, node, hda_fgrp_type, &resp);
	if (rc != EOK)
		return rc;

	*unsol = (resp & BIT_V(uint32_t, fgrpt_unsol)) != 0;
	*type = BIT_RANGE_EXTRACT(uint32_t, fgrpt_type_h, fgrpt_type_l, resp);

	return EOK;
}

static errno_t hda_get_clist_len(hda_codec_t *codec, int node, bool *longform,
    int *items)
{
	errno_t rc;
	uint32_t resp;

	rc = hda_get_parameter(codec, node, hda_clist_len, &resp);
	if (rc != EOK)
		return rc;

	ddf_msg(LVL_DEBUG2, "hda_get_clist_len: resp=0x%x", resp);
	*longform = resp & BIT_V(uint32_t, cll_longform);
	*items = resp & BIT_RANGE_EXTRACT(uint32_t, cll_len_h, cll_len_l, resp);
	return EOK;
}

static errno_t hda_get_clist_entry(hda_codec_t *codec, int node, int n, uint32_t *resp)
{
	return hda_ccmd(codec, node, hda_clist_entry_get, n, resp);
}

static errno_t hda_get_eapd_btl_enable(hda_codec_t *codec, int node, uint32_t *resp)
{
	return hda_ccmd(codec, node, hda_eapd_btl_enable_get, 0, resp);
}

static errno_t hda_set_eapd_btl_enable(hda_codec_t *codec, int node, uint8_t payload)
{
	return hda_ccmd(codec, node, hda_eapd_btl_enable_set, payload, NULL);
}

/** Get Suppported PCM Size, Rates */
static errno_t hda_get_supp_rates(hda_codec_t *codec, int node, uint32_t *rates)
{
	return hda_get_parameter(codec, node, hda_supp_rates, rates);
}

/** Get Suppported Stream Formats */
static errno_t hda_get_supp_formats(hda_codec_t *codec, int node, uint32_t *fmts)
{
	return hda_get_parameter(codec, node, hda_supp_formats, fmts);
}

static errno_t hda_set_converter_fmt(hda_codec_t *codec, int node, uint16_t fmt)
{
	return hda_ccmd(codec, node, hda_converter_fmt_set, fmt, NULL);
}

static errno_t hda_set_converter_ctl(hda_codec_t *codec, int node, uint8_t stream,
    uint8_t channel)
{
	uint32_t ctl;

	ctl = (stream << cctl_stream_l) | (channel << cctl_channel_l);
	return hda_ccmd(codec, node, hda_converter_ctl_set, ctl, NULL);
}

static errno_t hda_set_pin_ctl(hda_codec_t *codec, int node, uint8_t pctl)
{
	return hda_ccmd(codec, node, hda_pin_ctl_set, pctl, NULL);
}

static errno_t hda_get_pin_ctl(hda_codec_t *codec, int node, uint8_t *pctl)
{
	errno_t rc;
	uint32_t resp;

	rc = hda_ccmd(codec, node, hda_pin_ctl_get, 0, &resp);
	if (rc != EOK)
		return rc;

	*pctl = resp;
	return EOK;
}

/** Get Audio Widget Capabilities */
static errno_t hda_get_aw_caps(hda_codec_t *codec, int node,
    hda_awidget_type_t *type, uint32_t *caps)
{
	errno_t rc;
	uint32_t resp;

	rc = hda_get_parameter(codec, node, hda_aw_caps, &resp);
	if (rc != EOK)
		return rc;

	*type = BIT_RANGE_EXTRACT(uint32_t, awc_type_h, awc_type_l, resp);
	*caps = resp;

	return EOK;
}

/** Get Pin Capabilities */
static errno_t hda_get_pin_caps(hda_codec_t *codec, int node, uint32_t *caps)
{
	return hda_get_parameter(codec, node, hda_pin_caps, caps);
}

/** Get Power State */
static errno_t hda_get_power_state(hda_codec_t *codec, int node, uint32_t *pstate)
{
	return hda_ccmd(codec, node, hda_power_state_get, 0, pstate);
}

/** Get Configuration Default */
static errno_t hda_get_cfg_def(hda_codec_t *codec, int node, uint32_t *cfgdef)
{
	return hda_ccmd(codec, node, hda_cfg_def_get, 0, cfgdef);
}

static errno_t hda_get_conn_sel(hda_codec_t *codec, int node, uint32_t *conn)
{
	return hda_ccmd(codec, node, hda_conn_sel_get, 0, conn);
}

/** Get Amplifier Gain / Mute  */
static errno_t hda_get_amp_gain_mute(hda_codec_t *codec, int node, uint16_t payload,
    uint32_t *resp)
{
	ddf_msg(LVL_DEBUG2, "hda_get_amp_gain_mute(codec, %d, %x)",
	    node, payload);
	errno_t rc = hda_ccmd(codec, node, hda_amp_gain_mute_get, payload, resp);
	ddf_msg(LVL_DEBUG2, "hda_get_amp_gain_mute(codec, %d, %x, resp=%x)",
	    node, payload, *resp);
	return rc;
}

/** Get GP I/O Count */
static errno_t hda_get_gpio_cnt(hda_codec_t *codec, int node, uint32_t *resp)
{
	return hda_get_parameter(codec, node, hda_gpio_cnt, resp);
}

static errno_t hda_set_amp_gain_mute(hda_codec_t *codec, int node, uint16_t payload)
{
	ddf_msg(LVL_DEBUG2, "hda_set_amp_gain_mute(codec, %d, %x)",
	    node, payload);
	return hda_ccmd(codec, node, hda_amp_gain_mute_set, payload, NULL);
}

static errno_t hda_set_out_amp_max(hda_codec_t *codec, uint8_t aw)
{
	uint32_t ampcaps;
	uint32_t gmleft, gmright;
	uint32_t offset;
	errno_t rc;

	rc = hda_get_parameter(codec, aw,
	    hda_out_amp_caps, &ampcaps);
	if (rc != EOK)
		goto error;

	offset = ampcaps & 0x7f;
	ddf_msg(LVL_DEBUG, "out amp caps 0x%x (offset=0x%x)",
	    ampcaps, offset);

	rc = hda_set_amp_gain_mute(codec, aw, 0xb000 + offset);
	if (rc != EOK)
		goto error;

	rc = hda_get_amp_gain_mute(codec, aw, 0x8000, &gmleft);
	if (rc != EOK)
		goto error;

	rc = hda_get_amp_gain_mute(codec, aw, 0xa000, &gmright);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_DEBUG, "gain/mute: L:0x%x R:0x%x", gmleft, gmright);

	return EOK;
error:
	return rc;
}

static errno_t hda_set_in_amp_max(hda_codec_t *codec, uint8_t aw)
{
	uint32_t ampcaps;
	uint32_t gmleft, gmright;
	uint32_t offset;
	int i;
	errno_t rc;

	rc = hda_get_parameter(codec, aw,
	    hda_out_amp_caps, &ampcaps);
	if (rc != EOK)
		goto error;

	offset = ampcaps & 0x7f;
	ddf_msg(LVL_DEBUG, "in amp caps 0x%x (offset=0x%x)", ampcaps, offset);

	for (i = 0; i < 15; i++) {
		rc = hda_set_amp_gain_mute(codec, aw, 0x7000 + (i << 8) + offset);
		if (rc != EOK)
			goto error;

		rc = hda_get_amp_gain_mute(codec, aw, 0x0000 + i, &gmleft);
		if (rc != EOK)
			goto error;

		rc = hda_get_amp_gain_mute(codec, aw, 0x2000 + i, &gmright);
		if (rc != EOK)
			goto error;

		ddf_msg(LVL_DEBUG, "in:%d gain/mute: L:0x%x R:0x%x",
		    i, gmleft, gmright);
	}

	return EOK;
error:
	return rc;
}

static errno_t hda_clist_dump(hda_codec_t *codec, uint8_t aw)
{
	errno_t rc;
	bool longform;
	int len;
	uint32_t resp;
	uint32_t mask;
	uint32_t cidx;
	int shift;
	int epresp;
	int i, j;

	ddf_msg(LVL_DEBUG, "Connections for widget %d:", aw);

	rc = hda_get_clist_len(codec, aw, &longform, &len);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed getting connection list length.");
		return rc;
	}

	if (len > 1) {
		rc = hda_get_conn_sel(codec, aw, &cidx);
		if (rc != EOK) {
			ddf_msg(LVL_ERROR, "Failed getting connection select");
			return rc;
		}
	} else {
		cidx = 0;
	}

	ddf_msg(LVL_DEBUG2, "longform:%d len:%d", longform, len);

	if (longform) {
		epresp = 2;
		mask = 0xffff;
		shift = 16;
	} else {
		epresp = 4;
		mask = 0xff;
		shift = 8;
	}

	i = 0;
	while (i < len) {
		rc = hda_get_clist_entry(codec, aw, i, &resp);
		if (rc != EOK) {
			ddf_msg(LVL_ERROR, "Failed getting connection list entry.");
			return rc;
		}

		for (j = 0; j < epresp && i < len; j++) {
			ddf_msg(LVL_DEBUG, "<- %d%s", resp & mask,
			    (int)cidx == i ? " *** current *** " : "");
			resp = resp >> shift;
			++i;
		}

	}

	return rc;
}

static errno_t hda_pin_init(hda_codec_t *codec, uint8_t aw)
{
	errno_t rc;
	uint32_t cfgdef;
	uint32_t pcaps;
	uint32_t eapd;
	uint8_t pctl;

	rc = hda_get_cfg_def(codec, aw, &cfgdef);
	if (rc != EOK)
		goto error;
	ddf_msg(LVL_DEBUG, "aw %d: PIN cdfgef=0x%x",
	    aw, cfgdef);

	rc = hda_get_pin_caps(codec, aw, &pcaps);
	if (rc != EOK)
		goto error;
	ddf_msg(LVL_DEBUG, "aw %d : PIN caps=0x%x",
	    aw, pcaps);

	if ((pcaps & BIT_V(uint32_t, pwc_eapd)) != 0) {
		rc = hda_get_eapd_btl_enable(codec, aw, &eapd);
		if (rc != EOK)
			goto error;

		ddf_msg(LVL_DEBUG, "PIN %d had EAPD value=0x%x", aw, eapd);

		rc = hda_set_eapd_btl_enable(codec, aw, eapd | 2);
		if (rc != EOK)
			goto error;

		rc = hda_get_eapd_btl_enable(codec, aw, &eapd);
		if (rc != EOK)
			goto error;

		ddf_msg(LVL_DEBUG, "PIN %d now has EAPD value=0x%x", aw, eapd);
	}

	pctl = 0;
	if ((pcaps & BIT_V(uint32_t, pwc_output)) != 0) {
		ddf_msg(LVL_DEBUG, "PIN %d will enable output", aw);
		pctl = pctl | BIT_V(uint8_t, pctl_out_enable);
	}

	if ((pcaps & BIT_V(uint32_t, pwc_input)) != 0) {
		ddf_msg(LVL_DEBUG, "PIN %d will enable input", aw);
		pctl = pctl | BIT_V(uint8_t, pctl_in_enable);
	}

	if ((pcaps & BIT_V(uint32_t, pwc_hpd)) != 0) {
		ddf_msg(LVL_DEBUG, "PIN %d will enable headphone drive", aw);
		pctl = pctl | BIT_V(uint8_t, pctl_hpd_enable);
	}

#if 0
	if ((pcaps & BIT_V(uint32_t, pwc_input)) != 0) {
		ddf_msg(LVL_DEBUG, "PIN %d will enable input");
		pctl = pctl | BIT_V(uint8_t, pctl_input_enable);
	}
#endif
	ddf_msg(LVL_DEBUG, "Setting PIN %d ctl to 0x%x", aw, pctl);
	rc = hda_set_pin_ctl(codec, aw, pctl);
	if (rc != EOK)
		goto error;

	pctl = 0;
	rc = hda_get_pin_ctl(codec, aw, &pctl);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_DEBUG, "PIN %d ctl reads as 0x%x", aw, pctl);

	return EOK;
error:
	return rc;
}

/** Init power-control in wiget capable of doing so. */
static errno_t hda_power_ctl_init(hda_codec_t *codec, uint8_t aw)
{
	errno_t rc;
	uint32_t pwrstate;

	ddf_msg(LVL_DEBUG, "aw %d is power control-capable", aw);

	rc = hda_get_power_state(codec, aw, &pwrstate);
	if (rc != EOK)
		goto error;
	ddf_msg(LVL_DEBUG, "aw %d: power state = 0x%x", aw, pwrstate);

	return EOK;
error:
	return rc;
}

hda_codec_t *hda_codec_init(hda_t *hda, uint8_t address)
{
	hda_codec_t *codec;
	errno_t rc;
	int sfg, nfg;
	int saw, naw;
	int fg, aw;
	bool unsol;
	hda_fgrp_type_t grptype;
	hda_awidget_type_t awtype;
	uint32_t awcaps;
	uint32_t rates;
	uint32_t formats;
	uint32_t gpio;

	codec = calloc(1, sizeof(hda_codec_t));
	if (codec == NULL)
		return NULL;

	codec->hda = hda;
	codec->address = address;
	codec->in_aw = -1;
	codec->out_aw = -1;

	rc = hda_get_subnc(codec, 0, &sfg, &nfg);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_DEBUG, "hda_get_subnc -> %s", str_error_name(rc));
	ddf_msg(LVL_DEBUG, "sfg=%d nfg=%d", sfg, nfg);

	for (fg = sfg; fg < sfg + nfg; fg++) {
		ddf_msg(LVL_DEBUG, "Enumerate FG %d", fg);

		rc = hda_get_fgrp_type(codec, fg, &unsol, &grptype);
		if (rc != EOK)
			goto error;

		ddf_msg(LVL_DEBUG, "hda_get_fgrp_type -> %s", str_error_name(rc));
		ddf_msg(LVL_DEBUG, "unsol: %d, grptype: %d", unsol, grptype);

		rc = hda_get_gpio_cnt(codec, fg, &gpio);
		if (rc != EOK)
			goto error;

		ddf_msg(LVL_DEBUG, "GPIO: wake=%d unsol=%d gpis=%d gpos=%d gpios=%d",
		    (gpio & BIT_V(uint32_t, 31)) != 0,
		    (gpio & BIT_V(uint32_t, 30)) != 0,
		    BIT_RANGE_EXTRACT(uint32_t, 23, 16, gpio),
		    BIT_RANGE_EXTRACT(uint32_t, 15, 8, gpio),
		    BIT_RANGE_EXTRACT(uint32_t, 7, 0, gpio));

		rc = hda_power_ctl_init(codec, fg);
		if (rc != EOK)
			goto error;

		rc = hda_get_subnc(codec, fg, &saw, &naw);
		if (rc != EOK)
			goto error;

		ddf_msg(LVL_DEBUG, "hda_get_subnc -> %s", str_error_name(rc));
		ddf_msg(LVL_DEBUG, "saw=%d baw=%d", saw, naw);

		for (aw = saw; aw < saw + naw; aw++) {
			rc = hda_get_aw_caps(codec, aw, &awtype, &awcaps);
			if (rc != EOK)
				goto error;
			ddf_msg(LVL_DEBUG, "aw %d: type=0x%x caps=0x%x",
			    aw, awtype, awcaps);

			if ((awcaps & BIT_V(uint32_t, awc_power_cntrl)) != 0) {
				rc = hda_power_ctl_init(codec, aw);
				if (rc != EOK)
					goto error;
			}

			switch (awtype) {
			case awt_audio_input:
			case awt_audio_mixer:
			case awt_audio_selector:
			case awt_pin_complex:
			case awt_power_widget:
				rc = hda_clist_dump(codec, aw);
				if (rc != EOK)
					goto error;
				break;
			default:
				break;
			}

			if (awtype == awt_pin_complex) {
				rc = hda_pin_init(codec, aw);
				if (rc != EOK)
					goto error;
			} else if (awtype == awt_audio_output) {
				rc = hda_get_supp_rates(codec, aw, &rates);
				if (rc != EOK)
					goto error;

				rc = hda_get_supp_formats(codec, aw, &formats);
				if (rc != EOK)
					goto error;

				if (rates != 0 && formats != 0 &&
				    codec->out_aw < 0) {
					ddf_msg(LVL_DEBUG, "Selected output "
					    "widget %d\n", aw);
					codec->out_aw = aw;
				} else {
					ddf_msg(LVL_DEBUG, "Ignoring output "
					    "widget %d\n", aw);
				}

				ddf_msg(LVL_NOTE, "Output widget %d: rates=0x%x formats=0x%x",
				    aw, rates, formats);
				codec->out_aw_rates = rates;
				codec->out_aw_formats = formats;
			} else if (awtype == awt_audio_input) {
				if (codec->in_aw < 0) {
					ddf_msg(LVL_DEBUG, "Selected input "
					    "widget %d\n", aw);
					codec->in_aw = aw;
				} else {
					ddf_msg(LVL_DEBUG, "Ignoring input "
					    "widget %d\n", aw);
				}

				rc = hda_get_supp_rates(codec, aw, &rates);
				if (rc != EOK)
					goto error;

				rc = hda_get_supp_formats(codec, aw, &formats);
				if (rc != EOK)
					goto error;

				ddf_msg(LVL_DEBUG, "Input widget %d: rates=0x%x formats=0x%x",
				    aw, rates, formats);
				codec->in_aw_rates = rates;
				codec->in_aw_formats = formats;
			}

			if ((awcaps & BIT_V(uint32_t, awc_out_amp_present)) != 0)
				hda_set_out_amp_max(codec, aw);

			if ((awcaps & BIT_V(uint32_t, awc_in_amp_present)) != 0)
				hda_set_in_amp_max(codec, aw);
		}
	}

	hda_ctl_dump_info(hda->ctl);

	ddf_msg(LVL_DEBUG, "Codec OK");
	return codec;
error:
	free(codec);
	return NULL;
}

void hda_codec_fini(hda_codec_t *codec)
{
	ddf_msg(LVL_DEBUG, "hda_codec_fini()");
	free(codec);
}

errno_t hda_out_converter_setup(hda_codec_t *codec, hda_stream_t *stream)
{
	errno_t rc;

	/* Configure converter */
	ddf_msg(LVL_DEBUG, "Configure output converter format / %d",
	    codec->out_aw);
	rc = hda_set_converter_fmt(codec, codec->out_aw, stream->fmt);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_DEBUG, "Configure output converter stream, channel");
	rc = hda_set_converter_ctl(codec, codec->out_aw, stream->sid, 0);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

errno_t hda_in_converter_setup(hda_codec_t *codec, hda_stream_t *stream)
{
	errno_t rc;

	/* Configure converter */

	ddf_msg(LVL_DEBUG, "Configure input converter format");
	rc = hda_set_converter_fmt(codec, codec->in_aw, stream->fmt);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_DEBUG, "Configure input converter stream, channel");
	rc = hda_set_converter_ctl(codec, codec->in_aw, stream->sid, 0);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	return rc;
}

/** @}
 */
