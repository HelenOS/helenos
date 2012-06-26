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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "blob.h"
#include "transform.h"

/** Initialize a new transform.
 * @param[out] xform Transform to initialize.
 * @param[in] ops Operations provided by the transform.
 * @return EOK or an error code from errno.h. */
int bithenge_new_transform(bithenge_transform_t *xform,
    const bithenge_transform_ops_t *ops)
{
	assert(ops);
	assert(ops->apply);
	assert(ops->destroy);
	xform->ops = ops;
	xform->refs = 1;
	return EOK;
}

static int transform_indestructible(bithenge_transform_t *xform)
{
	assert(false);
	return EOK;
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

static int uint32be_apply(bithenge_transform_t *xform, bithenge_node_t *in,
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

	return bithenge_new_integer_node(out, uint32_t_be2host(val[0]));
}

static int prefix_length_4(bithenge_transform_t *xform, bithenge_blob_t *blob,
    aoff64_t *out)
{
	*out = 4;
	return EOK;
}

static const bithenge_transform_ops_t uint32le_ops = {
	.apply = uint32le_apply,
	.prefix_length = prefix_length_4,
	.destroy = transform_indestructible,
};

static const bithenge_transform_ops_t uint32be_ops = {
	.apply = uint32be_apply,
	.prefix_length = prefix_length_4,
	.destroy = transform_indestructible,
};

/** The little-endian 32-bit unsigned integer transform. */
bithenge_transform_t bithenge_uint32le_transform = {
	&uint32le_ops, 1
};

/** The big-endian 32-bit unsigned integer transform. */
bithenge_transform_t bithenge_uint32be_transform = {
	&uint32be_ops, 1
};

static bithenge_named_transform_t primitive_transforms[] = {
	{"uint32le", &bithenge_uint32le_transform},
	{"uint32be", &bithenge_uint32be_transform},
	{NULL, NULL}
};

/** An array of named built-in transforms. */
bithenge_named_transform_t *bithenge_primitive_transforms = primitive_transforms;

typedef struct {
	bithenge_node_t base;
	struct struct_transform *transform;
	bithenge_blob_t *blob;
} struct_node_t;

typedef struct struct_transform {
	bithenge_transform_t base;
	bithenge_named_transform_t *subtransforms;
} struct_transform_t;

static bithenge_node_t *struct_as_node(struct_node_t *node)
{
	return &node->base;
}

static struct_node_t *node_as_struct(bithenge_node_t *node)
{
	return (struct_node_t *)node;
}

static bithenge_transform_t *struct_as_transform(struct_transform_t *xform)
{
	return &xform->base;
}

static struct_transform_t *transform_as_struct(bithenge_transform_t *xform)
{
	return (struct_transform_t *)xform;
}

static int struct_node_for_one(const char *name,
    bithenge_transform_t *subxform, bithenge_blob_t **blob,
    bithenge_for_each_func_t func, void *data)
{
	int rc;
	bithenge_node_t *subxform_result = NULL;

	aoff64_t sub_size;
	rc = bithenge_transform_prefix_length(subxform, *blob, &sub_size);
	if (rc != EOK)
		goto error;

	bithenge_node_t *subblob_node;
	bithenge_blob_inc_ref(*blob);
	rc = bithenge_new_subblob(&subblob_node, *blob, 0, sub_size);
	if (rc != EOK)
		goto error;

	rc = bithenge_transform_apply(subxform, subblob_node,
	    &subxform_result);
	bithenge_node_dec_ref(subblob_node);
	if (rc != EOK)
		goto error;

	if (name) {
		bithenge_node_t *name_node;
		rc = bithenge_new_string_node(&name_node, name, false);
		if (rc != EOK)
			goto error;
		rc = func(name_node, subxform_result, data);
		subxform_result = NULL;
		if (rc != EOK)
			goto error;
	} else {
		if (bithenge_node_type(subxform_result) !=
		    BITHENGE_NODE_INTERNAL) {
			rc = EINVAL;
			goto error;
		}
		rc = bithenge_node_for_each(subxform_result, func, data);
		if (rc != EOK)
			goto error;
	}

	bithenge_node_t *blob_node;
	rc = bithenge_new_offset_blob(&blob_node, *blob, sub_size);
	*blob = NULL;
	if (rc != EOK)
		goto error;
	*blob = bithenge_node_as_blob(blob_node);

error:
	bithenge_node_dec_ref(subxform_result);
	return rc;
}

static int struct_node_for_each(bithenge_node_t *base,
    bithenge_for_each_func_t func, void *data)
{
	int rc = EOK;
	struct_node_t *struct_node = node_as_struct(base);
	bithenge_named_transform_t *subxforms =
	    struct_node->transform->subtransforms;

	bithenge_node_t *blob_node = NULL;
	bithenge_blob_t *blob = NULL;
	bithenge_blob_inc_ref(struct_node->blob);
	rc = bithenge_new_offset_blob(&blob_node, struct_node->blob, 0);
	if (rc != EOK) {
		blob = NULL;
		goto error;
	}
	blob = bithenge_node_as_blob(blob_node);

	for (size_t i = 0; subxforms[i].transform; i++) {
		rc = struct_node_for_one(subxforms[i].name,
		    subxforms[i].transform, &blob, func, data);
		if (rc != EOK)
			goto error;
	}

	aoff64_t remaining;
	rc = bithenge_blob_size(blob, &remaining);
	if (rc != EOK)
		goto error;
	if (remaining != 0) {
		rc = EINVAL;
		goto error;
	}

error:
	bithenge_blob_dec_ref(blob);
	return rc;
}

static int struct_node_destroy(bithenge_node_t *base)
{
	struct_node_t *node = node_as_struct(base);
	bithenge_transform_dec_ref(struct_as_transform(node->transform));
	bithenge_blob_dec_ref(node->blob);
	free(node);
	return EOK;
}

static const bithenge_internal_node_ops_t struct_node_ops = {
	.for_each = struct_node_for_each,
	.destroy = struct_node_destroy,
};

static int struct_transform_apply(bithenge_transform_t *xform,
    bithenge_node_t *in, bithenge_node_t **out)
{
	struct_transform_t *struct_transform = transform_as_struct(xform);
	if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)
		return EINVAL;
	struct_node_t *node = malloc(sizeof(*node));
	if (!node)
		return ENOMEM;
	int rc = bithenge_init_internal_node(struct_as_node(node),
	    &struct_node_ops);
	if (rc != EOK) {
		free(node);
		return rc;
	}
	bithenge_transform_inc_ref(xform);
	node->transform = struct_transform;
	bithenge_node_inc_ref(in);
	node->blob = bithenge_node_as_blob(in);
	*out = struct_as_node(node);
	return EOK;
}

