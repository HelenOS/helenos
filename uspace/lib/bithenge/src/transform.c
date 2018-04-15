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
#include <stdarg.h>
#include <stdlib.h>
#include <bithenge/blob.h>
#include <bithenge/print.h>
#include <bithenge/transform.h>
#include "common.h"



/***************** transform                                 *****************/

/** Initialize a new transform.
 * @param[out] self Transform to initialize.
 * @param[in] ops Operations provided by the transform.
 * @param num_params The number of parameters required. If this is nonzero, the
 * transform will get its own context with parameters, probably provided by a
 * param_wrapper. If this is zero, the existing outer context will be used with
 * whatever parameters it has, so they can be passed to any param_wrappers
 * within.
 * @return EOK or an error code from errno.h. */
errno_t bithenge_init_transform(bithenge_transform_t *self,
    const bithenge_transform_ops_t *ops, int num_params)
{
	assert(ops);
	assert(ops->apply || ops->prefix_apply);
	assert(ops->destroy);
	if (bithenge_should_fail())
		return ENOMEM;
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
errno_t bithenge_transform_apply(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	assert(self);
	assert(self->ops);
	if (self->ops->apply)
		return self->ops->apply(self, scope, in, out);

	if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)
		return EINVAL;
	aoff64_t self_size, whole_size;
	errno_t rc = bithenge_transform_prefix_apply(self, scope,
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
errno_t bithenge_transform_prefix_length(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_blob_t *blob, aoff64_t *out)
{
	assert(self);
	assert(self->ops);
	if (self->ops->prefix_length)
		return self->ops->prefix_length(self, scope, blob, out);
	if (!self->ops->prefix_apply)
		return ENOTSUP;

	bithenge_node_t *node;
	errno_t rc = bithenge_transform_prefix_apply(self, scope, blob, &node,
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
 * @param[out] out_size Holds the size of the prefix. Can be null, in which
 * case the size is not determined.
 * @return EOK on success, ENOTSUP if not supported, or another error code from
 * errno.h. */
errno_t bithenge_transform_prefix_apply(bithenge_transform_t *self,
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

	aoff64_t size;
	errno_t rc = bithenge_transform_prefix_length(self, scope, blob, &size);
	if (rc != EOK)
		return rc;
	bithenge_node_t *prefix_blob;
	bithenge_blob_inc_ref(blob);
	rc = bithenge_new_subblob(&prefix_blob, blob, 0, size);
	if (rc != EOK)
		return rc;
	rc = bithenge_transform_apply(self, scope, prefix_blob, out_node);
	bithenge_node_dec_ref(prefix_blob);
	if (out_size)
		*out_size = size;
	return rc;
}



/***************** scope                                     *****************/

/** Create a transform scope. It must be dereferenced with @a
 * bithenge_scope_dec_ref after it is used. Takes ownership of nothing.
 * @memberof bithenge_scope_t
 * @param[out] out Holds the new scope.
 * @param outer The outer scope, or NULL.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_scope_new(bithenge_scope_t **out, bithenge_scope_t *outer)
{
	bithenge_scope_t *self = malloc(sizeof(*self));
	if (!self)
		return ENOMEM;
	self->refs = 1;
	if (outer)
		bithenge_scope_inc_ref(outer);
	self->outer = outer;
	self->error = NULL;
	self->barrier = false;
	self->num_params = 0;
	self->params = NULL;
	self->current_node = NULL;
	self->in_node = NULL;
	*out = self;
	return EOK;
}

/** Dereference a transform scope.
 * @memberof bithenge_scope_t
 * @param self The scope to dereference, or NULL. */
void bithenge_scope_dec_ref(bithenge_scope_t *self)
{
	if (!self)
		return;
	if (--self->refs)
		return;
	bithenge_node_dec_ref(self->current_node);
	for (int i = 0; i < self->num_params; i++)
		bithenge_node_dec_ref(self->params[i]);
	bithenge_scope_dec_ref(self->outer);
	free(self->params);
	free(self->error);
	free(self);
}

/** Get the outer scope of a scope, which may be NULL.
 * @memberof bithenge_scope_t
 * @param self The scope to examine.
 * @return The outer scope, which may be NULL. */
bithenge_scope_t *bithenge_scope_outer(bithenge_scope_t *self)
{
	return self->outer;
}

/** Get the error message stored in the scope, which may be NULL. The error
 * message only exists as long as the scope does.
 * @memberof bithenge_scope_t
 * @param scope The scope to get the error message from.
 * @return The error message, or NULL. */
const char *bithenge_scope_get_error(bithenge_scope_t *scope)
{
	return scope->error;
}

/** Set the error message for the scope. The error message is stored in the
 * outermost scope, but if any scope already has an error message this error
 * message is ignored.
 * @memberof bithenge_scope_t
 * @param scope The scope.
 * @param format The format string.
 * @return EINVAL normally, or another error code from errno.h. */
errno_t bithenge_scope_error(bithenge_scope_t *scope, const char *format, ...)
{
	if (scope->error)
		return EINVAL;
	while (scope->outer) {
		scope = scope->outer;
		if (scope->error)
			return EINVAL;
	}
	size_t space_left = 256;
	scope->error = malloc(space_left);
	if (!scope->error)
		return ENOMEM;
	char *out = scope->error;
	va_list ap;
	va_start(ap, format);

	while (*format) {
		if (format[0] == '%' && format[1] == 't') {
			format += 2;
			errno_t rc = bithenge_print_node_to_string(&out,
			    &space_left, BITHENGE_PRINT_PYTHON,
			    va_arg(ap, bithenge_node_t *));
			if (rc != EOK) {
				va_end(ap);
				return rc;
			}
		} else {
			const char *end = str_chr(format, '%');
			if (!end)
				end = format + str_length(format);
			size_t size = min((size_t)(end - format),
			    space_left - 1);
			memcpy(out, format, size);
			format = end;
			out += size;
			space_left -= size;
		}
	}
	*out = '\0';

	va_end(ap);
	return EINVAL;
}

/** Get the current node being created, which may be NULL.
 * @memberof bithenge_scope_t
 * @param scope The scope to get the current node from.
 * @return The node being created, or NULL. */
bithenge_node_t *bithenge_scope_get_current_node(bithenge_scope_t *scope)
{
	if (scope->current_node)
		bithenge_node_inc_ref(scope->current_node);
	return scope->current_node;
}

/** Set the current node being created. Takes a reference to @a node.
 * @memberof bithenge_scope_t
 * @param scope The scope to set the current node in.
 * @param node The current node being created, or NULL. */
void bithenge_scope_set_current_node(bithenge_scope_t *scope,
    bithenge_node_t *node)
{
	bithenge_node_dec_ref(scope->current_node);
	scope->current_node = node;
}

/** Get the current input node, which may be NULL.
 * @memberof bithenge_scope_t
 * @param scope The scope to get the current input node from.
 * @return The input node, or NULL. */
bithenge_node_t *bithenge_scope_in_node(bithenge_scope_t *scope)
{
	if (scope->in_node)
		bithenge_node_inc_ref(scope->in_node);
	return scope->in_node;
}

/** Set the current input node. Takes a reference to @a node.
 * @memberof bithenge_scope_t
 * @param scope The scope to set the input node in.
 * @param node The input node, or NULL. */
void bithenge_scope_set_in_node(bithenge_scope_t *scope, bithenge_node_t *node)
{
	bithenge_node_dec_ref(scope->in_node);
	scope->in_node = node;
}

/** Set a scope as a barrier.
 * @memberof bithenge_scope_t
 * @param self The scope to change. */
void bithenge_scope_set_barrier(bithenge_scope_t *self)
{
	self->barrier = true;
}

/** Check whether a scope is a barrier, meaning that variable lookup stops at
 * it.
 * @memberof bithenge_scope_t
 * @param self The scope to check.
 * @return Whether the scope is a barrier. */
bool bithenge_scope_is_barrier(bithenge_scope_t *self)
{
	return self->barrier;
}

/** Allocate parameters. The parameters must then be set with @a
 * bithenge_scope_set_param. This must not be called on a scope that already
 * has parameters.
 * @memberof bithenge_scope_t
 * @param scope The scope in which to allocate parameters.
 * @param num_params The number of parameters to allocate.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_scope_alloc_params(bithenge_scope_t *scope, int num_params)
{
	scope->params = malloc(sizeof(*scope->params) * num_params);
	if (!scope->params)
		return ENOMEM;
	scope->num_params = num_params;
	for (int i = 0; i < num_params; i++)
		scope->params[i] = NULL;
	return EOK;
}

/** Set a parameter. Takes a reference to @a node. Note that range checking is
 * not done in release builds.
 * @memberof bithenge_scope_t
 * @param scope The scope in which to allocate parameters.
 * @param i The index of the parameter to set.
 * @param node The value to store in the parameter.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_scope_set_param(bithenge_scope_t *scope, int i,
    bithenge_node_t *node)
{
	assert(scope);
	assert(i >= 0 && i < scope->num_params);
	if (bithenge_should_fail()) {
		bithenge_node_dec_ref(node);
		return ENOMEM;
	}
	scope->params[i] = node;
	return EOK;
}

/** Get a parameter. Note that range checking is not done in release builds.
 * @memberof bithenge_scope_t
 * @param scope The scope to get the parameter from.
 * @param i The index of the parameter to set.
 * @param[out] out Stores a new reference to the parameter.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_scope_get_param(bithenge_scope_t *scope, int i,
    bithenge_node_t **out)
{
	assert(scope);
	if (scope->num_params) {
		assert(i >= 0 && i < scope->num_params);
		*out = scope->params[i];
		bithenge_node_inc_ref(*out);
		return EOK;
	} else {
		return bithenge_scope_get_param(scope->outer, i, out);
	}
}



/***************** barrier_transform                         *****************/

typedef struct {
	bithenge_transform_t base;
	bithenge_transform_t *transform;
} barrier_transform_t;

static inline barrier_transform_t *transform_as_barrier(
    bithenge_transform_t *base)
{
	return (barrier_transform_t *)base;
}

static inline bithenge_transform_t *barrier_as_transform(
    barrier_transform_t *self)
{
	return &self->base;
}

static errno_t barrier_transform_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	barrier_transform_t *self = transform_as_barrier(base);
	bithenge_scope_t *inner_scope;
	errno_t rc = bithenge_scope_new(&inner_scope, scope);
	if (rc != EOK)
		return rc;
	bithenge_scope_set_barrier(inner_scope);
	bithenge_scope_set_in_node(inner_scope, in);
	rc = bithenge_transform_apply(self->transform, inner_scope, in, out);
	bithenge_scope_dec_ref(inner_scope);
	return rc;
}

static errno_t barrier_transform_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *in, aoff64_t *out)
{
	barrier_transform_t *self = transform_as_barrier(base);
	bithenge_scope_t *inner_scope;
	errno_t rc = bithenge_scope_new(&inner_scope, scope);
	if (rc != EOK)
		return rc;
	bithenge_scope_set_barrier(inner_scope);
	bithenge_scope_set_in_node(inner_scope, bithenge_blob_as_node(in));
	rc = bithenge_transform_prefix_length(self->transform, inner_scope, in,
	    out);
	bithenge_scope_dec_ref(inner_scope);
	return rc;
}

static errno_t barrier_transform_prefix_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *in, bithenge_node_t **out_node,
    aoff64_t *out_length)
{
	barrier_transform_t *self = transform_as_barrier(base);
	bithenge_scope_t *inner_scope;
	errno_t rc = bithenge_scope_new(&inner_scope, scope);
	if (rc != EOK)
		return rc;
	bithenge_scope_set_barrier(inner_scope);
	bithenge_scope_set_in_node(inner_scope, bithenge_blob_as_node(in));
	rc = bithenge_transform_prefix_apply(self->transform, inner_scope, in,
	    out_node, out_length);
	bithenge_scope_dec_ref(inner_scope);
	return rc;
}

static void barrier_transform_destroy(bithenge_transform_t *base)
{
	barrier_transform_t *self = transform_as_barrier(base);
	bithenge_transform_dec_ref(self->transform);
	free(self);
}

static const bithenge_transform_ops_t barrier_transform_ops = {
	.apply = barrier_transform_apply,
	.prefix_length = barrier_transform_prefix_length,
	.prefix_apply = barrier_transform_prefix_apply,
	.destroy = barrier_transform_destroy,
};

/** Set the subtransform of a barrier transform. This must be done before the
 * barrier transform is used. Takes a reference to @a transform.
 * @param base The barrier transform.
 * @param transform The subtransform to use for all operations.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_barrier_transform_set_subtransform(bithenge_transform_t *base,
    bithenge_transform_t *transform)
{
	assert(transform);
	assert(bithenge_transform_num_params(transform) == 0);

	if (bithenge_should_fail()) {
		bithenge_transform_dec_ref(transform);
		return ENOMEM;
	}

	barrier_transform_t *self = transform_as_barrier(base);
	assert(!self->transform);
	self->transform = transform;
	return EOK;
}

/** Create a wrapper transform that creates a new scope. This ensures nothing
 * from the outer scope is passed in, other than parameters. The wrapper may
 * have a different value for num_params. The subtransform must be set with @a
 * bithenge_barrier_transform_set_subtransform before the result is used.
 * @param[out] out Holds the created transform.
 * @param num_params The number of parameters to require, which may be 0.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_new_barrier_transform(bithenge_transform_t **out, int num_params)
{
	errno_t rc;
	barrier_transform_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}
	rc = bithenge_init_transform(barrier_as_transform(self),
	    &barrier_transform_ops, num_params);
	if (rc != EOK)
		goto error;
	self->transform = NULL;
	*out = barrier_as_transform(self);
	return EOK;
error:
	free(self);
	return rc;
}



/***************** ascii                                     *****************/

static errno_t ascii_apply(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_node_t *in, bithenge_node_t **out)
{
	errno_t rc;
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



/***************** bit                                       *****************/

static errno_t bit_prefix_apply(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_blob_t *blob, bithenge_node_t **out_node,
    aoff64_t *out_size)
{
	char buffer;
	aoff64_t size = 1;
	errno_t rc = bithenge_blob_read_bits(blob, 0, &buffer, &size, true);
	if (rc != EOK)
		return rc;
	if (size != 1)
		return EINVAL;
	if (out_size)
		*out_size = size;
	return bithenge_new_boolean_node(out_node, (buffer & 1) != 0);
}

static const bithenge_transform_ops_t bit_ops = {
	.prefix_apply = bit_prefix_apply,
	.destroy = transform_indestructible,
};

/** A transform that decodes a bit as a boolean. */
bithenge_transform_t bithenge_bit_transform = {
	&bit_ops, 1, 0
};



/***************** bits_be, bits_le                          *****************/

typedef struct {
	bithenge_blob_t base;
	bithenge_blob_t *bytes;
	bool little_endian;
} bits_xe_blob_t;

static bits_xe_blob_t *blob_as_bits_xe(bithenge_blob_t *base)
{
	return (bits_xe_blob_t *)base;
}

static bithenge_blob_t *bits_xe_as_blob(bits_xe_blob_t *self)
{
	return &self->base;
}

static errno_t bits_xe_size(bithenge_blob_t *base, aoff64_t *out)
{
	bits_xe_blob_t *self = blob_as_bits_xe(base);
	errno_t rc = bithenge_blob_size(self->bytes, out);
	*out *= 8;
	return rc;
}

static uint8_t reverse_byte(uint8_t val)
{
	val = ((val & 0x0f) << 4) ^ ((val & 0xf0) >> 4);
	val = ((val & 0x33) << 2) ^ ((val & 0xcc) >> 2);
	val = ((val & 0x55) << 1) ^ ((val & 0xaa) >> 1);
	return val;
}

static errno_t bits_xe_read_bits(bithenge_blob_t *base, aoff64_t offset,
    char *buffer, aoff64_t *size, bool little_endian)
{
	bits_xe_blob_t *self = blob_as_bits_xe(base);
	aoff64_t bytes_offset = offset / 8;
	aoff64_t bit_offset = offset % 8;
	aoff64_t output_num_bytes = (*size + 7) / 8;
	aoff64_t bytes_size = (*size + bit_offset + 7) / 8;
	bool separate_buffer = bit_offset != 0;
	uint8_t *bytes_buffer;
	if (separate_buffer) {
		/* Allocate an extra byte, to make sure byte1 can be read. */
		bytes_buffer = malloc(bytes_size + 1);
		if (!bytes_buffer)
			return ENOMEM;
	} else
		bytes_buffer = (uint8_t *)buffer;

	errno_t rc = bithenge_blob_read(self->bytes, bytes_offset,
	    (char *)bytes_buffer, &bytes_size);
	if (rc != EOK)
		goto end;
	*size = min(*size, bytes_size * 8 - bit_offset);

	if (little_endian != self->little_endian)
		for (aoff64_t i = 0; i < bytes_size; i++)
			bytes_buffer[i] = reverse_byte(bytes_buffer[i]);

	if (bit_offset || separate_buffer) {
		for (aoff64_t i = 0; i < output_num_bytes; i++) {
			uint8_t byte0 = bytes_buffer[i];
			uint8_t	byte1 = bytes_buffer[i + 1];
			buffer[i] = little_endian ?
			    (byte0 >> bit_offset) ^ (byte1 << (8 - bit_offset)) :
			    (byte0 << bit_offset) ^ (byte1 >> (8 - bit_offset));
		}
	}

end:
	if (separate_buffer)
		free(bytes_buffer);
	return rc;
}

static void bits_xe_destroy(bithenge_blob_t *base)
{
	bits_xe_blob_t *self = blob_as_bits_xe(base);
	bithenge_blob_dec_ref(self->bytes);
	free(self);
}

static const bithenge_random_access_blob_ops_t bits_xe_blob_ops = {
	.size = bits_xe_size,
	.read_bits = bits_xe_read_bits,
	.destroy = bits_xe_destroy,
};

static errno_t bits_xe_apply(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_node_t *in, bithenge_node_t **out)
{
	if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)
		return EINVAL;
	bits_xe_blob_t *blob = malloc(sizeof(*blob));
	if (!blob)
		return ENOMEM;
	errno_t rc = bithenge_init_random_access_blob(bits_xe_as_blob(blob),
	    &bits_xe_blob_ops);
	if (rc != EOK) {
		free(blob);
		return rc;
	}
	bithenge_node_inc_ref(in);
	blob->bytes = bithenge_node_as_blob(in);
	blob->little_endian = (self == &bithenge_bits_le_transform);
	*out = bithenge_blob_as_node(bits_xe_as_blob(blob));
	return EOK;
}

static const bithenge_transform_ops_t bits_xe_ops = {
	.apply = bits_xe_apply,
	.destroy = transform_indestructible,
};

/** A transform that converts a byte blob to a bit blob, most-significant bit
 * first. */
bithenge_transform_t bithenge_bits_be_transform = {
	&bits_xe_ops, 1, 0
};

/** A transform that converts a byte blob to a bit blob, least-significant bit
 * first. */
bithenge_transform_t bithenge_bits_le_transform = {
	&bits_xe_ops, 1, 0
};



/***************** invalid                                   *****************/

static errno_t invalid_apply(bithenge_transform_t *self, bithenge_scope_t *scope,
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



/***************** known_length                              *****************/

static errno_t known_length_apply(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	bithenge_node_t *length_node;
	errno_t rc = bithenge_scope_get_param(scope, 0, &length_node);
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

static errno_t known_length_prefix_length(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_blob_t *in, aoff64_t *out)
{
	bithenge_node_t *length_node;
	errno_t rc = bithenge_scope_get_param(scope, 0, &length_node);
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

static errno_t nonzero_boolean_apply(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	if (bithenge_node_type(in) != BITHENGE_NODE_INTEGER)
		return EINVAL;
	bool value = bithenge_integer_node_value(in) != 0;
	return bithenge_new_boolean_node(out, value);
}

static const bithenge_transform_ops_t nonzero_boolean_ops = {
	.apply = nonzero_boolean_apply,
	.destroy = transform_indestructible,
};

/** A transform that converts integers to booleans, true if nonzero. */
bithenge_transform_t bithenge_nonzero_boolean_transform = {
	&nonzero_boolean_ops, 1, 0
};

static errno_t prefix_length_1(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_blob_t *blob, aoff64_t *out)
{
	*out = 1;
	return EOK;
}

static errno_t prefix_length_2(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_blob_t *blob, aoff64_t *out)
{
	*out = 2;
	return EOK;
}

static errno_t prefix_length_4(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_blob_t *blob, aoff64_t *out)
{
	*out = 4;
	return EOK;
}

static errno_t prefix_length_8(bithenge_transform_t *self, bithenge_scope_t *scope,
    bithenge_blob_t *blob, aoff64_t *out)
{
	*out = 8;
	return EOK;
}

static uint8_t uint8_t_identity(uint8_t arg)
{
	return arg;
}

/** @cond internal */
#define MAKE_UINT_TRANSFORM(NAME, TYPE, ENDIAN, PREFIX_LENGTH_FUNC)            \
	static errno_t NAME##_apply(bithenge_transform_t *self,                    \
	    bithenge_scope_t *scope, bithenge_node_t *in,                      \
	    bithenge_node_t **out)                                             \
	{                                                                      \
		errno_t rc;                                                        \
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

MAKE_UINT_TRANSFORM(uint8,    uint8_t,  uint8_t_identity, prefix_length_1);
MAKE_UINT_TRANSFORM(uint16le, uint16_t, uint16_t_le2host, prefix_length_2);
MAKE_UINT_TRANSFORM(uint16be, uint16_t, uint16_t_be2host, prefix_length_2);
MAKE_UINT_TRANSFORM(uint32le, uint32_t, uint32_t_le2host, prefix_length_4);
MAKE_UINT_TRANSFORM(uint32be, uint32_t, uint32_t_be2host, prefix_length_4);
MAKE_UINT_TRANSFORM(uint64le, uint64_t, uint64_t_le2host, prefix_length_8);
MAKE_UINT_TRANSFORM(uint64be, uint64_t, uint64_t_be2host, prefix_length_8);
/** @endcond */



/***************** uint_be, uint_le                          *****************/

static errno_t uint_xe_prefix_apply(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_blob_t *blob, bithenge_node_t **out_node,
    aoff64_t *out_size)
{
	bool little_endian = (self == &bithenge_uint_le_transform);
	bithenge_node_t *num_bits_node;
	errno_t rc = bithenge_scope_get_param(scope, 0, &num_bits_node);
	if (rc != EOK)
		return rc;
	if (bithenge_node_type(num_bits_node) != BITHENGE_NODE_INTEGER) {
		bithenge_node_dec_ref(num_bits_node);
		return EINVAL;
	}
	bithenge_int_t num_bits = bithenge_integer_node_value(num_bits_node);
	bithenge_node_dec_ref(num_bits_node);
	if (num_bits < 0)
		return EINVAL;
	if ((size_t)num_bits > sizeof(bithenge_int_t) * 8 - 1)
		return EINVAL;

	aoff64_t size = num_bits;
	uint8_t buffer[sizeof(bithenge_int_t)];
	rc = bithenge_blob_read_bits(blob, 0, (char *)buffer, &size,
	    little_endian);
	if (rc != EOK)
		return rc;
	if (size != (aoff64_t)num_bits)
		return EINVAL;
	if (out_size)
		*out_size = size;

	bithenge_int_t result = 0;
	bithenge_int_t num_easy_bytes = num_bits / 8;
	if (little_endian) {
		for (bithenge_int_t i = 0; i < num_easy_bytes; i++)
			result += buffer[i] << 8 * i;
		if (num_bits % 8)
			result += (buffer[num_easy_bytes] &
			    ((1 << num_bits % 8) - 1)) << 8 * num_easy_bytes;
	} else {
		for (bithenge_int_t i = 0; i < num_easy_bytes; i++)
			result += buffer[i] << (num_bits - 8 * (i + 1));
		if (num_bits % 8)
			result += buffer[num_easy_bytes] >> (8 - num_bits % 8);
	}

	return bithenge_new_integer_node(out_node, result);
}

static const bithenge_transform_ops_t uint_xe_ops = {
	.prefix_apply = uint_xe_prefix_apply,
	.destroy = transform_indestructible,
};

/** A transform that reads an unsigned integer from an arbitrary number of
 * bits, most-significant bit first. */
bithenge_transform_t bithenge_uint_be_transform = {
	&uint_xe_ops, 1, 1
};

/** A transform that reads an unsigned integer from an arbitrary number of
 * bits, least-significant bit first. */
bithenge_transform_t bithenge_uint_le_transform = {
	&uint_xe_ops, 1, 1
};



/***************** zero_terminated                           *****************/

static errno_t zero_terminated_apply(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	errno_t rc;
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

static errno_t zero_terminated_prefix_length(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_blob_t *blob, aoff64_t *out)
{
	errno_t rc;
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
	{ "ascii", &bithenge_ascii_transform },
	{ "bit", &bithenge_bit_transform },
	{ "bits_be", &bithenge_bits_be_transform },
	{ "bits_le", &bithenge_bits_le_transform },
	{ "known_length", &bithenge_known_length_transform },
	{ "nonzero_boolean", &bithenge_nonzero_boolean_transform },
	{ "uint8", &bithenge_uint8_transform },
	{ "uint16be", &bithenge_uint16be_transform },
	{ "uint16le", &bithenge_uint16le_transform },
	{ "uint32be", &bithenge_uint32be_transform },
	{ "uint32le", &bithenge_uint32le_transform },
	{ "uint64be", &bithenge_uint64be_transform },
	{ "uint64le", &bithenge_uint64le_transform },
	{ "uint_be", &bithenge_uint_be_transform },
	{ "uint_le", &bithenge_uint_le_transform },
	{ "zero_terminated", &bithenge_zero_terminated_transform },
	{ NULL, NULL }
};

/** An array of named built-in transforms. */
bithenge_named_transform_t *bithenge_primitive_transforms = primitive_transforms;

/** @}
 */
