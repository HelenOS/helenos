/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2018 Martin Decky
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

/** @addtogroup init
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <loc.h>
#include <untar.h>
#include <block.h>
#include "init.h"
#include "untar.h"

typedef struct {
	const char *dev;

	service_id_t sid;
	aoff64_t offset;
} tar_state_t;

static int bd_tar_open(tar_file_t *tar)
{
	tar_state_t *state = (tar_state_t *) tar->data;

	errno_t ret = loc_service_get_id(state->dev, &state->sid,
	    IPC_FLAG_BLOCKING);
	if (ret != EOK)
		return ret;

	ret = block_init(state->sid);
	if (ret != EOK)
		return ret;

	state->offset = 0;
	return EOK;
}

static void bd_tar_close(tar_file_t *tar)
{
	tar_state_t *state = (tar_state_t *) tar->data;
	block_fini(state->sid);
}

static size_t bd_tar_read(tar_file_t *tar, void *data, size_t size)
{
	tar_state_t *state = (tar_state_t *) tar->data;

	if (block_read_bytes_direct(state->sid, state->offset, size,
	    data) != EOK)
		return 0;

	state->offset += size;
	return size;
}

static void bd_tar_vreport(tar_file_t *tar, const char *fmt, va_list args)
{
	vprintf(fmt, args);
}

tar_file_t tar = {
	.open = bd_tar_open,
	.close = bd_tar_close,

	.read = bd_tar_read,
	.vreport = bd_tar_vreport
};

bool bd_untar(const char *dev)
{
	tar_state_t state;
	state.dev = dev;

	tar.data = (void *) &state;
	return (untar(&tar) == EOK);
}

/** @}
 */
