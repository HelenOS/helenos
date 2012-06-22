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
 * Transforms.
 */

#include <byteorder.h>
#include <errno.h>
#include "blob.h"
#include "transform.h"

static int transform_indestructible(bithenge_transform_t *xform)
{
	return EINVAL;
}

static int uint32le_apply(bithenge_transform_t *xform, bithenge_node_t *in,
    bithenge_node_t **out)
{
	int rc;
	if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)
		return EINVAL;
	bithenge_blob_t *blob = bithenge_node_as_blob(in);

	// Try to read 5 bytes and fail if the blob is too long.
	uint32_t val[2];
	aoff64_t size = sizeof(val[0]) + 1;
	rc = bithenge_blob_read(blob, 0, (char *)val, &size);
	if (rc != EOK)
		return rc;
	if (size != 4)
		return EINVAL;

	return bithenge_new_integer_node(out, uint32_t_le2host(val[0]));
}

static int uint32le_prefix_length(bithenge_transform_t *xform,
    bithenge_blob_t *blob, aoff64_t *out)
{
	*out = 4;
	return EOK;
}

static const bithenge_transform_ops_t uint32le_ops = {
	.apply = uint32le_apply,
	.prefix_length = uint32le_prefix_length,
	.destroy = transform_indestructible,
};

static bithenge_transform_t uint32le_transform = {
	&uint32le_ops, 1
};

/** Create a little-endian 32-bit unsigned integer transform.
 * @param out Holds the transform.
 * @return EOK on success or an error code from errno.h. */
int bithenge_uint32le_transform(bithenge_transform_t **out)
{
	*out = &uint32le_transform;
	return EOK;
}

/** @}
 */
