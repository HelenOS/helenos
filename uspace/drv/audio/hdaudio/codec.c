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

static int hda_get_parameter(hda_codec_t *codec, int node, hda_param_id_t param,
    uint32_t *resp)
{
	uint32_t verb;

	verb = (codec->address << 28) | (node << 20) | ((hda_get_param) << 8) | param;
	return hda_cmd(codec->hda, verb, resp);
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
		}
	}

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
