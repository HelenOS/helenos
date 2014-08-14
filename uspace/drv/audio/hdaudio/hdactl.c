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

#include <as.h>
#include <async.h>
#include <bitops.h>
#include <ddf/log.h>
#include <ddi.h>
#include <errno.h>
#include <macros.h>
#include <stdint.h>

#include "hdactl.h"
#include "regif.h"
#include "spec/regs.h"

enum {
	ctrl_init_wait_max = 10,
	codec_enum_wait_us = 512
};

/** Select an appropriate CORB/RIRB size.
 *
 * We always use the largest available size. In @a sizecap each of bits
 * 0, 1, 2 determine whether one of the supported size (0 == 2 enries,
 * 1 == 16 entries, 2 == 256 entries) is supported. @a *selsz is set to
 * one of 0, 1, 2 on success.
 *
 * @param sizecap	CORB/RIRB Size Capability
 * @param selsz		Place to store CORB/RIRB Size
 * @return		EOK on success, EINVAL if sizecap has no valid bits set
 *
 */
static int hda_rb_size_select(uint8_t sizecap, uint8_t *selsz)
{
	int i;

	for (i = 2; i >= 0; --i) {
		if ((sizecap & BIT_V(uint8_t, i)) != 0) {
			*selsz = i;
			return EOK;
		}
	}

	return EINVAL;
}

static size_t hda_rb_entries(uint8_t selsz)
{
	switch (selsz) {
	case 0:
		return 2;
	case 1:
		return 16;
	case 2:
		return 256;
	default:
		assert(false);
		return 0;
	}
}

/** Initialize the CORB */
static int hda_corb_init(hda_t *hda)
{
	uint8_t ctl;
	uint8_t corbsz;
	uint8_t sizecap;
	uint8_t selsz;
	bool ok64bit;
	int rc;

	ddf_msg(LVL_NOTE, "hda_corb_init()");

	/* Stop CORB if not stopped. */
	ctl = hda_reg8_read(&hda->regs->corbctl);
	if ((ctl & BIT_V(uint8_t, corbctl_run)) != 0) {
		ddf_msg(LVL_NOTE, "CORB is enabled, disabling first.");
		hda_reg8_write(&hda->regs->corbctl, ctl & ~BIT_V(uint8_t,
		    corbctl_run));
	}

	/* Determine CORB size and allocate CORB buffer */
	corbsz = hda_reg8_read(&hda->regs->corbsize);
	sizecap = BIT_RANGE_EXTRACT(uint8_t, corbsize_cap_h,
	    corbsize_cap_l, corbsz);
	rc = hda_rb_size_select(sizecap, &selsz);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Invalid CORB Size Capability");
		goto error;
	}
	corbsz = corbsz & ~BIT_RANGE(uint8_t, corbsize_size_h, corbsize_size_l);
	corbsz = corbsz | selsz;

	ddf_msg(LVL_NOTE, "Setting CORB Size register to 0x%x", corbsz);
	hda_reg8_write(&hda->regs->corbsize, corbsz);
	hda->ctl->corb_entries = hda_rb_entries(selsz);

	ddf_msg(LVL_NOTE, "Read GCAP");
	uint16_t gcap = hda_reg16_read(&hda->regs->gcap);
	ok64bit = (gcap & BIT_V(uint8_t, gcap_64ok)) != 0;
	ddf_msg(LVL_NOTE, "GCAP: 0x%x (64OK=%d)", gcap, ok64bit);

	/*
	 * CORB must be aligned to 128 bytes. If 64OK is not set,
	 * it must be within the 32-bit address space.
	 */
	hda->ctl->corb_virt = AS_AREA_ANY;
	rc = dmamem_map_anonymous(hda->ctl->corb_entries * sizeof(uint32_t),
	    ok64bit ? 0 : DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &hda->ctl->corb_phys, &hda->ctl->corb_virt);

	ddf_msg(LVL_NOTE, "Set CORB base registers");

	/* Update CORB base registers */
	hda_reg32_write(&hda->regs->corblbase, LOWER32(hda->ctl->corb_phys));
	hda_reg32_write(&hda->regs->corbubase, UPPER32(hda->ctl->corb_phys));

	ddf_msg(LVL_NOTE, "Rset CORB Read/Write pointers");

	/* Reset CORB Read Pointer */
	hda_reg16_write(&hda->regs->corbrp, BIT_V(uint16_t, corbrp_rst));

	/* Reset CORB Write Poitner */
	hda_reg16_write(&hda->regs->corbwp, 0);

	ddf_msg(LVL_NOTE, "CORB initialized");
	return EOK;
error:
	return EIO;
}

