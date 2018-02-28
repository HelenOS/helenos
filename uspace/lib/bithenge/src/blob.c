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
#include <errno.h>
#include <stdlib.h>
#include "common.h"
#include <bithenge/blob.h>
#include <bithenge/tree.h>

/** Initialize a random access blob.
 * @memberof bithenge_blob_t
 * @param[out] blob The blob to initialize.
 * @param[in] ops Operations providing random access support. This pointer must
 * be valid until the blob is destroyed.
 * @return EOK on success or an error code from errno.h.
 */
errno_t bithenge_init_random_access_blob(bithenge_blob_t *blob,
    const bithenge_random_access_blob_ops_t *ops)
{
	assert(blob);
	assert(ops);
	assert(ops->destroy);
	assert(ops->read || ops->read_bits);
	assert(ops->size);

	if (bithenge_should_fail())
		return ENOMEM;

	blob->base.type = BITHENGE_NODE_BLOB;
	blob->base.refs = 1;
	blob->base.blob_ops = ops;
	return EOK;
}

static errno_t sequential_buffer(bithenge_sequential_blob_t *blob, aoff64_t end)
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
	errno_t rc = blob->ops->read(blob, blob->buffer + blob->data_size, &size);
	if (rc != EOK)
		return rc;
	blob->data_size += size;
	return EOK;
}

static inline bithenge_sequential_blob_t *blob_as_sequential(
    bithenge_blob_t *base)
{
	return (bithenge_sequential_blob_t *)base;
}

static inline bithenge_blob_t *sequential_as_blob(
    bithenge_sequential_blob_t *blob)
{
	return &blob->base;
}

