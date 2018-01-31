/*
 * Copyright (c) 2012 Sean Bartell
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

/** @addtogroup bithenge
 * @{
 */
/**
 * @file
 * Access files as blobs.
 * @todo Provide more information about the file.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs/vfs.h>
#include <stddef.h>
#include "common.h"
#include <bithenge/blob.h>
#include <bithenge/file.h>

typedef struct {
	bithenge_blob_t base;
	int fd;
	aoff64_t size; // needed by file_read()
	bool needs_close;
} file_blob_t;

static inline file_blob_t *blob_as_file(bithenge_blob_t *base)
{
	return (file_blob_t *)base;
}

static inline bithenge_blob_t *file_as_blob(file_blob_t *blob)
{
	return &blob->base;
}

static errno_t file_size(bithenge_blob_t *base, aoff64_t *size)
{
	file_blob_t *blob = blob_as_file(base);
	*size = blob->size;
	return EOK;
}

static errno_t file_read(bithenge_blob_t *base, aoff64_t offset, char *buffer,
    aoff64_t *size)
{
	file_blob_t *blob = blob_as_file(base);
	if (offset > blob->size)
		return ELIMIT;

	size_t amount_read;
	errno_t rc = vfs_read(blob->fd, &offset, buffer, *size, &amount_read);
	if (rc != EOK)
		return errno;
	*size += amount_read;
	return EOK;
}

static void file_destroy(bithenge_blob_t *base)
{
	file_blob_t *blob = blob_as_file(base);
	vfs_put(blob->fd);
	free(blob);
}

static const bithenge_random_access_blob_ops_t file_ops = {
	.size = file_size,
	.read = file_read,
	.destroy = file_destroy,
};

static errno_t new_file_blob(bithenge_node_t **out, int fd, bool needs_close)
{
	assert(out);

	vfs_stat_t stat;
	errno_t rc = vfs_stat(fd, &stat);
	if (rc != EOK) {
		if (needs_close)
			vfs_put(fd);
		return rc;
	}

	// Create blob
	file_blob_t *blob = malloc(sizeof(*blob));
	if (!blob) {
		if (needs_close)
			vfs_put(fd);
		return ENOMEM;
	}
	rc = bithenge_init_random_access_blob(file_as_blob(blob), &file_ops);
	if (rc != EOK) {
		free(blob);
		if (needs_close)
			vfs_put(fd);
		return rc;
	}
	blob->fd = fd;
#ifdef __HELENOS__
	blob->size = stat.size;
#else
	blob->size = stat.st_size;
#endif
	blob->needs_close = needs_close;
	*out = bithenge_blob_as_node(file_as_blob(blob));

	return EOK;
}

/** Create a blob for a file. The blob must be freed with @a
 * bithenge_node_t::bithenge_node_destroy after it is used.
 * @param[out] out Stores the created blob.
 * @param filename The name of the file.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_new_file_blob(bithenge_node_t **out, const char *filename)
{
	assert(filename);

	int fd;
	errno_t rc = vfs_lookup_open(filename, WALK_REGULAR, MODE_READ, &fd);
	if (rc != EOK)
		return rc;

	return new_file_blob(out, fd, true);
}

/** Create a blob for a file descriptor. The blob must be freed with @a
 * bithenge_node_t::bithenge_node_destroy after it is used.
 * @param[out] out Stores the created blob.
 * @param fd The file descriptor.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_new_file_blob_from_fd(bithenge_node_t **out, int fd)
{
	return new_file_blob(out, fd, false);
}

/** Create a blob for a file pointer. The blob must be freed with @a
 * bithenge_node_t::bithenge_node_destroy after it is used.
 * @param[out] out Stores the created blob.
 * @param file The file pointer.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_new_file_blob_from_file(bithenge_node_t **out, FILE *file)
{
	int fd = fileno(file);
	if (fd < 0)
		return errno;
	return new_file_blob(out, fd, false);
}

/** @}
 */
