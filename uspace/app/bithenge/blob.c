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
 * Raw binary blobs.
 */

#include <assert.h>
#include <bool.h>
#include <errno.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>
#include "blob.h"

/** Initialize a random access blob.
 * @memberof bithenge_blob_t
 * @param[out] blob The blob to initialize.
 * @param[in] ops Operations providing random access support. This pointer must
 * be valid until the blob is destroyed.
 * @return EOK on success or an error code from errno.h.
 */
int bithenge_new_random_access_blob(bithenge_blob_t *blob,
    const bithenge_random_access_blob_ops_t *ops)
{
	assert(blob);
	assert(ops);
	assert(ops->destroy);
	assert(ops->read);
	assert(ops->size);

	blob->ops = ops;
	return EOK;
}

static int sequential_buffer(bithenge_sequential_blob_t *blob, aoff64_t end)
{
	bool need_realloc = false;
	while (end > blob->buffer_size) {
		blob->buffer_size = max(4096, 2 * blob->buffer_size);
		need_realloc = true;
	}
	if (need_realloc) {
		char *buffer = realloc(blob->buffer, blob->buffer_size);
		if (!buffer)
			return ENOMEM;
		blob->buffer = buffer;
	}
	aoff64_t size = end - blob->data_size;
	int rc = blob->ops->read(blob, blob->buffer + blob->data_size, &size);
	if (rc != EOK)
		return rc;
	blob->data_size += size;
	return EOK;
}

static int sequential_size(bithenge_blob_t *base, aoff64_t *size)
{
	bithenge_sequential_blob_t *blob = (bithenge_sequential_blob_t *)base;
	int rc;
	if (blob->ops->size) {
		rc = blob->ops->size(blob, size);
		if (rc == EOK)
			return EOK;
	}
	rc = sequential_buffer(blob, blob->buffer_size);
	if (rc != EOK)
		return rc;
	while (blob->data_size == blob->buffer_size) {
		rc = sequential_buffer(blob, 2 * blob->buffer_size);
		if (rc != EOK)
			return rc;
	}
	*size = blob->data_size;
	return EOK;
}

static int sequential_read(bithenge_blob_t *base, aoff64_t offset,
    char *buffer, aoff64_t *size)
{
	bithenge_sequential_blob_t *blob = (bithenge_sequential_blob_t *)base;
	aoff64_t end = offset + *size;
	if (end > blob->data_size) {
		int rc = sequential_buffer(blob, end);
		if (rc != EOK)
			return rc;
	}
	if (offset > blob->data_size)
		return EINVAL;
	*size = min(*size, blob->data_size - end);
	memcpy(buffer, blob->buffer + offset, *size);
	return EOK;
}

static int sequential_destroy(bithenge_blob_t *base)
{
	bithenge_sequential_blob_t *blob = (bithenge_sequential_blob_t *)base;
	free(blob->buffer);
	return blob->ops->destroy(blob);
}

static const bithenge_random_access_blob_ops_t sequential_ops = {
	.size = sequential_size,
	.read = sequential_read,
	.destroy = sequential_destroy,
};

/** Initialize a sequential blob.
 * @memberof bithenge_sequential_blob_t
 * @param[out] blob The blob to initialize.
 * @param[in] ops Operations providing sequential access support. This pointer
 * must be valid until the blob is destroyed.
 * @return EOK on success or an error code from errno.h.
 */
int bithenge_new_sequential_blob(bithenge_sequential_blob_t *blob,
    const bithenge_sequential_blob_ops_t *ops)
{
	assert(blob);
	assert(ops);
	assert(ops->destroy);
	assert(ops->read);
	// ops->size is optional

	int rc = bithenge_new_random_access_blob(&blob->base, &sequential_ops);
	if (rc != EOK)
		return rc;
	blob->ops = ops;
	blob->buffer = NULL; // realloc(NULL, ...) works like malloc
	blob->buffer_size = 0;
	blob->data_size = 0;
	return EOK;
}

/** @}
 */
