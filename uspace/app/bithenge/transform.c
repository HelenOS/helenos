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
 * @param[out] self Transform to initialize.
 * @param[in] ops Operations provided by the transform.
 * @param num_params The number of parameters required. If this is nonzero, the
 * transform will get its own context with parameters, probably provided by a
 * param_wrapper. If this is zero, the existing outer context will be used with
 * whatever parameters it has, so they can be passed to any param_wrappers
 * within.
 * @return EOK or an error code from errno.h. */
int bithenge_init_transform(bithenge_transform_t *self,
    const bithenge_transform_ops_t *ops, int num_params)
{
	assert(ops);
	assert(ops->apply);
	assert(ops->destroy);
	self->ops = ops;
	self->refs = 1;
	self->num_params = num_params;
	return EOK;
}

static void transform_indestructible(bithenge_transform_t *self)
{
	assert(false);
}

typedef struct {
	bithenge_transform_t base;
	bithenge_transform_t *transform;
} param_transform_t;

static inline param_transform_t *transform_as_param(
    bithenge_transform_t *base)
{
	return (param_transform_t *)base;
}

static inline bithenge_transform_t *param_as_transform(
    param_transform_t *self)
{
	return &self->base;
}

static int param_transform_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	param_transform_t *self = transform_as_param(base);
	return bithenge_transform_apply(self->transform, scope, in, out);
}

static int param_transform_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *in, aoff64_t *out)
{
	param_transform_t *self = transform_as_param(base);
	return bithenge_transform_prefix_length(self->transform, scope, in,
	    out);
}

static void param_transform_destroy(bithenge_transform_t *base)
{
	param_transform_t *self = transform_as_param(base);
	bithenge_transform_dec_ref(self->transform);
	free(self);
}

static const bithenge_transform_ops_t param_transform_ops = {
	.apply = param_transform_apply,
	.prefix_length = param_transform_prefix_length,
	.destroy = param_transform_destroy,
};

/** Create a wrapper transform with a different number of parameters. Takes a
 * reference to @a transform, which it will use for all operations.
 * @param[out] out Holds the created transform.
 * @param transform The transform to wrap.
 * @param num_params The number of parameters to require.
 * @return EOK on success or an error code from errno.h. */
int bithenge_new_param_transform(bithenge_transform_t **out,
    bithenge_transform_t *transform, int num_params)
{
	assert(transform);
	assert(bithenge_transform_num_params(transform) == 0);
	assert(num_params != 0);

	int rc;
	param_transform_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}
	rc = bithenge_init_transform(param_as_transform(self),
	    &param_transform_ops, num_params);
	if (rc != EOK)
		goto error;
	self->transform = transform;
	*out = param_as_transform(self);
	return EOK;
error:
	bithenge_transform_dec_ref(transform);
	free(self);
	return rc;
}

static int ascii_apply(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_node_t *in, bithenge_node_t **out)
{
	int rc;
	if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)
		return EINVAL;
	bithenge_blob_t *blob = bithenge_node_as_blob(in);
	aoff64_t size;
	rc = bithenge_blob_size(blob, &size);
	if (rc != EOK)
		return rc;

	char *buffer = malloc(size + 1);
	if (!buffer)
		return ENOMEM;
	aoff64_t size_read = size;
	rc = bithenge_blob_read(blob, 0, buffer, &size_read);
	if (rc != EOK) {
		free(buffer);
		return rc;
	}
	if (size_read != size) {
		free(buffer);
		return EINVAL;
	}
	buffer[size] = '\0';

	/* TODO: what if the OS encoding is incompatible with ASCII? */
	return bithenge_new_string_node(out, buffer, true);
}

static const bithenge_transform_ops_t ascii_ops = {
	.apply = ascii_apply,
	.destroy = transform_indestructible,
};

/** The ASCII text transform. */
bithenge_transform_t bithenge_ascii_transform = {
	&ascii_ops, 1, 0
};

