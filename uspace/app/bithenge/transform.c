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
	assert(ops->apply || ops->prefix_apply);
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

/** Apply a transform. Takes ownership of nothing.
 * @memberof bithenge_transform_t
 * @param self The transform.
 * @param scope The scope.
 * @param in The input tree.
 * @param[out] out Where the output tree will be stored.
 * @return EOK on success or an error code from errno.h. */
int bithenge_transform_apply(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	assert(self);
	assert(self->ops);
	if (self->ops->apply)
		return self->ops->apply(self, scope, in, out);

	if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)
		return EINVAL;
	aoff64_t self_size, whole_size;
	int rc = bithenge_transform_prefix_apply(self, scope,
	    bithenge_node_as_blob(in), out, &self_size);
	if (rc != EOK)
		return rc;
	rc = bithenge_blob_size(bithenge_node_as_blob(in), &whole_size);
	if (rc == EOK && whole_size != self_size)
		rc = EINVAL;
	if (rc != EOK) {
		bithenge_node_dec_ref(*out);
		return rc;
	}
	return EOK;
}

/** Find the length of the prefix of a blob this transform can use as input. In
 * other words, figure out how many bytes this transform will use up.  This
 * method is optional and can return an error, but it must succeed for struct
 * subtransforms. Takes ownership of nothing.
 * @memberof bithenge_transform_t
 * @param self The transform.
 * @param scope The scope.
 * @param blob The blob.
 * @param[out] out Where the prefix length will be stored.
 * @return EOK on success, ENOTSUP if not supported, or another error code from
 * errno.h. */
int bithenge_transform_prefix_length(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_blob_t *blob, aoff64_t *out)
{
	assert(self);
	assert(self->ops);
	if (self->ops->prefix_length)
		return self->ops->prefix_length(self, scope, blob, out);
	if (!self->ops->prefix_apply)
		return ENOTSUP;

	bithenge_node_t *node;
	int rc = bithenge_transform_prefix_apply(self, scope, blob, &node,
	    out);
	if (rc != EOK)
		return rc;
	bithenge_node_dec_ref(node);
	return EOK;
}

/** Apply this transform to a prefix of a blob. In other words, feed as much of
 * the blob into this transform as possible. Takes ownership of nothing.
 * @memberof bithenge_transform_t
 * @param self The transform.
 * @param scope The scope.
 * @param blob The blob.
 * @param[out] out_node Holds the result of applying this transform to the
 * prefix.
 * @param[out] out_size Holds the size of the prefix.
 * @return EOK on success, ENOTSUP if not supported, or another error code from
 * errno.h. */
int bithenge_transform_prefix_apply(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_blob_t *blob, bithenge_node_t **out_node,
    aoff64_t *out_size)
{
	assert(self);
	assert(self->ops);
	if (self->ops->prefix_apply)
		return self->ops->prefix_apply(self, scope, blob, out_node,
		    out_size);
	if (!self->ops->prefix_length)
		return ENOTSUP;

	int rc = bithenge_transform_prefix_length(self, scope, blob, out_size);
	if (rc != EOK)
		return rc;
	bithenge_node_t *prefix_blob;
	bithenge_blob_inc_ref(blob);
	rc = bithenge_new_subblob(&prefix_blob, blob, 0, *out_size);
	if (rc != EOK)
		return rc;
	rc = bithenge_transform_apply(self, scope, prefix_blob, out_node);
	bithenge_node_dec_ref(prefix_blob);
	return rc;
}

/** Initialize a transform scope. It must be destroyed with @a
 * bithenge_scope_destroy after it is used.
 * @param[out] scope The scope to initialize. */
void bithenge_scope_init(bithenge_scope_t *scope)
{
	scope->num_params = 0;
	scope->params = NULL;
	scope->current_node = NULL;
}

/** Destroy a transform scope.
 * @param scope The scope to destroy.
 * @return EOK on success or an error code from errno.h. */
