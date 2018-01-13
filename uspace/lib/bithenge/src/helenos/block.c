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
 * Access block devices as blobs.
 * @todo Provide more information about the block device (block size).
 */

#include <assert.h>
#include <block.h>
#include <errno.h>
#include <loc.h>
#include <macros.h>
#include <stdlib.h>
#include <bithenge/blob.h>
#include "block.h"

typedef struct {
	bithenge_blob_t base;
	service_id_t service_id;
	aoff64_t size;
} block_blob_t;

static inline block_blob_t *blob_as_block(bithenge_blob_t *base)
{
	return (block_blob_t *)base;
}

static inline bithenge_blob_t *block_as_blob(block_blob_t *blob)
{
	return &blob->base;
}

static errno_t block_size(bithenge_blob_t *base, aoff64_t *size)
{
	block_blob_t *self = blob_as_block(base);
	*size = self->size;
	return EOK;
}

static errno_t block_read(bithenge_blob_t *base, aoff64_t offset, char *buffer,
    aoff64_t *size)
{
	block_blob_t *self = blob_as_block(base);
	if (offset > self->size)
		return ELIMIT;
	*size = min(*size, self->size - offset);
	return block_read_bytes_direct(self->service_id, offset, *size, buffer);
}

static void block_destroy(bithenge_blob_t *base)
{
	block_blob_t *self = blob_as_block(base);
	block_fini(self->service_id);
	free(self);
}

static const bithenge_random_access_blob_ops_t block_ops = {
	.size = block_size,
	.read = block_read,
	.destroy = block_destroy,
};

/** Create a blob for a block device. The blob must be freed with
 * @a bithenge_node_t::bithenge_node_destroy after it is used.
 * @param[out] out Stores the created blob.
 * @param service_id The service ID of the block device.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_new_block_blob(bithenge_node_t **out, service_id_t service_id)
{
	assert(out);

	// Initialize libblock
	errno_t rc;
	rc = block_init(service_id, 2048);
	if (rc != EOK)
		return rc;

	// Calculate total device size
	size_t bsize;
	aoff64_t nblocks;
	aoff64_t size;
	rc = block_get_bsize(service_id, &bsize);
	if (rc != EOK) {
		block_fini(service_id);
		return rc;
	}
	rc = block_get_nblocks(service_id, &nblocks);
	if (rc != EOK) {
		block_fini(service_id);
		return rc;
	}
	size = bsize * nblocks;

	// Create blob
	block_blob_t *blob = malloc(sizeof(*blob));
	if (!blob) {
		block_fini(service_id);
		return ENOMEM;
	}
	rc = bithenge_init_random_access_blob(block_as_blob(blob),
	    &block_ops);
	if (rc != EOK) {
		free(blob);
		block_fini(service_id);
		return rc;
	}
	blob->service_id = service_id;
	blob->size = size;
	*out = bithenge_blob_as_node(block_as_blob(blob));

	return EOK;
}

/** @}
 */