static int struct_transform_prefix_length(bithenge_transform_t *xform,
    bithenge_blob_t *blob, aoff64_t *out)
{
	struct_transform_t *struct_transform = transform_as_struct(xform);
	int rc = EOK;
	bithenge_node_t *node;
	bithenge_blob_inc_ref(blob);
	rc = bithenge_new_offset_blob(&node, blob, 0);
	blob = NULL;
	if (rc != EOK)
		goto error;
	blob = bithenge_node_as_blob(node);
	*out = 0;
	for (size_t i = 0; struct_transform->subtransforms[i].transform; i++) {
		bithenge_transform_t *subxform =
		    struct_transform->subtransforms[i].transform;
		aoff64_t sub_size;
		rc = bithenge_transform_prefix_length(subxform, blob, &sub_size);
		if (rc != EOK)
			goto error;
		*out += sub_size;
		rc = bithenge_new_offset_blob(&node, blob, sub_size);
		blob = NULL;
		if (rc != EOK)
			goto error;
		blob = bithenge_node_as_blob(node);
	}
error:
	bithenge_blob_dec_ref(blob);
	return EOK;
}

static void free_subtransforms(bithenge_named_transform_t *subtransforms)
{
	for (size_t i = 0; subtransforms[i].transform; i++) {
		free((void *)subtransforms[i].name);
		bithenge_transform_dec_ref(subtransforms[i].transform);
	}
	free(subtransforms);
}

static int struct_transform_destroy(bithenge_transform_t *xform)
{
	struct_transform_t *struct_transform = transform_as_struct(xform);
	free_subtransforms(struct_transform->subtransforms);
	free(struct_transform);
	return EOK;
}

static bithenge_transform_ops_t struct_transform_ops = {
	.apply = struct_transform_apply,
	.prefix_length = struct_transform_prefix_length,
	.destroy = struct_transform_destroy,
};

/** Create a struct transform. The transform will apply its subtransforms
 * sequentially to a blob to create an internal node. Each result is either
 * given a key from @a subtransforms or, if the name is NULL, the result's keys
 * and values are merged into the struct transform's result. This function
 * takes ownership of @a subtransforms and the names and references therein.
 * @param[out] out Stores the created transform.
 * @param subtransforms The subtransforms and field names.
 * @return EOK on success or an error code from errno.h. */
int bithenge_new_struct(bithenge_transform_t **out,
    bithenge_named_transform_t *subtransforms)
{
	int rc;
	struct_transform_t *struct_transform =
	    malloc(sizeof(*struct_transform));
	if (!struct_transform) {
		rc = ENOMEM;
		goto error;
	}
	rc = bithenge_new_transform(struct_as_transform(struct_transform),
	    &struct_transform_ops);
	if (rc != EOK)
		goto error;
	struct_transform->subtransforms = subtransforms;
	*out = struct_as_transform(struct_transform);
	return EOK;
error:
	free_subtransforms(subtransforms);
	free(struct_transform);
	return rc;
}

/** @}
 */