void bithenge_scope_destroy(bithenge_scope_t *scope)
{
	bithenge_node_dec_ref(scope->current_node);
	for (int i = 0; i < scope->num_params; i++)
		bithenge_node_dec_ref(scope->params[i]);
	free(scope->params);
}

/** Copy a scope.
 * @param[out] out The scope to fill in; must have been initialized with @a
 * bithenge_scope_init.
 * @param scope The scope to copy.
 * @return EOK on success or an error code from errno.h. */
int bithenge_scope_copy(bithenge_scope_t *out, bithenge_scope_t *scope)
{
	out->params = malloc(sizeof(*out->params) * scope->num_params);
	if (!out->params)
		return ENOMEM;
	memcpy(out->params, scope->params, sizeof(*out->params) *
	    scope->num_params);
	out->num_params = scope->num_params;
	for (int i = 0; i < out->num_params; i++)
		bithenge_node_inc_ref(out->params[i]);
	out->current_node = scope->current_node;
	if (out->current_node)
		bithenge_node_inc_ref(out->current_node);
	return EOK;
}

/** Set the current node being created. Takes a reference to @a node.
 * @param scope The scope to set the current node in.
 * @param node The current node being created, or NULL.
 * @return EOK on success or an error code from errno.h. */
void bithenge_scope_set_current_node(bithenge_scope_t *scope,
    bithenge_node_t *node)
{
	bithenge_node_dec_ref(scope->current_node);
	scope->current_node = node;
}

/** Get the current node being created, which may be NULL.
 * @param scope The scope to get the current node from.
 * @return The node being created, or NULL. */
bithenge_node_t *bithenge_scope_get_current_node(bithenge_scope_t *scope)
{
	if (scope->current_node)
		bithenge_node_inc_ref(scope->current_node);
	return scope->current_node;
}

/** Allocate parameters. The parameters must then be set with @a
 * bithenge_scope_set_param. This must not be called on a scope that already
 * has parameters.
 * @param scope The scope in which to allocate parameters.
 * @param num_params The number of parameters to allocate.
 * @return EOK on success or an error code from errno.h. */
int bithenge_scope_alloc_params(bithenge_scope_t *scope, int num_params)
{
	scope->params = malloc(sizeof(*scope->params) * num_params);
	if (!scope->params)
		return ENOMEM;
	scope->num_params = num_params;
	for (int i = 0; i < num_params; i++)
		scope->params[i] = NULL;
	return EOK;
}

/** Set a parameter. Takes a reference to @a value. Note that range checking is
 * not done in release builds.
 * @param scope The scope in which to allocate parameters.
 * @param i The index of the parameter to set.
 * @param value The value to store in the parameter.
 * @return EOK on success or an error code from errno.h. */
int bithenge_scope_set_param( bithenge_scope_t *scope, int i,
    bithenge_node_t *node)
{
	assert(scope);
	assert(i >= 0 && i < scope->num_params);
	scope->params[i] = node;
	return EOK;
}

/** Get a parameter. Note that range checking is not done in release builds.
 * @param scope The scope to get the parameter from.
 * @param i The index of the parameter to set.
 * @param[out] out Stores a new reference to the parameter.
 * @return EOK on success or an error code from errno.h. */
int bithenge_scope_get_param(bithenge_scope_t *scope, int i,
    bithenge_node_t **out)
{
	assert(scope);
	assert(i >= 0 && i < scope->num_params);
	*out = scope->params[i];
	bithenge_node_inc_ref(*out);
	return EOK;
}

typedef struct {
	bithenge_transform_t base;
	bithenge_transform_t *transform;
} scope_transform_t;

static inline scope_transform_t *transform_as_param(
    bithenge_transform_t *base)
{
	return (scope_transform_t *)base;
}

static inline bithenge_transform_t *param_as_transform(
    scope_transform_t *self)
{
	return &self->base;
}

