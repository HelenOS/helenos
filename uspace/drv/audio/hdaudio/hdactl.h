/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio controller
 */

#ifndef HDACTL_H
#define HDACTL_H

#include <fibril_synch.h>
#include <stdbool.h>
#include "hdaudio.h"
#include "spec/regs.h"

enum {
	/** Software response buffer size in entries */
	softrb_entries = 128
};

typedef struct hda_ctl {
	bool ok64bit;
	int iss;
	int oss;
	int bss;

	uintptr_t corb_phys;
	void *corb_virt;
	size_t corb_entries;

	uintptr_t rirb_phys;
	void *rirb_virt;
	size_t rirb_entries;
	size_t rirb_rp;

	fibril_mutex_t solrb_lock;
	fibril_condvar_t solrb_cv;
	hda_rirb_entry_t solrb[softrb_entries];
	size_t solrb_rp;
	size_t solrb_wp;

	hda_rirb_entry_t unsolrb[softrb_entries];
	size_t unsolrb_rp;
	size_t unsolrb_wp;

	struct hda_codec *codec;
	struct hda *hda;
} hda_ctl_t;

extern hda_ctl_t *hda_ctl_init(hda_t *);
extern void hda_ctl_fini(hda_ctl_t *);
extern void hda_ctl_interrupt(hda_ctl_t *);
extern errno_t hda_cmd(hda_t *, uint32_t, uint32_t *);
extern void hda_ctl_dump_info(hda_ctl_t *);

#endif

/** @}
 */