static int known_length_apply(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	bithenge_node_t *length_node;
	int rc = bithenge_scope_get_param(scope, 0, &length_node);
	if (rc != EOK)
		return rc;
	if (bithenge_node_type(length_node) != BITHENGE_NODE_INTEGER) {
		bithenge_node_dec_ref(length_node);
		return EINVAL;
	}
	bithenge_int_t length = bithenge_integer_node_value(length_node);
	bithenge_node_dec_ref(length_node);

	if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)
		return EINVAL;
	aoff64_t size;
	rc = bithenge_blob_size(bithenge_node_as_blob(in), &size);
	if (rc != EOK)
		return rc;
	if (length != (bithenge_int_t)size)
		return EINVAL;

	bithenge_node_inc_ref(in);
	*out = in;
	return EOK;
}

static int known_length_prefix_length(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_blob_t *in, aoff64_t *out)
{
	bithenge_node_t *length_node;
	int rc = bithenge_scope_get_param(scope, 0, &length_node);
	if (rc != EOK)
		return rc;
	if (bithenge_node_type(length_node) != BITHENGE_NODE_INTEGER) {
		bithenge_node_dec_ref(length_node);
		return EINVAL;
	}
	bithenge_int_t length = bithenge_integer_node_value(length_node);
	bithenge_node_dec_ref(length_node);

	*out = (aoff64_t)length;
	return EOK;
}

static const bithenge_transform_ops_t known_length_ops = {
	.apply = known_length_apply,
	.prefix_length = known_length_prefix_length,
	.destroy = transform_indestructible,
};

/** Pass through a blob, but require its length to equal the first argument. */
bithenge_transform_t bithenge_known_length_transform = {
	&known_length_ops, 1, 1
};

static int prefix_length_1(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_blob_t *blob, aoff64_t *out)
{
	*out = 1;
	return EOK;
}

static int prefix_length_2(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_blob_t *blob, aoff64_t *out)
{
	*out = 2;
	return EOK;
}

static int prefix_length_4(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_blob_t *blob, aoff64_t *out)
{
	*out = 4;
	return EOK;
}

static int prefix_length_8(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_blob_t *blob, aoff64_t *out)
{
	*out = 8;
	return EOK;
}

#define MAKE_UINT_TRANSFORM(NAME, TYPE, ENDIAN, PREFIX_LENGTH_FUNC)            \
	static int NAME##_apply(bithenge_transform_t *self,                    \
	    bithenge_scope_t *scope, bithenge_node_t *in,                      \
	    bithenge_node_t **out)                                             \
	{                                                                      \
		int rc;                                                        \
		if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)              \
			return EINVAL;                                         \
		bithenge_blob_t *blob = bithenge_node_as_blob(in);             \
		                                                               \
		/* Read too many bytes; success means the blob is too long. */ \
		TYPE val[2];                                                   \
		aoff64_t size = sizeof(val[0]) + 1;                            \
		rc = bithenge_blob_read(blob, 0, (char *)val, &size);          \
		if (rc != EOK)                                                 \
			return rc;                                             \
		if (size != sizeof(val[0]))                                    \
			return EINVAL;                                         \
		                                                               \
		return bithenge_new_integer_node(out, ENDIAN(val[0]));         \
	}                                                                      \
	                                                                       \
	static const bithenge_transform_ops_t NAME##_ops = {                   \
		.apply = NAME##_apply,                                         \
		.prefix_length = PREFIX_LENGTH_FUNC,                           \
		.destroy = transform_indestructible,                           \
	};                                                                     \
	                                                                       \
	bithenge_transform_t bithenge_##NAME##_transform = {                   \
		&NAME##_ops, 1, 0                                              \
	}

MAKE_UINT_TRANSFORM(uint8   , uint8_t ,                 , prefix_length_1);
MAKE_UINT_TRANSFORM(uint16le, uint16_t, uint16_t_le2host, prefix_length_2);
MAKE_UINT_TRANSFORM(uint16be, uint16_t, uint16_t_be2host, prefix_length_2);
MAKE_UINT_TRANSFORM(uint32le, uint32_t, uint32_t_le2host, prefix_length_4);
MAKE_UINT_TRANSFORM(uint32be, uint32_t, uint32_t_be2host, prefix_length_4);
MAKE_UINT_TRANSFORM(uint64le, uint64_t, uint32_t_le2host, prefix_length_8);
MAKE_UINT_TRANSFORM(uint64be, uint64_t, uint32_t_be2host, prefix_length_8);

