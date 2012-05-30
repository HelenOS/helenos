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

#include <sys/types.h>

/** A blob of raw binary data. */
typedef struct {
	/** @privatesection */
	/** Operations providing random access. */
	const struct bithenge_random_access_blob_ops_t *ops;
} bithenge_blob_t;

/** Operations providing random access to binary data.
 * @todo Should these be thread-safe? */
typedef struct bithenge_random_access_blob_ops_t {
	/** @copydoc bithenge_blob_t::bithenge_blob_size */
	int (*size)(bithenge_blob_t *blob, aoff64_t *size);
	/** @copydoc bithenge_blob_t::bithenge_blob_read */
	int (*read)(bithenge_blob_t *blob, aoff64_t offset, char *buffer,
	    aoff64_t *size);
	/** @copydoc bithenge_blob_t::bithenge_blob_destroy */
	int (*destroy)(bithenge_blob_t *blob);
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
	 * @param blob The blob.
	 * @param[out] size Total size of the blob.
	 * @return EOK on success or an error code from errno.h.
	 */
	int (*size)(bithenge_sequential_blob_t *blob, aoff64_t *size);

	/** Read the next part of the blob. If the requested data extends
	 * beyond the end of the blob, the data up until the end of the blob
	 * will be read.
	 *
	 * @param blob The blob.
	 * @param[out] buffer Buffer to read into. If an error occurs, the contents are
	 * undefined.
	 * @param[in,out] size Number of bytes to read; may be 0. If not enough
	 * data is left in the blob, the actual number of bytes read should be
	 * stored here. If an error occurs, the contents are undefined.
	 * @return EOK on success or an error code from errno.h.
	 */
	int (*read)(bithenge_sequential_blob_t *blob, char *buffer,
	    aoff64_t *size);

	/** Destroy the blob.
	 * @param blob The blob.
	 * @return EOK on success or an error code from errno.h. */
	int (*destroy)(bithenge_sequential_blob_t *blob);
} bithenge_sequential_blob_ops_t;

/** Get the total size of the blob.
 *
 * @memberof bithenge_blob_t
 * @param blob The blob.
 * @param[out] size Total size of the blob.
 * @return EOK on success or an error code from errno.h.
 */
static inline int bithenge_blob_size(bithenge_blob_t *blob, aoff64_t *size)
{
	assert(blob);
	assert(blob->ops);
	return blob->ops->size(blob, size);
}

/** Read part of the blob. If the requested data extends beyond the end of the
 * blob, the data up until the end of the blob will be read. If the offset is
 * beyond the end of the blob, even if the size is zero, an error will be
 * returned.
 *
 * @memberof bithenge_blob_t
 * @param blob The blob.
 * @param offset Byte offset within the blob.
 * @param[out] buffer Buffer to read into. If an error occurs, the contents are
 * undefined.
 * @param[in,out] size Number of bytes to read; may be 0. If the requested
 * range extends beyond the end of the blob, the actual number of bytes read
 * should be stored here. If an error occurs, the contents are undefined.
 * @return EOK on success or an error code from errno.h.
 */
static inline int bithenge_blob_read(bithenge_blob_t *blob, aoff64_t offset,
    char *buffer, aoff64_t *size)
{
	assert(blob);
	assert(blob->ops);
	return blob->ops->read(blob, offset, buffer, size);
}

/** Destroy the blob.
 * @memberof bithenge_blob_t
 * @param blob The blob.
 * @return EOK on success or an error code from errno.h.
 */
static inline int bithenge_blob_destroy(bithenge_blob_t *blob)
{
	assert(blob);
	assert(blob->ops);
	return blob->ops->destroy(blob);
}

int bithenge_new_random_access_blob(bithenge_blob_t *blob,
    const bithenge_random_access_blob_ops_t *ops);

int bithenge_new_sequential_blob(bithenge_sequential_blob_t *blob,
    const bithenge_sequential_blob_ops_t *ops);

int bithenge_new_blob_from_data(bithenge_blob_t **out, const void *data,
    size_t len);

int bithenge_new_blob_from_buffer(bithenge_blob_t **out, const void *buffer,
    size_t len, bool needs_free);

#endif

/** @}
 */
