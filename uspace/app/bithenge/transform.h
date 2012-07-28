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

#ifndef BITHENGE_TRANSFORM_H_
#define BITHENGE_TRANSFORM_H_

#include "blob.h"
#include "tree.h"

/** A transform that creates a new tree from an old tree. */
typedef struct {
	/** @privatesection */
	const struct bithenge_transform_ops *ops;
	unsigned int refs;
	int num_params;
} bithenge_transform_t;

/** Context and parameters used when applying transforms. */
typedef struct {
	/** @privatesection */
	bithenge_node_t **params;
	int num_params;
} bithenge_scope_t;

/** Operations that may be provided by a transform. */
typedef struct bithenge_transform_ops {
	/** @copydoc bithenge_transform_t::bithenge_transform_apply */
	int (*apply)(bithenge_transform_t *self, bithenge_scope_t *scope,
	    bithenge_node_t *in, bithenge_node_t **out);
	/** @copydoc bithenge_transform_t::bithenge_transform_prefix_length */
	int (*prefix_length)(bithenge_transform_t *self,
	    bithenge_scope_t *scope, bithenge_blob_t *blob, aoff64_t *out);
	/** Destroy the transform.
	 * @param self The transform. */
	void (*destroy)(bithenge_transform_t *self);
} bithenge_transform_ops_t;

/** Initialize a transform scope. It must be destroyed with @a
 * bithenge_scope_destroy after it is used.
 * @param[out] scope The scope to initialize. */
static inline void bithenge_scope_init(bithenge_scope_t *scope)
{
	scope->params = NULL;
	scope->num_params = 0;
}

/** Destroy a transform scope.
 * @param scope The scope to destroy.
 * @return EOK on success or an error code from errno.h. */
static inline void bithenge_scope_destroy(bithenge_scope_t *scope)
{
	for (int i = 0; i < scope->num_params; i++)
		bithenge_node_dec_ref(scope->params[i]);
	free(scope->params);
}

/** Allocate parameters. The parameters must then be set with @a
 * bithenge_scope_set_param.
 * @param scope The scope in which to allocate parameters.
 * @param num_params The number of parameters to allocate.
 * @return EOK on success or an error code from errno.h. */
static inline int bithenge_scope_alloc_params(bithenge_scope_t *scope,
    int num_params)
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
static inline int bithenge_scope_set_param( bithenge_scope_t *scope, int i,
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
static inline int bithenge_scope_get_param(bithenge_scope_t *scope, int i,
    bithenge_node_t **out)
{
	assert(scope);
	assert(i >= 0 && i < scope->num_params);
	*out = scope->params[i];
	bithenge_node_inc_ref(*out);
	return EOK;
}

/** Get the number of parameters required by a transform. This number is used
 * by the parser and param-wrapper. Takes ownership of nothing.
 * @param self The transform.
 * @return The number of parameters required. */
static inline int bithenge_transform_num_params(bithenge_transform_t *self)
{
	assert(self);
	return self->num_params;
}

/** Apply a transform. Takes ownership of nothing.
 * @memberof bithenge_transform_t
 * @param self The transform.
 * @param scope The scope.
 * @param in The input tree.
 * @param[out] out Where the output tree will be stored.
 * @return EOK on success or an error code from errno.h. */
static inline int bithenge_transform_apply(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	assert(self);
	assert(self->ops);
	return self->ops->apply(self, scope, in, out);
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
static inline int bithenge_transform_prefix_length(bithenge_transform_t *self,
    bithenge_scope_t *scope, bithenge_blob_t *blob, aoff64_t *out)
{
	assert(self);
	assert(self->ops);
	if (!self->ops->prefix_length)
		return ENOTSUP;
	return self->ops->prefix_length(self, scope, blob, out);
}

/** Increment a transform's reference count.
 * @param self The transform to reference. */
static inline void bithenge_transform_inc_ref(bithenge_transform_t *self)
{
	assert(self);
	self->refs++;
}

/** Decrement a transform's reference count and free it if appropriate.
 * @param self The transform to dereference, or NULL. */
static inline void bithenge_transform_dec_ref(bithenge_transform_t *self)
{
	if (!self)
		return;
	assert(self->ops);
	if (--self->refs == 0)
		self->ops->destroy(self);
}

/** A transform with a name. */
typedef struct {
	const char *name;
	bithenge_transform_t *transform;
} bithenge_named_transform_t;

extern bithenge_transform_t bithenge_ascii_transform;
extern bithenge_transform_t bithenge_known_length_transform;
extern bithenge_transform_t bithenge_uint8_transform;
extern bithenge_transform_t bithenge_uint16le_transform;
extern bithenge_transform_t bithenge_uint16be_transform;
extern bithenge_transform_t bithenge_uint32le_transform;
extern bithenge_transform_t bithenge_uint32be_transform;
extern bithenge_transform_t bithenge_uint64le_transform;
extern bithenge_transform_t bithenge_uint64be_transform;
extern bithenge_transform_t bithenge_zero_terminated_transform;
extern bithenge_named_transform_t *bithenge_primitive_transforms;

int bithenge_init_transform(bithenge_transform_t *,
    const bithenge_transform_ops_t *, int);
int bithenge_new_param_transform(bithenge_transform_t **,
    bithenge_transform_t *, int);
int bithenge_new_struct(bithenge_transform_t **,
    bithenge_named_transform_t *);
int bithenge_new_composed_transform(bithenge_transform_t **,
    bithenge_transform_t **, size_t);

#endif

/** @}
 */