static int zero_terminated_apply(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	int rc;
	if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)
		return EINVAL;
	bithenge_blob_t *blob = bithenge_node_as_blob(in);
	aoff64_t size;
	rc = bithenge_blob_size(blob, &size);
	if (rc != EOK)
		return rc;
	if (size < 1)
		return EINVAL;
	char ch;
	aoff64_t size_read = 1;
	rc = bithenge_blob_read(blob, size - 1, &ch, &size_read);
	if (rc != EOK)
		return rc;
	if (size_read != 1 || ch != '\0')
		return EINVAL;
	bithenge_blob_inc_ref(blob);
	return bithenge_new_subblob(out, blob, 0, size - 1);
}

static int zero_terminated_prefix_length(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_blob_t *blob, aoff64_t *out)
{
	int rc;
	char buffer[4096];
	aoff64_t offset = 0, size_read = sizeof(buffer);
	do {
		rc = bithenge_blob_read(blob, offset, buffer, &size_read);
		if (rc != EOK)
			return rc;
		char *found = memchr(buffer, '\0', size_read);
		if (found) {
			*out = found - buffer + offset + 1;
			return EOK;
		}
		offset += size_read;
	} while (size_read == sizeof(buffer));
	return EINVAL;
}

static const bithenge_transform_ops_t zero_terminated_ops = {
	.apply = zero_terminated_apply,
	.prefix_length = zero_terminated_prefix_length,
	.destroy = transform_indestructible,
};

/** The zero-terminated data transform. */
bithenge_transform_t bithenge_zero_terminated_transform = {
	&zero_terminated_ops, 1, 0
};

static bithenge_named_transform_t primitive_transforms[] = {
	{"ascii", &bithenge_ascii_transform},
	{"known_length", &bithenge_known_length_transform},
	{"uint8", &bithenge_uint8_transform},
	{"uint16le", &bithenge_uint16le_transform},
	{"uint16be", &bithenge_uint16be_transform},
	{"uint32le", &bithenge_uint32le_transform},
	{"uint32be", &bithenge_uint32be_transform},
	{"uint64le", &bithenge_uint64le_transform},
	{"uint64be", &bithenge_uint64be_transform},
	{"zero_terminated", &bithenge_zero_terminated_transform},
	{NULL, NULL}
};

/** An array of named built-in transforms. */
bithenge_named_transform_t *bithenge_primitive_transforms = primitive_transforms;

typedef struct {
	bithenge_node_t base;
	struct struct_transform *transform;
	bithenge_scope_t *scope;
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
    bithenge_transform_t *subxform, bithenge_scope_t *scope,
    bithenge_blob_t **blob, bithenge_for_each_func_t func, void *data)
{
	int rc;
	bithenge_node_t *subxform_result = NULL;

	aoff64_t sub_size;
	rc = bithenge_transform_prefix_length(subxform, scope, *blob,
	    &sub_size);
	if (rc != EOK)
		goto error;

	bithenge_node_t *subblob_node;
	bithenge_blob_inc_ref(*blob);
	rc = bithenge_new_subblob(&subblob_node, *blob, 0, sub_size);
	if (rc != EOK)
		goto error;

	rc = bithenge_transform_apply(subxform, scope, subblob_node,
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
		    subxforms[i].transform, struct_node->scope, &blob, func,
		    data);
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

static void struct_node_destroy(bithenge_node_t *base)
{
	struct_node_t *node = node_as_struct(base);
	bithenge_transform_dec_ref(struct_as_transform(node->transform));
	bithenge_blob_dec_ref(node->blob);
	free(node);
}

static const bithenge_internal_node_ops_t struct_node_ops = {
	.for_each = struct_node_for_each,
	.destroy = struct_node_destroy,
};

static int struct_transform_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	struct_transform_t *self = transform_as_struct(base);
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
	bithenge_transform_inc_ref(base);
	node->transform = self;
	bithenge_node_inc_ref(in);
	node->scope = scope;
	node->blob = bithenge_node_as_blob(in);
	*out = struct_as_node(node);
	return EOK;
}

