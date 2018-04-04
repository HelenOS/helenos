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
#include <fibril_synch.h>
#include <macros.h>
#include <stdint.h>

#include "codec.h"
#include "hdactl.h"
#include "regif.h"
#include "spec/regs.h"

enum {
	ctrl_init_wait_max = 10,
	codec_enum_wait_us = 512,
	corb_wait_max = 10,
	rirb_wait_max = 100,
	solrb_wait_us = 100 * 1000
};

static void hda_ctl_process_rirb(hda_ctl_t *);

/** Perform set-reset handshake on a 16-bit register.
 *
 * The bit(s) specified in the mask are written as 1, then we wait
 * for them to read as 1. Then we write them as 0 and we wait for them
 * to read as 0.
 */
static errno_t hda_ctl_reg16_set_reset(uint16_t *reg, uint16_t mask)
{
	uint16_t val;
	int wcnt;

	val = hda_reg16_read(reg);
	hda_reg16_write(reg, val | mask);

	wcnt = 1000;
	while (wcnt > 0) {
		val = hda_reg16_read(reg);
		if ((val & mask) == mask)
			break;

		async_usleep(1000);
		--wcnt;
	}

	if ((val & mask) != mask)
		return ETIMEOUT;

	val = hda_reg16_read(reg);
	hda_reg16_write(reg, val & ~mask);

	wcnt = 1000;
	while (wcnt > 0) {
		val = hda_reg16_read(reg);
		if ((val & mask) == 0)
			break;

		async_usleep(1000);
		--wcnt;
	}

	if ((val & mask) != 0)
		return ETIMEOUT;

	return EOK;
}

/** Select an appropriate CORB/RIRB size.
 *
 * We always use the largest available size. In @a sizecap each of bits
 * 0, 1, 2 determine whether one of the supported size (0 == 2 entries,
 * 1 == 16 entries, 2 == 256 entries) is supported. @a *selsz is set to
 * one of 0, 1, 2 on success.
 *
 * @param sizecap	CORB/RIRB Size Capability
 * @param selsz		Place to store CORB/RIRB Size
 * @return		EOK on success, EINVAL if sizecap has no valid bits set
 *
 */
static errno_t hda_rb_size_select(uint8_t sizecap, uint8_t *selsz)
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
static errno_t hda_corb_init(hda_t *hda)
{
	uint8_t ctl;
	uint8_t corbsz;
	uint8_t sizecap;
	uint8_t selsz;
	errno_t rc;

	ddf_msg(LVL_NOTE, "hda_corb_init()");

	/* Stop CORB if not stopped */
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


	/*
	 * CORB must be aligned to 128 bytes. If 64OK is not set,
	 * it must be within the 32-bit address space.
	 */
	hda->ctl->corb_virt = AS_AREA_ANY;
	rc = dmamem_map_anonymous(hda->ctl->corb_entries * sizeof(uint32_t),
	    hda->ctl->ok64bit ? 0 : DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &hda->ctl->corb_phys, &hda->ctl->corb_virt);

	ddf_msg(LVL_NOTE, "Set CORB base registers");

	/* Update CORB base registers */
	hda_reg32_write(&hda->regs->corblbase, LOWER32(hda->ctl->corb_phys));
	hda_reg32_write(&hda->regs->corbubase, UPPER32(hda->ctl->corb_phys));

	ddf_msg(LVL_NOTE, "Reset CORB Read/Write pointers");

	/* Reset CORB Read Pointer */
	rc = hda_ctl_reg16_set_reset(&hda->regs->corbrp,
	    BIT_V(uint16_t, corbrp_rst));
	if (rc != EOK) {
		ddf_msg(LVL_NOTE, "Failed resetting CORBRP");
		goto error;
	}

	/* Reset CORB Write Pointer */
	hda_reg16_write(&hda->regs->corbwp, 0);

	/* Start CORB */
	ctl = hda_reg8_read(&hda->regs->corbctl);
	ddf_msg(LVL_NOTE, "CORBctl (0x%x) = 0x%x",
	    (unsigned)((void *)&hda->regs->corbctl - (void *)hda->regs), ctl | BIT_V(uint8_t, corbctl_run));
	hda_reg8_write(&hda->regs->corbctl, ctl | BIT_V(uint8_t, corbctl_run));

	ddf_msg(LVL_NOTE, "CORB initialized");
	return EOK;
error:
	if (hda->ctl->corb_virt != NULL)
		dmamem_unmap_anonymous(&hda->ctl->corb_virt);
	return EIO;
}

