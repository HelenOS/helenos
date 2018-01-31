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

#ifndef BITHENGE_BLOB_H_
#define BITHENGE_BLOB_H_

#include <offset.h>
#include <errno.h>
#include "tree.h"

/** A blob of raw binary data.
 * @implements bithenge_node_t */
typedef struct {
	/** @privatesection */
	struct bithenge_node_t base;
} bithenge_blob_t;

/** Operations providing random access to binary data.
 * @todo Should these be thread-safe? */
typedef struct bithenge_random_access_blob_ops_t {
	/** @copydoc bithenge_blob_t::bithenge_blob_size */
	errno_t (*size)(bithenge_blob_t *self, aoff64_t *size);
	/** @copydoc bithenge_blob_t::bithenge_blob_read */
	errno_t (*read)(bithenge_blob_t *self, aoff64_t offset, char *buffer,
	    aoff64_t *size);
	/** @copydoc bithenge_blob_t::bithenge_blob_read_bits */
	errno_t (*read_bits)(bithenge_blob_t *self, aoff64_t offset, char *buffer,
	    aoff64_t *size, bool little_endian);
	/** Destroy the blob.
	 * @param blob The blob. */
	void (*destroy)(bithenge_blob_t *self);
} bithenge_random_access_blob_ops_t;

/** A blob built from an object that supports only sequential reading.
 * @implements bithenge_blob_t */
typedef struct {
	/** @privatesection */
	/** The base random-access blob. */
	bithenge_blob_t base;
	/** Operations providing sequential access. */
	const struct bithenge_sequential_blob_ops_t *ops;
	/** Buffer containing all data read. */
	char *buffer;
	/** Size of buffer. */
	aoff64_t buffer_size;
	/** Amount of data actually in buffer. */
	aoff64_t data_size;
} bithenge_sequential_blob_t;

/** Operations providing sequential access to binary data.
 * @memberof bithenge_sequential_blob_t */
typedef struct bithenge_sequential_blob_ops_t {

	/** Get the total size of the blob. If the total size cannot be
	 * determined easily, this field may be null or return an error,
	 * forcing the entire blob to be read to determine its size.
	 *
	 * @memberof bithenge_blob_t
	 * @param self The blob.
	 * @param[out] size Total size of the blob.
	 * @return EOK on success or an error code from errno.h.
	 */
	errno_t (*size)(bithenge_sequential_blob_t *self, aoff64_t *size);

	/** Read the next part of the blob. If the requested data extends
	 * beyond the end of the blob, the data up until the end of the blob
	 * will be read.
	 *
	 * @param self The blob.
	 * @param[out] buffer Buffer to read into. If an error occurs, the contents are
	 * undefined.
	 * @param[in,out] size Number of bytes to read; may be 0. If not enough
	 * data is left in the blob, the actual number of bytes read should be
	 * stored here. If an error occurs, the contents are undefined.
	 * @return EOK on success or an error code from errno.h.
	 */
	errno_t (*read)(bithenge_sequential_blob_t *self, char *buffer,
	    aoff64_t *size);

	/** Destroy the blob.
	 * @param self The blob. */
	void (*destroy)(bithenge_sequential_blob_t *self);
} bithenge_sequential_blob_ops_t;

/** Get the total size of the blob.
 *
 * @memberof bithenge_blob_t
 * @param self The blob.
 * @param[out] size Total size of the blob.
 * @return EOK on success or an error code from errno.h.
 */
static inline errno_t bithenge_blob_size(bithenge_blob_t *self, aoff64_t *size)
{
	assert(self);
	assert(self->base.blob_ops);
	return self->base.blob_ops->size(self, size);
}

/** Read part of the blob. If the requested data extends beyond the end of the
 * blob, the data up until the end of the blob will be read. If the offset is
 * beyond the end of the blob, even if the size is zero, an error will be
 * returned.
 *
 * @memberof bithenge_blob_t
 * @param self The blob.
 * @param offset Byte offset within the blob.
 * @param[out] buffer Buffer to read into. If an error occurs, the contents are
 * undefined.
 * @param[in,out] size Number of bytes to read; may be 0. If the requested
 * range extends beyond the end of the blob, the actual number of bytes read
 * should be stored here. If an error occurs, the contents are undefined.
 * @return EOK on success or an error code from errno.h.
 */