static int struct_transform_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *blob, aoff64_t *out)
{
	struct_transform_t *self = transform_as_struct(base);
	int rc = EOK;
	bithenge_node_t *node;
	bithenge_blob_inc_ref(blob);
	rc = bithenge_new_offset_blob(&node, blob, 0);
	blob = NULL;
	if (rc != EOK)
		goto error;
	blob = bithenge_node_as_blob(node);
	*out = 0;
	for (size_t i = 0; self->subtransforms[i].transform; i++) {
		bithenge_transform_t *subxform =
		    self->subtransforms[i].transform;
		aoff64_t sub_size;
		rc = bithenge_transform_prefix_length(subxform, scope, blob,
		    &sub_size);
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

static void struct_transform_destroy(bithenge_transform_t *base)
{
	struct_transform_t *self = transform_as_struct(base);
	free_subtransforms(self->subtransforms);
	free(self);
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
	struct_transform_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}
	rc = bithenge_init_transform(struct_as_transform(self),
	    &struct_transform_ops, 0);
	if (rc != EOK)
		goto error;
	self->subtransforms = subtransforms;
	*out = struct_as_transform(self);
	return EOK;
error:
	free_subtransforms(subtransforms);
	free(self);
	return rc;
}

typedef struct {
	bithenge_transform_t base;
	bithenge_transform_t **xforms;
	size_t num;
} compose_transform_t;

static bithenge_transform_t *compose_as_transform(compose_transform_t *xform)
{
	return &xform->base;
}

static compose_transform_t *transform_as_compose(bithenge_transform_t *xform)
{
	return (compose_transform_t *)xform;
}

static int compose_apply(bithenge_transform_t *base, bithenge_scope_t *scope,
    bithenge_node_t *in, bithenge_node_t **out)
{
	int rc;
	compose_transform_t *self = transform_as_compose(base);
	bithenge_node_inc_ref(in);

	/* i ranges from (self->num - 1) to 0 inside the loop. */
	for (size_t i = self->num; i--; ) {
		bithenge_node_t *tmp;
		rc = bithenge_transform_apply(self->xforms[i], scope, in,
		    &tmp);
		bithenge_node_dec_ref(in);
		if (rc != EOK)
			return rc;
		in = tmp;
	}

	*out = in;
	return rc;
}

static int compose_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *blob, aoff64_t *out)
{
	compose_transform_t *self = transform_as_compose(base);
	return bithenge_transform_prefix_length(self->xforms[self->num - 1],
	    scope, blob, out);
}

static void compose_destroy(bithenge_transform_t *base)
{
	compose_transform_t *self = transform_as_compose(base);
	for (size_t i = 0; i < self->num; i++)
		bithenge_transform_dec_ref(self->xforms[i]);
	free(self->xforms);
	free(self);
}

static const bithenge_transform_ops_t compose_transform_ops = {
	.apply = compose_apply,
	.prefix_length = compose_prefix_length,
	.destroy = compose_destroy,
};

/** Create a composition of multiple transforms. When the result is applied to a
 * node, each transform is applied in turn, with the last transform applied
 * first. @a xforms may contain any number of transforms or no transforms at
 * all. This function takes ownership of @a xforms and the references therein.
 * @param[out] out Holds the result.
 * @param[in] xforms The transforms to apply.
 * @param num The number of transforms.
 * @return EOK on success or an error code from errno.h. */
int bithenge_new_composed_transform(bithenge_transform_t **out,
    bithenge_transform_t **xforms, size_t num)
{
	if (num == 0) {
		/* TODO: optimize */
	} else if (num == 1) {
		*out = xforms[0];
		free(xforms);
		return EOK;
	}

	int rc;
	compose_transform_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}
	rc = bithenge_init_transform(compose_as_transform(self),
	    &compose_transform_ops, 0);
	if (rc != EOK)
		goto error;
	self->xforms = xforms;
	self->num = num;
	*out = compose_as_transform(self);
	return EOK;
error:
	for (size_t i = 0; i < num; i++)
		bithenge_transform_dec_ref(xforms[i]);
	free(xforms);
	free(self);
	return rc;
}

/** @}
 */