/** Tear down the CORB */
static void hda_corb_fini(hda_t *hda)
{
	uint8_t ctl;

	/* Stop CORB */
	ctl = hda_reg8_read(&hda->regs->corbctl);
	hda_reg8_write(&hda->regs->corbctl, ctl & ~BIT_V(uint8_t, corbctl_run));

	if (hda->ctl->corb_virt != NULL)
		dmamem_unmap_anonymous(&hda->ctl->corb_virt);
}

/** Initialize the RIRB */
static errno_t hda_rirb_init(hda_t *hda)
{
	uint8_t ctl;
	uint8_t rirbsz;
	uint8_t sizecap;
	uint8_t selsz;
	errno_t rc;

	ddf_msg(LVL_NOTE, "hda_rirb_init()");

	/* Stop RIRB if not stopped */
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
	rirbsz = rirbsz | (selsz << rirbsize_size_l);

	ddf_msg(LVL_NOTE, "Setting RIRB Size register to 0x%x", rirbsz);
	hda_reg8_write(&hda->regs->rirbsize, rirbsz);
	hda->ctl->rirb_entries = hda_rb_entries(selsz);

	/*
	 * RIRB must be aligned to 128 bytes. If 64OK is not set,
	 * it must be within the 32-bit address space.
	 */
	hda->ctl->rirb_virt = AS_AREA_ANY;
	rc = dmamem_map_anonymous(hda->ctl->rirb_entries * sizeof(uint64_t),
	    hda->ctl->ok64bit ? 0 : DMAMEM_4GiB, AS_AREA_READ | AS_AREA_WRITE, 0,
	    &hda->ctl->rirb_phys, &hda->ctl->rirb_virt);

	ddf_msg(LVL_NOTE, "Set RIRB base registers");

	/* Update RIRB base registers */
	hda_reg32_write(&hda->regs->rirblbase, LOWER32(hda->ctl->rirb_phys));
	hda_reg32_write(&hda->regs->rirbubase, UPPER32(hda->ctl->rirb_phys));

	ddf_msg(LVL_NOTE, "Reset RIRB Write pointer");

	/* Reset RIRB Write Pointer */
	hda_reg16_write(&hda->regs->rirbwp, BIT_V(uint16_t, rirbwp_rst));

	/* Set RINTCNT - Qemu won't read from CORB if this is zero */
	hda_reg16_write(&hda->regs->rintcnt, hda->ctl->rirb_entries / 2);

	hda->ctl->rirb_rp = 0;

	/* Start RIRB and enable RIRB interrupt */
	ctl = hda_reg8_read(&hda->regs->rirbctl);
	ddf_msg(LVL_NOTE, "RIRBctl (0x%x) = 0x%x",
	    (unsigned)((void *)&hda->regs->rirbctl - (void *)hda->regs), ctl | BIT_V(uint8_t, rirbctl_run));
	hda_reg8_write(&hda->regs->rirbctl, ctl | BIT_V(uint8_t, rirbctl_run) |
	    BIT_V(uint8_t, rirbctl_int));

	ddf_msg(LVL_NOTE, "RIRB initialized");
	return EOK;
error:
	if (hda->ctl->rirb_virt != NULL)
		dmamem_unmap_anonymous(&hda->ctl->rirb_virt);
	return EIO;
}

/** Tear down the RIRB */
static void hda_rirb_fini(hda_t *hda)
{
	uint8_t ctl;

	/* Stop RIRB and disable RIRB interrupt */
	ctl = hda_reg8_read(&hda->regs->rirbctl);
	hda_reg8_write(&hda->regs->rirbctl, ctl &
	    ~(BIT_V(uint8_t, rirbctl_run) | BIT_V(uint8_t, rirbctl_int)));

	if (hda->ctl->rirb_virt != NULL)
		dmamem_unmap_anonymous(&hda->ctl->rirb_virt);
}