static int scope_transform_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	scope_transform_t *self = transform_as_param(base);
	bithenge_scope_t inner_scope;
	bithenge_scope_init(&inner_scope);
	int rc = bithenge_scope_copy(&inner_scope, scope);
	if (rc != EOK)
		goto error;
	bithenge_scope_set_current_node(&inner_scope, NULL);
	rc = bithenge_transform_apply(self->transform, scope, in, out);
error:
	bithenge_scope_destroy(&inner_scope);
	return rc;
}

static int scope_transform_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *in, aoff64_t *out)
{
	scope_transform_t *self = transform_as_param(base);
	return bithenge_transform_prefix_length(self->transform, scope, in,
	    out);
}

static void scope_transform_destroy(bithenge_transform_t *base)
{
	scope_transform_t *self = transform_as_param(base);
	bithenge_transform_dec_ref(self->transform);
	free(self);
}

static const bithenge_transform_ops_t scope_transform_ops = {
	.apply = scope_transform_apply,
	.prefix_length = scope_transform_prefix_length,
	.destroy = scope_transform_destroy,
};

/** Create a wrapper transform that creates a new outer scope. This ensures
 * nothing from the transform's users is passed in, other than parameters. The
 * wrapper may have a different value for num_params. Takes a reference to
 * @a transform, which it will use for all operations.
 * @param[out] out Holds the created transform.
 * @param transform The transform to wrap.
 * @param num_params The number of parameters to require, which may be 0.
 * @return EOK on success or an error code from errno.h. */
int bithenge_new_scope_transform(bithenge_transform_t **out,
    bithenge_transform_t *transform, int num_params)
{
	assert(transform);
	assert(bithenge_transform_num_params(transform) == 0);

	int rc;
	scope_transform_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}
	rc = bithenge_init_transform(param_as_transform(self),
	    &scope_transform_ops, num_params);
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

static int invalid_apply(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_node_t *in, bithenge_node_t **out)
{
	return EINVAL;
}

static const bithenge_transform_ops_t invalid_ops = {
	.apply = invalid_apply,
	.destroy = transform_indestructible,
};

/** A transform that always raises an error. */
bithenge_transform_t bithenge_invalid_transform = {
	&invalid_ops, 1, 0
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
	bithenge_transform_t base;
	bithenge_named_transform_t *subtransforms;
	size_t num_subtransforms;
} struct_transform_t;

typedef struct {
	bithenge_node_t base;
	bithenge_scope_t scope;
	struct_transform_t *transform;
	bithenge_blob_t *blob;
	aoff64_t *ends;
	size_t num_ends;
	bool prefix;
} struct_node_t;

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

static int struct_node_field_offset(struct_node_t *self, aoff64_t *out,
    size_t index)
{
	if (index == 0) {
		*out = 0;
		return EOK;
	}
	index--;
	aoff64_t prev_offset =
	    self->num_ends ? self->ends[self->num_ends - 1] : 0;
	for (; self->num_ends <= index; self->num_ends++) {
		bithenge_node_t *subblob_node;
		bithenge_blob_inc_ref(self->blob);
		int rc = bithenge_new_offset_blob(&subblob_node, self->blob,
		    prev_offset);
		if (rc != EOK)
			return rc;

		bithenge_blob_t *subblob = bithenge_node_as_blob(subblob_node);
		aoff64_t field_size;
		rc = bithenge_transform_prefix_length(
		    self->transform->subtransforms[self->num_ends].transform,
		    &self->scope, subblob, &field_size);
		bithenge_node_dec_ref(subblob_node);
		if (rc != EOK)
			return rc;

		prev_offset = self->ends[self->num_ends] =
		    prev_offset + field_size;
	}
	*out = self->ends[index];
	return EOK;
}

