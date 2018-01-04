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