static size_t hda_get_corbrp(hda_t *hda)
{
	uint16_t corbrp;

	corbrp = hda_reg16_read(&hda->regs->corbrp);
	return BIT_RANGE_EXTRACT(uint16_t, corbrp_rp_h, corbrp_rp_l, corbrp);
}

static size_t hda_get_corbwp(hda_t *hda)
{
	uint16_t corbwp;

	corbwp = hda_reg16_read(&hda->regs->corbwp);
	return BIT_RANGE_EXTRACT(uint16_t, corbwp_wp_h, corbwp_wp_l, corbwp);
}

static void hda_set_corbwp(hda_t *hda, size_t wp)
{
	ddf_msg(LVL_DEBUG2, "Set CORBWP = %zu", wp);
	hda_reg16_write(&hda->regs->corbwp, wp);
}

static size_t hda_get_rirbwp(hda_t *hda)
{
	uint16_t rirbwp;

	rirbwp = hda_reg16_read(&hda->regs->rirbwp);
	return BIT_RANGE_EXTRACT(uint16_t, rirbwp_wp_h, rirbwp_wp_l, rirbwp);
}

/** Determine number of free entries in CORB */
static size_t hda_corb_avail(hda_t *hda)
{
	int rp, wp;
	int avail;

	rp = hda_get_corbrp(hda);
	wp = hda_get_corbwp(hda);

	avail = rp - wp - 1;
	while (avail < 0)
		avail += hda->ctl->corb_entries;

	return avail;
}

/** Write to CORB */
static errno_t hda_corb_write(hda_t *hda, uint32_t *data, size_t count)
{
	size_t avail;
	size_t wp;
	size_t idx;
	size_t now;
	size_t i;
	uint32_t *corb;
	int wcnt;

	avail = hda_corb_avail(hda);
	wp = hda_get_corbwp(hda);
	corb = (uint32_t *)hda->ctl->corb_virt;

	idx = 0;
	while (idx < count) {
		now = min(avail, count - idx);

		for (i = 0; i < now; i++) {
			wp = (wp + 1) % hda->ctl->corb_entries;
			corb[wp] = data[idx++];
		}

		hda_set_corbwp(hda, wp);

		if (idx < count) {
			/* We filled up CORB but still data remaining */
			wcnt = corb_wait_max;
			while (hda_corb_avail(hda) < 1 && wcnt > 0) {
				async_usleep(100);
				--wcnt;
			}

			/* If CORB is still full return timeout error */
			if (hda_corb_avail(hda) < 1)
				return ETIMEOUT;
		}
	}

	return EOK;
}

static errno_t hda_rirb_read(hda_t *hda, hda_rirb_entry_t *data)
{
	size_t wp;
	hda_rirb_entry_t resp;
	hda_rirb_entry_t *rirb;

	rirb = (hda_rirb_entry_t *)hda->ctl->rirb_virt;

	wp = hda_get_rirbwp(hda);
	ddf_msg(LVL_DEBUG2, "hda_rirb_read: wp=%zu", wp);
	if (hda->ctl->rirb_rp == wp)
		return ENOENT;

	hda->ctl->rirb_rp = (hda->ctl->rirb_rp + 1) % hda->ctl->rirb_entries;
	resp = rirb[hda->ctl->rirb_rp];

	ddf_msg(LVL_DEBUG2, "RESPONSE resp=0x%x respex=0x%x",
	    resp.resp, resp.respex);
	*data = resp;
	return EOK;
}

