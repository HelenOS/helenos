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
/** @file High Definition Audio controller
 */

#include <ddf/log.h>
#include <stdint.h>

#include "hdactl.h"
#include "regif.h"
#include "hdaudio_regs.h"

hda_ctl_t *hda_ctl_init(hda_t *hda)
{
	hda_ctl_t *ctl;

	ctl = calloc(1, sizeof(hda_ctl_t));
	if (ctl == NULL)
		return NULL;

	uint8_t vmaj = hda_reg8_read(&hda->regs->vmaj);
	uint8_t vmin = hda_reg8_read(&hda->regs->vmin);
	ddf_msg(LVL_NOTE, "HDA version %d.%d", vmaj, vmin);

	if (vmaj != 1 || vmin != 0) {
		ddf_msg(LVL_ERROR, "Unsupported HDA version (%d.%d).",
		    vmaj, vmin);
		goto error;
	}

	return ctl;
error:
	free(ctl);
	return NULL;
}

void hda_ctl_fini(hda_ctl_t *ctl)
{
	ddf_msg(LVL_NOTE, "hda_ctl_fini()");
	free(ctl);
}

/** @}
 */