/** Initialize the RIRB */
static int hda_rirb_init(hda_t *hda)
{
	uint8_t ctl;
	uint8_t rirbsz;
	uint8_t sizecap;
	uint8_t selsz;
	bool ok64bit;
	int rc;

	ddf_msg(LVL_NOTE, "hda_rirb_init()");

	/* Stop RIRB if not stopped. */
	ctl = hda_reg8_read(&hda->regs->rirbctl);
	if ((ctl & BIT_V(uint8_t, rirbctl_run)) != 0) {
		ddf_msg(LVL_NOTE, "RIRB is enabled, disabling first.");
		hda_reg8_write(&hda->regs->corbctl, ctl & ~BIT_V(uint8_t,
		    rirbctl_run));
	}

	/* Determine RIRB size and allocate RIRB buffer */
	rirbsz = hda_reg8_read(&hda->regs->rirbsize);
	sizecap = BIT_RANGE_EXTRACT(uint8_t, rirbsize_cap_h,
	    rirbsize_cap_l, rirbsz);
	rc = hda_rb_size_select(sizecap, &selsz);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Invalid RIRB Size Capability");
		goto error;
	}
	rirbsz = rirbsz & ~BIT_RANGE(uint8_t, rirbsize_size_h, rirbsize_size_l);
	rirbsz = rirbsz | selsz;

	ddf_msg(LVL_NOTE, "Setting RIRB Size register to 0x%x", rirbsz);
	hda_reg8_write(&hda->regs->rirbsize, rirbsz);
	hda->ctl->rirb_entries = hda_rb_entries(selsz);

	ddf_msg(LVL_NOTE, "Read GCAP");
	uint16_t gcap = hda_reg16_read(&hda->regs->gcap);
	ok64bit = (gcap & BIT_V(uint8_t, gcap_64ok)) != 0;
	ddf_msg(LVL_NOTE, "GCAP: 0x%x (64OK=%d)", gcap, ok64bit);

	/*
	 * RIRB must be aligned to 128 bytes. If 64OK is not set,
	 * it must be within the 32-bit address space.
	 */
	hda->ctl->rirb_virt = AS_AREA_ANY;
	rc = dmamem_map_anonymous(hda->ctl->rirb_entries * sizeof(uint64_t),
	    ok64bit ? 0 : DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &hda->ctl->rirb_phys, &hda->ctl->rirb_virt);

	ddf_msg(LVL_NOTE, "Set RIRB base registers");

	/* Update RIRB base registers */
	hda_reg32_write(&hda->regs->rirblbase, LOWER32(hda->ctl->rirb_phys));
	hda_reg32_write(&hda->regs->rirbubase, UPPER32(hda->ctl->rirb_phys));

	ddf_msg(LVL_NOTE, "Rset RIRB Write pointer");

	/* Reset RIRB Write Pointer */
	hda_reg16_write(&hda->regs->rirbwp, BIT_V(uint16_t, rirbwp_rst));

	ddf_msg(LVL_NOTE, "RIRB initialized");
	return EOK;
error:
	return EIO;
}

hda_ctl_t *hda_ctl_init(hda_t *hda)
{
	hda_ctl_t *ctl;
	uint32_t gctl;
	int cnt;
	int rc;

	ctl = calloc(1, sizeof(hda_ctl_t));
	if (ctl == NULL)
		return NULL;

	hda->ctl = ctl;

	uint8_t vmaj = hda_reg8_read(&hda->regs->vmaj);
	uint8_t vmin = hda_reg8_read(&hda->regs->vmin);
	ddf_msg(LVL_NOTE, "HDA version %d.%d", vmaj, vmin);

	if (vmaj != 1 || vmin != 0) {
		ddf_msg(LVL_ERROR, "Unsupported HDA version (%d.%d).",
		    vmaj, vmin);
		goto error;
	}

	ddf_msg(LVL_NOTE, "reg 0x%zx STATESTS = 0x%x",
	    (void *)&hda->regs->statests - (void *)hda->regs, hda_reg16_read(&hda->regs->statests));

	gctl = hda_reg32_read(&hda->regs->gctl);
	if ((gctl & BIT_V(uint32_t, gctl_crst)) != 0) {
		ddf_msg(LVL_NOTE, "Controller not in reset. Resetting.");
		hda_reg32_write(&hda->regs->gctl, gctl & ~BIT_V(uint32_t, gctl_crst));
	}

	ddf_msg(LVL_NOTE, "Taking controller out of reset.");
	hda_reg32_write(&hda->regs->gctl, gctl | BIT_V(uint32_t, gctl_crst));

	/* Wait for CRST to read as 1 */
	cnt = ctrl_init_wait_max;
	while (cnt > 0) {
		gctl = hda_reg32_read(&hda->regs->gctl);
		if ((gctl & BIT_V(uint32_t, gctl_crst)) != 0) {
			ddf_msg(LVL_NOTE, "gctl=0x%x", gctl);
			break;
		}

		ddf_msg(LVL_NOTE, "Waiting for controller to initialize.");
		async_usleep(100*1000);
		--cnt;
	}

	if (cnt == 0) {
		ddf_msg(LVL_ERROR, "Timed out waiting for controller to come up.");
		goto error;
	}

	ddf_msg(LVL_NOTE, "Controller is out of reset.");

	/* Give codecs enough time to enumerate themselves */
	async_usleep(codec_enum_wait_us);

	ddf_msg(LVL_NOTE, "STATESTS = 0x%x",
	    hda_reg16_read(&hda->regs->statests));

	rc = hda_corb_init(hda);
	if (rc != EOK)
		goto error;

	rc = hda_rirb_init(hda);
	if (rc != EOK)
		goto error;

	return ctl;
error:
	free(ctl);
	hda->ctl = NULL;
	return NULL;
}

void hda_ctl_fini(hda_ctl_t *ctl)
{
	ddf_msg(LVL_NOTE, "hda_ctl_fini()");
	free(ctl);
}

/** @}
 */