static errno_t sequential_size(bithenge_blob_t *base, aoff64_t *size)
{
	bithenge_sequential_blob_t *blob = blob_as_sequential(base);
	errno_t rc;
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

static errno_t sequential_read(bithenge_blob_t *base, aoff64_t offset,
    char *buffer, aoff64_t *size)
{
	bithenge_sequential_blob_t *blob = blob_as_sequential(base);
	aoff64_t end = offset + *size;
	if (end > blob->data_size) {
		errno_t rc = sequential_buffer(blob, end);
		if (rc != EOK)
			return rc;
	}
	if (offset > blob->data_size)
		return EINVAL;
	*size = min(*size, blob->data_size - end);
	memcpy(buffer, blob->buffer + offset, *size);
	return EOK;
}

static void sequential_destroy(bithenge_blob_t *base)
{
	bithenge_sequential_blob_t *blob = blob_as_sequential(base);
	free(blob->buffer);
	blob->ops->destroy(blob);
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
errno_t bithenge_init_sequential_blob(bithenge_sequential_blob_t *blob,
    const bithenge_sequential_blob_ops_t *ops)
{
	assert(blob);
	assert(ops);
	assert(ops->destroy);
	assert(ops->read);
	// ops->size is optional

	errno_t rc = bithenge_init_random_access_blob(sequential_as_blob(blob),
	    &sequential_ops);
	if (rc != EOK)
		return rc;
	blob->ops = ops;
	blob->buffer = NULL; // realloc(NULL, ...) works like malloc
	blob->buffer_size = 0;
	blob->data_size = 0;
	return EOK;
}

typedef struct {
	bithenge_blob_t base;
	const char *buffer;
	size_t size;
	bool needs_free;
} memory_blob_t;

static inline memory_blob_t *blob_as_memory(bithenge_blob_t *base)
{
	return (memory_blob_t *)base;
}

static inline bithenge_blob_t *memory_as_blob(memory_blob_t *blob)
{
	return &blob->base;
}

static errno_t memory_size(bithenge_blob_t *base, aoff64_t *size)
{
	memory_blob_t *blob = blob_as_memory(base);
	if (bithenge_should_fail())
		return EIO;
	*size = blob->size;
	return EOK;
}

static errno_t memory_read(bithenge_blob_t *base, aoff64_t offset, char *buffer,
    aoff64_t *size)
{
	memory_blob_t *blob = blob_as_memory(base);
	if (offset > blob->size)
		return ELIMIT;
	*size = min(*size, blob->size - offset);
	memcpy(buffer, blob->buffer + offset, *size);
	return EOK;
}

static void memory_destroy(bithenge_blob_t *base)
{
	memory_blob_t *blob = blob_as_memory(base);
	if (blob->needs_free)
		free((void *)blob->buffer);
	free(blob);
}

static const bithenge_random_access_blob_ops_t memory_ops = {
	.size = memory_size,
	.read = memory_read,
	.destroy = memory_destroy,
};

/** Create a blob node from a buffer. The buffer must exist as long as the blob
 * does. The blob must be freed with @a bithenge_node_t::bithenge_node_destroy
 * after it is used.
 * @memberof bithenge_blob_t
 * @param[out] out Stores the created blob node.
 * @param[in] buffer The buffer, which must not be changed until the blob is
 * destroyed.
 * @param len The length of the data.
 * @param needs_free If true, the buffer will be freed with free() if this
 * function fails or the blob is destroyed.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_new_blob_from_buffer(bithenge_node_t **out, const void *buffer,
    size_t len, bool needs_free)
{
	errno_t rc;
	assert(buffer || !len);

	memory_blob_t *blob = malloc(sizeof(*blob));
	if (!blob) {
		rc = ENOMEM;
		goto error;
	}
	rc = bithenge_init_random_access_blob(memory_as_blob(blob),
	    &memory_ops);
	if (rc != EOK)
		goto error;
	blob->buffer = buffer;
	blob->size = len;
	blob->needs_free = needs_free;
	*out = bithenge_blob_as_node(memory_as_blob(blob));
	return EOK;

error:
	if (needs_free)
		free((void *)buffer);
	free(blob);
	return rc;
}

/** Create a blob node from data. Unlike with @a
 * bithenge_blob_t::bithenge_new_blob_from_buffer, the data is copied into a
 * new buffer and the original data can be changed after this call. The blob
 * must be freed with @a bithenge_node_t::bithenge_node_destroy after it is
 * used.
 * @memberof bithenge_blob_t
 * @param[out] out Stores the created blob node.
 * @param[in] data The data.
 * @param len The length of the data.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_new_blob_from_data(bithenge_node_t **out, const void *data,
    size_t len)
{
	char *buffer = malloc(len);
	if (!buffer)
		return ENOMEM;
	memcpy(buffer, data, len);

	return bithenge_new_blob_from_buffer(out, buffer, len, true);
}



typedef struct {
	bithenge_blob_t base;
	bithenge_blob_t *source;
	aoff64_t offset;
	aoff64_t size;
	bool size_matters;
} subblob_t;

static inline subblob_t *blob_as_subblob(bithenge_blob_t *base)
{
	return (subblob_t *)base;
}

static inline bithenge_blob_t *subblob_as_blob(subblob_t *blob)
{
	return &blob->base;
}

static errno_t subblob_size(bithenge_blob_t *base, aoff64_t *size)
{
	subblob_t *blob = blob_as_subblob(base);
	if (blob->size_matters) {
		*size = blob->size;
		return EOK;
	} else {
		errno_t rc = bithenge_blob_size(blob->source, size);
		*size -= blob->offset;
		return rc;
	}
}

static errno_t subblob_read(bithenge_blob_t *base, aoff64_t offset,
    char *buffer, aoff64_t *size)
{
	subblob_t *blob = blob_as_subblob(base);
	if (blob->size_matters) {
		if (offset > blob->size)
			return EINVAL;
		*size = min(*size, blob->size - offset);
	}
	offset += blob->offset;
	return bithenge_blob_read(blob->source, offset, buffer, size);
}

static errno_t subblob_read_bits(bithenge_blob_t *base, aoff64_t offset,
    char *buffer, aoff64_t *size, bool little_endian)
{
	subblob_t *blob = blob_as_subblob(base);
	if (blob->size_matters) {
		if (offset > blob->size)
			return EINVAL;
		*size = min(*size, blob->size - offset);
	}
	offset += blob->offset;
	return bithenge_blob_read_bits(blob->source, offset, buffer, size,
	    little_endian);
}

static void subblob_destroy(bithenge_blob_t *base)
{
	subblob_t *blob = blob_as_subblob(base);
	bithenge_blob_dec_ref(blob->source);
	free(blob);
}

static const bithenge_random_access_blob_ops_t subblob_ops = {
	.size = subblob_size,
	.read = subblob_read,
	.read_bits = subblob_read_bits,
	.destroy = subblob_destroy,
};

static bool is_subblob(bithenge_blob_t *blob)
{
	return blob->base.blob_ops == &subblob_ops;
}

static errno_t new_subblob(bithenge_node_t **out, bithenge_blob_t *source,
    aoff64_t offset, aoff64_t size, bool size_matters)
{
	assert(out);
	assert(source);
	errno_t rc;
	subblob_t *blob = 0;

	if (is_subblob(source)) {
		/* We can do some optimizations this way */
		if (!size_matters)
			size = 0;
		subblob_t *source_subblob = blob_as_subblob(source);
		if (source_subblob->size_matters &&
		    offset + size > source_subblob->size) {
			rc = EINVAL;
			goto error;
		}

		if (source->base.refs == 1) {
			source_subblob->offset += offset;
			source_subblob->size -= offset;
			if (size_matters) {
				source_subblob->size_matters = true;
				source_subblob->size = size;
			}
			*out = bithenge_blob_as_node(source);
			return EOK;
		}

		if (!size_matters && source_subblob->size_matters) {
			size_matters = true;
			size = source_subblob->size - offset;
		}
		offset += source_subblob->offset;
		source = source_subblob->source;
		bithenge_blob_inc_ref(source);
		bithenge_blob_dec_ref(subblob_as_blob(source_subblob));
	}

	blob = malloc(sizeof(*blob));
	if (!blob) {
		rc = ENOMEM;
		goto error;
	}
	rc = bithenge_init_random_access_blob(subblob_as_blob(blob),
	    &subblob_ops);
	if (rc != EOK)
		goto error;
	blob->source = source;
	blob->offset = offset;
	blob->size = size;
	blob->size_matters = size_matters;
	*out = bithenge_blob_as_node(subblob_as_blob(blob));
	return EOK;

error:
	bithenge_blob_dec_ref(source);
	free(blob);
	return rc;
}