static errno_t hda_solrb_read(hda_t *hda, hda_rirb_entry_t *data, size_t count)
{
	hda_rirb_entry_t resp;

	ddf_msg(LVL_DEBUG, "hda_solrb_read()");

	fibril_mutex_lock(&hda->ctl->solrb_lock);

	while (count > 0) {
		while (count > 0 && hda->ctl->solrb_rp != hda->ctl->solrb_wp) {
			hda->ctl->solrb_rp = (hda->ctl->solrb_rp + 1) % softrb_entries;
			resp = hda->ctl->solrb[hda->ctl->solrb_rp];

			ddf_msg(LVL_DEBUG2, "solrb RESPONSE resp=0x%x respex=0x%x",
			    resp.resp, resp.respex);
			if ((resp.respex & BIT_V(uint32_t, respex_unsol)) == 0) {
				/* Solicited response */
				*data++ = resp;
				--count;
			}
		}

		if (count > 0) {
			if (hda->ctl->solrb_wp == hda->ctl->solrb_rp) {
				fibril_condvar_wait_timeout(
				    &hda->ctl->solrb_cv, &hda->ctl->solrb_lock,
				    solrb_wait_us);
			}

			if (hda->ctl->solrb_wp == hda->ctl->solrb_rp) {
				ddf_msg(LVL_NOTE, "hda_solrb_read() - last ditch effort process RIRB");
				fibril_mutex_unlock(&hda->ctl->solrb_lock);
				hda_ctl_process_rirb(hda->ctl);
				fibril_mutex_lock(&hda->ctl->solrb_lock);
			}

			if (hda->ctl->solrb_wp == hda->ctl->solrb_rp) {
				ddf_msg(LVL_NOTE, "hda_solrb_read() time out");
				fibril_mutex_unlock(&hda->ctl->solrb_lock);
				return ETIMEOUT;
			}
		}
	}

	fibril_mutex_unlock(&hda->ctl->solrb_lock);
	return EOK;
}

hda_ctl_t *hda_ctl_init(hda_t *hda)
{
	hda_ctl_t *ctl;
	uint32_t gctl;
	uint32_t intctl;
	int cnt;
	errno_t rc;

	ctl = calloc(1, sizeof(hda_ctl_t));
	if (ctl == NULL)
		return NULL;

	fibril_mutex_initialize(&ctl->solrb_lock);
	fibril_condvar_initialize(&ctl->solrb_cv);

	hda->ctl = ctl;
	ctl->hda = hda;

	uint8_t vmaj = hda_reg8_read(&hda->regs->vmaj);
	uint8_t vmin = hda_reg8_read(&hda->regs->vmin);
	ddf_msg(LVL_NOTE, "HDA version %d.%d", vmaj, vmin);

	if (vmaj != 1 || vmin != 0) {
		ddf_msg(LVL_ERROR, "Unsupported HDA version (%d.%d).",
		    vmaj, vmin);
		goto error;
	}

	ddf_msg(LVL_NOTE, "reg 0x%zx STATESTS = 0x%x",
	    (void *)&hda->regs->statests - (void *)hda->regs,
	    hda_reg16_read(&hda->regs->statests));
	/**
	  * Clear STATESTS bits so they don't generate an interrupt later
	  * when we enable interrupts.
	  */
	hda_reg16_write(&hda->regs->statests, 0x7f);

	ddf_msg(LVL_NOTE, "after clearing reg 0x%zx STATESTS = 0x%x",
	    (void *)&hda->regs->statests - (void *)hda->regs,
	    hda_reg16_read(&hda->regs->statests));

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
		async_usleep(100 * 1000);
		--cnt;
	}

	if (cnt == 0) {
		ddf_msg(LVL_ERROR, "Timed out waiting for controller to come up.");
		goto error;
	}

	ddf_msg(LVL_NOTE, "Controller is out of reset.");

	ddf_msg(LVL_NOTE, "Read GCAP");
	uint16_t gcap = hda_reg16_read(&hda->regs->gcap);
	ctl->ok64bit = (gcap & BIT_V(uint16_t, gcap_64ok)) != 0;
	ctl->oss = BIT_RANGE_EXTRACT(uint16_t, gcap_oss_h, gcap_oss_l, gcap);
	ctl->iss = BIT_RANGE_EXTRACT(uint16_t, gcap_iss_h, gcap_iss_l, gcap);
	ctl->bss = BIT_RANGE_EXTRACT(uint16_t, gcap_bss_h, gcap_bss_l, gcap);
	ddf_msg(LVL_NOTE, "GCAP: 0x%x (64OK=%d)", gcap, ctl->ok64bit);
	ddf_msg(LVL_NOTE, "iss: %d, oss: %d, bss: %d\n",
	    ctl->iss, ctl->oss, ctl->bss);
	/* Give codecs enough time to enumerate themselves */
	async_usleep(codec_enum_wait_us);

	ddf_msg(LVL_NOTE, "STATESTS = 0x%x",
	    hda_reg16_read(&hda->regs->statests));

	/* Enable interrupts */
	intctl = hda_reg32_read(&hda->regs->intctl);
	ddf_msg(LVL_NOTE, "intctl (0x%x) := 0x%x",
	    (unsigned)((void *)&hda->regs->intctl - (void *)hda->regs),
	    intctl | BIT_V(uint32_t, intctl_gie) | BIT_V(uint32_t, intctl_cie));
	hda_reg32_write(&hda->regs->intctl, intctl |
	    BIT_V(uint32_t, intctl_gie) | BIT_V(uint32_t, intctl_cie) |
	    0x3fffffff);

	rc = hda_corb_init(hda);
	if (rc != EOK)
		goto error;

	rc = hda_rirb_init(hda);
	if (rc != EOK)
		goto error;

	ddf_msg(LVL_NOTE, "call hda_codec_init()");
	hda->ctl->codec = hda_codec_init(hda, 0);
	if (hda->ctl->codec == NULL) {
		ddf_msg(LVL_NOTE, "hda_codec_init() failed");
		goto error;
	}

	ddf_msg(LVL_NOTE, "intsts=0x%x", hda_reg32_read(&hda->regs->intsts));
	ddf_msg(LVL_NOTE, "sdesc[%d].sts=0x%x",
	    hda->ctl->iss, hda_reg8_read(&hda->regs->sdesc[hda->ctl->iss].sts));

	return ctl;