static inline errno_t bithenge_blob_read(bithenge_blob_t *self, aoff64_t offset,
    char *buffer, aoff64_t *size)
{
	assert(self);
	assert(self->base.blob_ops);
	if (!self->base.blob_ops->read)
		return EINVAL;
	return self->base.blob_ops->read(self, offset, buffer, size);
}

/** Read part of the bit blob. If the requested data extends beyond the end of
 * the blob, the data up until the end of the blob will be read. If the offset
 * is beyond the end of the blob, even if the size is zero, an error will be
 * returned.
 *
 * @memberof bithenge_blob_t
 * @param self The blob.
 * @param offset Byte offset within the blob.
 * @param[out] buffer Buffer to read into. If an error occurs, the contents are
 * undefined.
 * @param[in,out] size Number of bytes to read; may be 0. If the requested
 * range extends beyond the end of the blob, the actual number of bytes read
 * should be stored here. If an error occurs, the contents are undefined.
 * @param little_endian If true, bytes will be filled starting at the least
 * significant bit; otherwise, they will be filled starting at the most
 * significant bit.
 * @return EOK on success or an error code from errno.h.
 */
static inline errno_t bithenge_blob_read_bits(bithenge_blob_t *self,
    aoff64_t offset, char *buffer, aoff64_t *size, bool little_endian)
{
	assert(self);
	assert(self->base.blob_ops);
	if (!self->base.blob_ops->read_bits)
		return EINVAL;
	return self->base.blob_ops->read_bits(self, offset, buffer, size,
	    little_endian);
}

/** Check whether the blob is empty.
 *
 * @memberof bithenge_blob_t
 * @param self The blob.
 * @param[out] out Holds whether the blob is empty.
 * @return EOK on success or an error code from errno.h. */
static inline errno_t bithenge_blob_empty(bithenge_blob_t *self, bool *out)
{
	assert(self);
	assert(self->base.blob_ops);
	aoff64_t size;
	errno_t rc = bithenge_blob_size(self, &size);
	*out = size == 0;
	return rc;
}

/** Cast a blob node to a generic node.
 * @memberof bithenge_blob_t
 * @param blob The blob to cast.
 * @return The blob node as a generic node. */
static inline bithenge_node_t *bithenge_blob_as_node(bithenge_blob_t *blob)
{
	return &blob->base;
}

/** Cast a generic node to a blob node.
 * @memberof bithenge_blob_t
 * @param node The node to cast, which must be a blob node.
 * @return The generic node as a blob node. */
static inline bithenge_blob_t *bithenge_node_as_blob(bithenge_node_t *node)
{
	assert(node->type == BITHENGE_NODE_BLOB);
	return (bithenge_blob_t *)node;
}

/** Increment a blob's reference count.
 * @param blob The blob to reference. */
static inline void bithenge_blob_inc_ref(bithenge_blob_t *blob)
{
	bithenge_node_inc_ref(bithenge_blob_as_node(blob));
}

/** Decrement a blob's reference count.
 * @param blob The blob to dereference, or NULL. */
static inline void bithenge_blob_dec_ref(bithenge_blob_t *blob)
{
	if (blob)
		bithenge_node_dec_ref(bithenge_blob_as_node(blob));
}

/** @memberof bithenge_blob_t */
errno_t bithenge_init_random_access_blob(bithenge_blob_t *,
    const bithenge_random_access_blob_ops_t *);
/** @memberof bithenge_sequential_blob_t */
errno_t bithenge_init_sequential_blob(bithenge_sequential_blob_t *,
    const bithenge_sequential_blob_ops_t *);
/** @memberof bithenge_blob_t */
errno_t bithenge_new_blob_from_data(bithenge_node_t **, const void *, size_t);
/** @memberof bithenge_blob_t */
errno_t bithenge_new_blob_from_buffer(bithenge_node_t **, const void *, size_t,
    bool);
errno_t bithenge_new_offset_blob(bithenge_node_t **, bithenge_blob_t *, aoff64_t);
errno_t bithenge_new_subblob(bithenge_node_t **, bithenge_blob_t *, aoff64_t,
    aoff64_t);
/** @memberof bithenge_blob_t */
errno_t bithenge_blob_equal(bool *, bithenge_blob_t *, bithenge_blob_t *);

#endif

/** @}
 */
