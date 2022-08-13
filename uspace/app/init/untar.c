/*
 * SPDX-FileCopyrightText: 2018 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

	ret = block_init(state->sid, 4096);
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
