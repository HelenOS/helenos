/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio driver
 */

#ifndef HDAUDIO_H
#define HDAUDIO_H

#include <async.h>
#include <ddf/driver.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <stdint.h>

#include "spec/regs.h"

/** High Definition Audio driver instance */
typedef struct hda {
	fibril_mutex_t lock;
	async_sess_t *parent_sess;
	async_sess_t *ev_sess;
	ddf_fun_t *fun_pcm;
	uint64_t rwbase;
	size_t rwsize;
	hda_regs_t *regs;
	struct hda_ctl *ctl;
	struct hda_stream *pcm_stream;
	struct hda_stream_buffers *pcm_buffers;
	bool playing;
	bool capturing;
} hda_t;

extern void hda_lock(hda_t *);
extern void hda_unlock(hda_t *);

#endif

/** @}
 */
