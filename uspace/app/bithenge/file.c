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
#include <fcntl.h>
#include <macros.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "blob.h"
#include "file.h"

typedef struct {
	bithenge_blob_t base;
	int fd;
	aoff64_t size; // needed by file_read()
} file_blob_t;

static inline file_blob_t *file_from_blob(bithenge_blob_t *base)
{
	return (file_blob_t *)base;
}

static inline bithenge_blob_t *blob_from_file(file_blob_t *blob)
{
	return &blob->base;
}

static int file_size(bithenge_blob_t *base, aoff64_t *size)
{
	file_blob_t *blob = file_from_blob(base);
	*size = blob->size;
	return EOK;
}

static int file_read(bithenge_blob_t *base, aoff64_t offset, char *buffer,
    aoff64_t *size)
{
	file_blob_t *blob = file_from_blob(base);
	if (offset > blob->size)
		return ELIMIT;
	*size = min(*size, blob->size - offset);
	int rc = lseek(blob->fd, offset, SEEK_SET);
	if (rc != EOK)
		return rc;
	return read(blob->fd, buffer, *size);
}

static int file_destroy(bithenge_blob_t *base)
{
	file_blob_t *blob = file_from_blob(base);
	close(blob->fd);
	free(blob);
	return EOK;
}

static const bithenge_random_access_blob_ops_t file_ops = {
	.size = file_size,
	.read = file_read,
	.destroy = file_destroy,
};

/** Create a blob for a file. The blob must be freed with @a
 * bithenge_blob_t::bithenge_blob_destroy after it is used.
 * @param[out] out Stores the created blob.
 * @param filename The name of the file.
 * @return EOK on success or an error code from errno.h. */
int bithenge_new_file_blob(bithenge_blob_t **out, const char *filename)
{
	assert(out);
	assert(filename);

	int fd;
	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return fd;

	struct stat stat;
	int rc = fstat(fd, &stat);
	if (rc != EOK) {
		close(fd);
		return rc;
	}

	// Create blob
	file_blob_t *blob = malloc(sizeof(*blob));
	if (!blob) {
		close(fd);
		return ENOMEM;
	}
	rc = bithenge_new_random_access_blob(blob_from_file(blob),
	    &file_ops);
	if (rc != EOK) {
		free(blob);
		close(fd);
		return rc;
	}
	blob->fd = fd;
	blob->size = stat.size;
	*out = blob_from_file(blob);

	return EOK;
}

/** @}
 */
