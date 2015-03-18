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
