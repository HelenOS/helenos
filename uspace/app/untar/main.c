/*
 * Copyright (c) 2013 Vojtech Horky
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

/** @addtogroup untar
 * @{
 */
/** @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <str_error.h>
#include <vfs/vfs.h>
#include "tar.h"

static size_t get_block_count(size_t bytes)
{
	return (bytes + TAR_BLOCK_SIZE - 1) / TAR_BLOCK_SIZE;
}

static errno_t skip_blocks(FILE *tarfile, size_t valid_data_size)
{
	size_t blocks_to_read = get_block_count(valid_data_size);
	while (blocks_to_read > 0) {
		uint8_t block[TAR_BLOCK_SIZE];
		size_t actually_read = fread(block, TAR_BLOCK_SIZE, 1, tarfile);
		if (actually_read != 1) {
			return errno;
		}
		blocks_to_read--;
	}
	return EOK;
}

static errno_t handle_normal_file(const tar_header_t *header, FILE *tarfile)
{
	// FIXME: create the directory first

	FILE *file = fopen(header->filename, "wb");
	if (file == NULL) {
		fprintf(stderr, "Failed to create %s: %s.\n", header->filename,
		    str_error(errno));
		return errno;
	}

	errno_t rc = EOK;
	size_t bytes_remaining = header->size;
	size_t blocks = get_block_count(bytes_remaining);
	while (blocks > 0) {
		uint8_t block[TAR_BLOCK_SIZE];
		size_t actually_read = fread(block, 1, TAR_BLOCK_SIZE, tarfile);
		if (actually_read != TAR_BLOCK_SIZE) {
			rc = errno;
			fprintf(stderr, "Failed to read block for %s: %s.\n",
			    header->filename, str_error(rc));
			break;
		}
		size_t to_write = TAR_BLOCK_SIZE;
		if (bytes_remaining < TAR_BLOCK_SIZE) {
			to_write = bytes_remaining;
		}
		size_t actually_written = fwrite(block, 1, to_write, file);
		if (actually_written != to_write) {
			rc = errno;
			fprintf(stderr, "Failed to write to %s: %s.\n",
			    header->filename, str_error(rc));
			break;
		}
		blocks--;
		bytes_remaining -= TAR_BLOCK_SIZE;
	}

	fclose(file);

	return rc;
}

static errno_t handle_directory(const tar_header_t *header, FILE *tarfile)
{
	errno_t rc;

	rc = vfs_link_path(header->filename, KIND_DIRECTORY, NULL);
	if (rc != EOK) {
		if (rc != EEXIST) {
			fprintf(stderr, "Failed to create directory %s: %s.\n",
			    header->filename, str_error(rc));
			return rc;
		}
	}

	return skip_blocks(tarfile, header->size);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s tar-file\n", argv[0]);
		return 1;
	}

	const char *filename = argv[1];

	FILE *tarfile = fopen(filename, "rb");
	if (tarfile == NULL) {
		fprintf(stderr, "Failed to open `%s': %s.\n", filename, str_error(errno));
		return 2;
	}

	while (true) {
		size_t header_ok;
		tar_header_raw_t header_raw;
		tar_header_t header;
		header_ok = fread(&header_raw, sizeof(header_raw), 1, tarfile);
		if (header_ok != 1) {
			break;
		}
		errno_t rc = tar_header_parse(&header, &header_raw);
		if (rc == EEMPTY) {
			continue;
		}
		if (rc != EOK) {
			fprintf(stderr, "Failed parsing TAR header: %s.\n", str_error(rc));
			break;
		}

		//printf(" ==> %s (%zuB, type %s)\n", header.filename,
		//    header.size, tar_type_str(header.type));

		switch (header.type) {
		case TAR_TYPE_DIRECTORY:
			rc = handle_directory(&header, tarfile);
			break;
		case TAR_TYPE_NORMAL:
			rc = handle_normal_file(&header, tarfile);
			break;
		default:
			rc = skip_blocks(tarfile, header.size);
			break;
		}
		if (rc != EOK) {
			break;
		}

	}

	fclose(tarfile);

	return 0;
}

/** @}
 */
