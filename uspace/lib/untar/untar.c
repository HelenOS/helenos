/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libuntar
 * @{
 */
/** @file
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <str_error.h>
#include <vfs/vfs.h>
#include "private/tar.h"
#include "untar.h"

static size_t get_block_count(size_t bytes)
{
	return (bytes + TAR_BLOCK_SIZE - 1) / TAR_BLOCK_SIZE;
}

static int tar_open(tar_file_t *tar)
{
	return tar->open(tar);
}

static void tar_close(tar_file_t *tar)
{
	tar->close(tar);
}

static size_t tar_read(tar_file_t *tar, void *data, size_t size)
{
	return tar->read(tar, data, size);
}

static void tar_report(tar_file_t *tar, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	tar->vreport(tar, fmt, args);

	va_end(args);
}

static errno_t tar_skip_blocks(tar_file_t *tar, size_t valid_data_size)
{
	size_t blocks_to_read = get_block_count(valid_data_size);

	while (blocks_to_read > 0) {
		uint8_t block[TAR_BLOCK_SIZE];
		size_t actually_read = tar_read(tar, block, TAR_BLOCK_SIZE);
		if (actually_read != TAR_BLOCK_SIZE)
			return errno;

		blocks_to_read--;
	}

	return EOK;
}

static errno_t tar_handle_normal_file(tar_file_t *tar,
    const tar_header_t *header)
{
	// FIXME: create the directory first

	FILE *file = fopen(header->filename, "wb");
	if (file == NULL) {
		tar_report(tar, "Failed to create %s: %s.\n", header->filename,
		    str_error(errno));
		return errno;
	}

	errno_t rc = EOK;
	size_t bytes_remaining = header->size;
	size_t blocks = get_block_count(bytes_remaining);

	while (blocks > 0) {
		uint8_t block[TAR_BLOCK_SIZE];
		size_t actually_read = tar_read(tar, block, TAR_BLOCK_SIZE);
		if (actually_read != TAR_BLOCK_SIZE) {
			rc = errno;
			tar_report(tar, "Failed to read block for %s: %s.\n",
			    header->filename, str_error(rc));
			break;
		}

		size_t to_write = TAR_BLOCK_SIZE;
		if (bytes_remaining < TAR_BLOCK_SIZE)
			to_write = bytes_remaining;

		size_t actually_written = fwrite(block, 1, to_write, file);
		if (actually_written != to_write) {
			rc = errno;
			tar_report(tar, "Failed to write to %s: %s.\n",
			    header->filename, str_error(rc));
			break;
		}

		blocks--;
		bytes_remaining -= TAR_BLOCK_SIZE;
	}

	fclose(file);
	return rc;
}

static errno_t tar_handle_directory(tar_file_t *tar, const tar_header_t *header)
{
	errno_t rc = vfs_link_path(header->filename, KIND_DIRECTORY, NULL);
	if (rc != EOK) {
		if (rc != EEXIST) {
			tar_report(tar, "Failed to create directory %s: %s.\n",
			    header->filename, str_error(rc));
			return rc;
		}
	}

	return tar_skip_blocks(tar, header->size);
}

int untar(tar_file_t *tar)
{
	int rc = tar_open(tar);
	if (rc != EOK) {
		tar_report(tar, "Failed to open: %s.\n", str_error(rc));
		return rc;
	}

	while (true) {
		tar_header_raw_t header_raw;
		size_t header_ok = tar_read(tar, &header_raw, sizeof(header_raw));
		if (header_ok != sizeof(header_raw))
			break;

		tar_header_t header;
		errno_t rc = tar_header_parse(&header, &header_raw);
		if (rc == EEMPTY)
			continue;

		if (rc != EOK) {
			tar_report(tar, "Failed parsing TAR header: %s.\n", str_error(rc));
			break;
		}

		switch (header.type) {
		case TAR_TYPE_DIRECTORY:
			rc = tar_handle_directory(tar, &header);
			break;
		case TAR_TYPE_NORMAL:
			rc = tar_handle_normal_file(tar, &header);
			break;
		default:
			rc = tar_skip_blocks(tar, header.size);
			break;
		}

		if (rc != EOK)
			break;
	}

	tar_close(tar);
	return EOK;
}

/** @}
 */