error:
	hda_rirb_fini(hda);
	hda_corb_fini(hda);
	free(ctl);
	hda->ctl = NULL;
	return NULL;
}

void hda_ctl_fini(hda_ctl_t *ctl)
{
	ddf_msg(LVL_NOTE, "hda_ctl_fini()");
	hda_rirb_fini(ctl->hda);
	hda_corb_fini(ctl->hda);
	free(ctl);
}

errno_t hda_cmd(hda_t *hda, uint32_t verb, uint32_t *resp)
{
	errno_t rc;
	hda_rirb_entry_t rentry;

	rc = hda_corb_write(hda, &verb, 1);
	if (rc != EOK)
		return rc;

	if (resp != NULL) {
		rc = hda_solrb_read(hda, &rentry, 1);
		if (rc != EOK)
			return rc;

		/* XXX Verify that response came from the correct codec */
		*resp = rentry.resp;
	}

	return EOK;
}

static void hda_ctl_process_rirb(hda_ctl_t *ctl)
{
	hda_rirb_entry_t resp;
	errno_t rc;

	while (true) {
		rc = hda_rirb_read(ctl->hda, &resp);
		if (rc != EOK) {
//			ddf_msg(LVL_NOTE, "nothing in rirb");
			break;
		}

		ddf_msg(LVL_DEBUG2, "writing to solrb");
		fibril_mutex_lock(&ctl->solrb_lock);
		ctl->solrb_wp = (ctl->solrb_wp + 1) % softrb_entries;
		ctl->solrb[ctl->solrb_wp] = resp;
		fibril_mutex_unlock(&ctl->solrb_lock);
		fibril_condvar_broadcast(&ctl->solrb_cv);
	}
}

void hda_ctl_interrupt(hda_ctl_t *ctl)
{
	hda_ctl_process_rirb(ctl);
}

void hda_ctl_dump_info(hda_ctl_t *ctl)
{
	ddf_msg(LVL_NOTE, "corbwp=%d, corbrp=%d",
	    hda_reg16_read(&ctl->hda->regs->corbwp),
	    hda_reg16_read(&ctl->hda->regs->corbrp));
	ddf_msg(LVL_NOTE, "corbctl=0x%x, corbsts=0x%x",
	    hda_reg8_read(&ctl->hda->regs->corbctl),
	    hda_reg8_read(&ctl->hda->regs->corbsts));
	ddf_msg(LVL_NOTE, "rirbwp=0x%x, soft-rirbrp=0x%zx",
	    hda_reg16_read(&ctl->hda->regs->rirbwp),
	    ctl->rirb_rp);
	ddf_msg(LVL_NOTE, "solrb_wp=0x%zx, solrb_rp=0x%zx",
	    ctl->solrb_wp, ctl->solrb_wp);
}

/** @}
 */
