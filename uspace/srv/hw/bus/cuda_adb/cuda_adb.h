/*
 * Copyright (c) 2006 Martin Decky
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup genarch
 * @{
 */
/** @file
 */

#ifndef CUDA_ADB_H_
#define CUDA_ADB_H_

#include <async.h>
#include <fibril_synch.h>
#include <loc.h>
#include <stdint.h>
#include "cuda_hw.h"

enum {
	CUDA_RCV_BUF_SIZE = 5
};

enum cuda_xfer_state {
	cx_listen,
	cx_receive,
	cx_rcv_end,
	cx_send_start,
	cx_send
};

typedef struct {
	service_id_t service_id;
	async_sess_t *client_sess;
} adb_dev_t;

typedef struct {
	struct cuda_regs *regs;
	uintptr_t cuda_physical;

	uint8_t rcv_buf[CUDA_RCV_BUF_SIZE];
	uint8_t snd_buf[CUDA_RCV_BUF_SIZE];
	size_t bidx;
	size_t snd_bytes;
	enum cuda_xfer_state xstate;
	fibril_mutex_t dev_lock;

	adb_dev_t adb_dev[ADB_MAX_ADDR];
} cuda_t;

#endif

/** @}
 */