static int struct_node_subtransform(struct_node_t *self, bithenge_node_t **out,
    size_t index)
{
	aoff64_t start_pos;
	int rc = struct_node_field_offset(self, &start_pos, index);
	if (rc != EOK)
		return rc;

	if (index == self->num_ends) {
		/* We can apply the subtransform and cache its prefix length at
		 * the same time. */
		bithenge_node_t *blob_node;
		bithenge_blob_inc_ref(self->blob);
		rc = bithenge_new_offset_blob(&blob_node, self->blob,
		    start_pos);
		if (rc != EOK)
			return rc;

		aoff64_t size;
		rc = bithenge_transform_prefix_apply(
		    self->transform->subtransforms[index].transform,
		    &self->scope, bithenge_node_as_blob(blob_node), out,
		    &size);
		bithenge_node_dec_ref(blob_node);
		if (rc != EOK)
			return rc;

		self->ends[self->num_ends++] = start_pos + size;
	} else {
		aoff64_t end_pos;
		int rc = struct_node_field_offset(self, &end_pos, index + 1);
		if (rc != EOK)
			return rc;

		bithenge_node_t *blob_node;
		bithenge_blob_inc_ref(self->blob);
		rc = bithenge_new_subblob(&blob_node, self->blob, start_pos,
		    end_pos - start_pos);
		if (rc != EOK)
			return rc;

		rc = bithenge_transform_apply(
		    self->transform->subtransforms[index].transform,
		    &self->scope, blob_node, out);
		bithenge_node_dec_ref(blob_node);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

static int struct_node_for_each(bithenge_node_t *base,
    bithenge_for_each_func_t func, void *data)
{
	int rc = EOK;
	struct_node_t *self = node_as_struct(base);
	bithenge_named_transform_t *subxforms =
	    self->transform->subtransforms;

	for (size_t i = 0; subxforms[i].transform; i++) {
		bithenge_node_t *subxform_result;
		rc = struct_node_subtransform(self, &subxform_result, i);
		if (rc != EOK)
			return rc;

		if (subxforms[i].name) {
			bithenge_node_t *name_node;
			rc = bithenge_new_string_node(&name_node,
			    subxforms[i].name, false);
			if (rc == EOK) {
				rc = func(name_node, subxform_result, data);
				subxform_result = NULL;
			}
		} else {
			if (bithenge_node_type(subxform_result) !=
			    BITHENGE_NODE_INTERNAL) {
				rc = EINVAL;
			} else {
				rc = bithenge_node_for_each(subxform_result,
				    func, data);
			}
		}
		bithenge_node_dec_ref(subxform_result);
		if (rc != EOK)
			return rc;
	}

	if (!self->prefix) {
		aoff64_t blob_size, end_pos;
		rc = bithenge_blob_size(self->blob, &blob_size);
		if (rc != EOK)
			return rc;
		rc = struct_node_field_offset(self, &end_pos,
		    self->transform->num_subtransforms);
		if (rc != EOK)
			return rc;
		if (blob_size != end_pos) {
			rc = EINVAL;
			return rc;
		}
	}

	return rc;
}

static int struct_node_get(bithenge_node_t *base, bithenge_node_t *key,
    bithenge_node_t **out)
{
	struct_node_t *self = node_as_struct(base);

	if (bithenge_node_type(key) != BITHENGE_NODE_STRING) {
		bithenge_node_dec_ref(key);
		return ENOENT;
	}
	const char *name = bithenge_string_node_value(key);

	for (size_t i = 0; self->transform->subtransforms[i].transform; i++) {
		if (self->transform->subtransforms[i].name
		    && !str_cmp(name, self->transform->subtransforms[i].name)) {
			bithenge_node_dec_ref(key);
			return struct_node_subtransform(self, out, i);
		}
	}

	for (size_t i = 0; self->transform->subtransforms[i].transform; i++) {
		if (self->transform->subtransforms[i].name)
			continue;
		bithenge_node_t *subxform_result;
		int rc = struct_node_subtransform(self, &subxform_result, i);
		if (rc != EOK) {
			bithenge_node_dec_ref(key);
			return rc;
		}
		if (bithenge_node_type(subxform_result) !=
		    BITHENGE_NODE_INTERNAL) {
			bithenge_node_dec_ref(subxform_result);
			bithenge_node_dec_ref(key);
			return EINVAL;
		}
		bithenge_node_inc_ref(key);
		rc = bithenge_node_get(subxform_result, key, out);
		bithenge_node_dec_ref(subxform_result);
		if (rc != ENOENT) {
			bithenge_node_dec_ref(key);
			return rc;
		}
	}

	bithenge_node_dec_ref(key);
	return ENOENT;
}

static void struct_node_destroy(bithenge_node_t *base)
{
	struct_node_t *node = node_as_struct(base);

	/* We didn't inc_ref for the scope in struct_transform_make_node, so
	 * make sure it doesn't try to dec_ref. */
	node->scope.current_node = NULL;
	bithenge_scope_destroy(&node->scope);

	bithenge_transform_dec_ref(struct_as_transform(node->transform));
	bithenge_blob_dec_ref(node->blob);
	free(node->ends);
	free(node);
}

static const bithenge_internal_node_ops_t struct_node_ops = {
	.for_each = struct_node_for_each,
	.get = struct_node_get,
	.destroy = struct_node_destroy,
};

static int struct_transform_make_node(struct_transform_t *self,
    bithenge_node_t **out, bithenge_scope_t *scope, bithenge_blob_t *blob,
    bool prefix)
{
	struct_node_t *node = malloc(sizeof(*node));
	if (!node)
		return ENOMEM;

	bithenge_scope_init(&node->scope);
	int rc = bithenge_scope_copy(&node->scope, scope);
	if (rc != EOK) {
		free(node);
		return rc;
	}

	node->ends = malloc(sizeof(*node->ends) * self->num_subtransforms);
	if (!node->ends) {
		bithenge_scope_destroy(&node->scope);
		free(node);
		return ENOMEM;
	}

	rc = bithenge_init_internal_node(struct_as_node(node),
	    &struct_node_ops);
	if (rc != EOK) {
		bithenge_scope_destroy(&node->scope);
		free(node->ends);
		free(node);
		return rc;
	}

	bithenge_transform_inc_ref(struct_as_transform(self));
	bithenge_blob_inc_ref(blob);
	node->transform = self;
	node->blob = blob;
	node->prefix = prefix;
	node->num_ends = 0;
	*out = struct_as_node(node);

	/* We should inc_ref(*out) here, but that would make a cycle. Instead,
	 * we leave it 1 too low, so that when the only remaining use of *out
	 * is the scope, *out will be destroyed. Also see the comment in
	 * struct_node_destroy. */
	bithenge_scope_set_current_node(&node->scope, *out);

	return EOK;
}

static int struct_transform_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	struct_transform_t *self = transform_as_struct(base);
	if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)
		return EINVAL;
	return struct_transform_make_node(self, out, scope,
	    bithenge_node_as_blob(in), false);
}

static int struct_transform_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *blob, aoff64_t *out)
{
	struct_transform_t *self = transform_as_struct(base);
	bithenge_node_t *struct_node;
	int rc = struct_transform_make_node(self, &struct_node, scope, blob,
	    true);
	if (rc != EOK)
		return rc;

	rc = struct_node_field_offset(node_as_struct(struct_node), out,
	    self->num_subtransforms);
	bithenge_node_dec_ref(struct_node);
	return rc;
}

static int struct_transform_prefix_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *blob, bithenge_node_t **out_node,
    aoff64_t *out_size)
{
	struct_transform_t *self = transform_as_struct(base);
	int rc = struct_transform_make_node(self, out_node, scope, blob,
	    true);
	if (rc != EOK)
		return rc;

	rc = struct_node_field_offset(node_as_struct(*out_node), out_size,
	    self->num_subtransforms);
	if (rc != EOK) {
		bithenge_node_dec_ref(*out_node);
		return rc;
	}

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
	.prefix_apply = struct_transform_prefix_apply,
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
	self->num_subtransforms = 0;
	for (self->num_subtransforms = 0;
	    subtransforms[self->num_subtransforms].transform;
	    self->num_subtransforms++);
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
