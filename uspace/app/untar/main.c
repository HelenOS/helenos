/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup untar
 * @{
 */
/** @file
 */

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <untar.h>

typedef struct {
	const char *filename;
	FILE *file;
} tar_state_t;

static int tar_open(tar_file_t *tar)
{
	tar_state_t *state = (tar_state_t *) tar->data;

	state->file = fopen(state->filename, "rb");
	if (state->file == NULL)
		return errno;

	return EOK;
}

static void tar_close(tar_file_t *tar)
{
	tar_state_t *state = (tar_state_t *) tar->data;
	fclose(state->file);
}

static size_t tar_read(tar_file_t *tar, void *data, size_t size)
{
	tar_state_t *state = (tar_state_t *) tar->data;
	return fread(data, 1, size, state->file);
}

static void tar_vreport(tar_file_t *tar, const char *fmt, va_list args)
{
	vfprintf(stderr, fmt, args);
}

tar_file_t tar = {
	.open = tar_open,
	.close = tar_close,

	.read = tar_read,
	.vreport = tar_vreport
};

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s tar-file\n", argv[0]);
		return 1;
	}

	tar_state_t state;
	state.filename = argv[1];

	tar.data = (void *) &state;
	return untar(&tar);
}

/** @}
 */