/** Create a blob from data offset within another blob. This function takes
 * ownership of a reference to @a blob.
 * @param[out] out Stores the created blob node. On error, this is unchanged.
 * @param[in] source The input blob.
 * @param offset The offset within the input blob at which the new blob will start.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_new_offset_blob(bithenge_node_t **out, bithenge_blob_t *source,
    aoff64_t offset)
{
	return new_subblob(out, source, offset, 0, false);
}

/** Create a blob from part of another blob. This function takes ownership of a
 * reference to @a blob.
 * @param[out] out Stores the created blob node. On error, this is unchanged.
 * @param[in] source The input blob.
 * @param offset The offset within the input blob at which the new blob will start.
 * @param size The size of the new blob.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_new_subblob(bithenge_node_t **out, bithenge_blob_t *source,
    aoff64_t offset, aoff64_t size)
{
	return new_subblob(out, source, offset, size, true);
}

/** Check whether the contents of two blobs are equal.
 * @memberof bithenge_blob_t
 * @param[out] out Holds whether the blobs are equal.
 * @param a, b Blobs to compare.
 * @return EOK on success, or an error code from errno.h.
 */
errno_t bithenge_blob_equal(bool *out, bithenge_blob_t *a, bithenge_blob_t *b)
{
	assert(a);
	assert(a->base.blob_ops);
	assert(b);
	assert(b->base.blob_ops);
	errno_t rc;
	char buffer_a[4096], buffer_b[4096];
	aoff64_t offset = 0, size_a = sizeof(buffer_a), size_b = sizeof(buffer_b);
	do {
		rc = bithenge_blob_read(a, offset, buffer_a, &size_a);
		if (rc != EOK)
			return rc;
		rc = bithenge_blob_read(b, offset, buffer_b, &size_b);
		if (rc != EOK)
			return rc;
		if (size_a != size_b || memcmp(buffer_a, buffer_b, size_a) != 0) {
			*out = false;
			return EOK;
		}
		offset += size_a;
	} while (size_a == sizeof(buffer_a));
	*out = true;
	return EOK;
}

/** @}
 */
