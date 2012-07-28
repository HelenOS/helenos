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
 * Expressions.
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include "blob.h"
#include "expression.h"
#include "transform.h"
#include "tree.h"

/** Initialize a new expression.
 * @param[out] self Expression to initialize.
 * @param[in] ops Operations provided by the expression.
 * @return EOK or an error code from errno.h. */
int bithenge_init_expression(bithenge_expression_t *self,
    const bithenge_expression_ops_t *ops)
{
	assert(ops);
	assert(ops->evaluate);
	assert(ops->destroy);
	self->ops = ops;
	self->refs = 1;
	return EOK;
}

typedef struct {
	bithenge_expression_t base;
	int index;
} param_expression_t;

static inline param_expression_t *expression_as_param(
    bithenge_expression_t *base)
{
	return (param_expression_t *)base;
}

static inline bithenge_expression_t *param_as_expression(
    param_expression_t *self)
{
	return &self->base;
}

static int param_expression_evaluate(bithenge_expression_t *base,
    bithenge_scope_t *scope, bithenge_node_t **out)
{
	param_expression_t *self = expression_as_param(base);
	return bithenge_scope_get_param(scope, self->index, out);
}

static void param_expression_destroy(bithenge_expression_t *base)
{
	param_expression_t *self = expression_as_param(base);
	free(self);
}

static const bithenge_expression_ops_t param_expression_ops = {
	.evaluate = param_expression_evaluate,
	.destroy = param_expression_destroy,
};

/** Create an expression that returns a parameter.
 * @param[out] out Holds the created expression.
 * @param index The index of the parameter to get.
 * @return EOK on success or an error code from errno.h. */
int bithenge_param_expression(bithenge_expression_t **out, int index)
{
	int rc;
	param_expression_t *self = malloc(sizeof(*self));
	if (!self)
		return ENOMEM;

	rc = bithenge_init_expression(param_as_expression(self),
	    &param_expression_ops);
	if (rc != EOK) {
		free(self);
		return rc;
	}

	self->index = index;
	*out = param_as_expression(self);
	return EOK;
}

typedef struct {
	bithenge_expression_t base;
	bithenge_node_t *node;
} const_expression_t;

static inline const_expression_t *expression_as_const(
    bithenge_expression_t *base)
{
	return (const_expression_t *)base;
}

static inline bithenge_expression_t *const_as_expression(
    const_expression_t *self)
{
	return &self->base;
}

static int const_expression_evaluate(bithenge_expression_t *base,
    bithenge_scope_t *scope, bithenge_node_t **out)
{
	const_expression_t *self = expression_as_const(base);
	bithenge_node_inc_ref(self->node);
	*out = self->node;
	return EOK;
}

static void const_expression_destroy(bithenge_expression_t *base)
{
	const_expression_t *self = expression_as_const(base);
	bithenge_node_dec_ref(self->node);
	free(self);
}

static const bithenge_expression_ops_t const_expression_ops = {
	.evaluate = const_expression_evaluate,
	.destroy = const_expression_destroy,
};

/** Create an expression that returns a constant. Takes a reference to @a node.
 * @param[out] out Holds the created expression.
 * @param node The constant.
 * @return EOK on success or an error code from errno.h. */
int bithenge_const_expression(bithenge_expression_t **out,
    bithenge_node_t *node)
{
	int rc;
	const_expression_t *self = malloc(sizeof(*self));
	if (!self)
		return ENOMEM;

	rc = bithenge_init_expression(const_as_expression(self),
	    &const_expression_ops);
	if (rc != EOK) {
		free(self);
		return rc;
	}

	self->node = node;
	*out = const_as_expression(self);
	return EOK;
}

typedef struct {
	bithenge_transform_t base;
	bithenge_transform_t *transform;
	bithenge_expression_t **params;
} param_wrapper_t;

static inline bithenge_transform_t *param_wrapper_as_transform(
    param_wrapper_t *self)
{
	return &self->base;
}

static inline param_wrapper_t *transform_as_param_wrapper(
    bithenge_transform_t *base)
{
	return (param_wrapper_t *)base;
}

static int param_wrapper_fill_scope(param_wrapper_t *self, bithenge_scope_t
    *inner, bithenge_scope_t *outer)
{
	int rc;
	int num_params = bithenge_transform_num_params(self->transform);
	rc = bithenge_scope_alloc_params(inner, num_params);
	if (rc != EOK)
		return rc;
	for (int i = 0; i < num_params; i++) {
		bithenge_node_t *node;
		rc = bithenge_expression_evaluate(self->params[i], outer,
		    &node);
		if (rc != EOK)
			return rc;
		rc = bithenge_scope_set_param(inner, i, node);
		if (rc != EOK)
			return rc;
	}
	return EOK;
}

static int param_wrapper_apply(bithenge_transform_t *base,
    bithenge_scope_t *outer, bithenge_node_t *in, bithenge_node_t **out)
{
	param_wrapper_t *self = transform_as_param_wrapper(base);
	bithenge_scope_t inner;
	bithenge_scope_init(&inner);
	int rc = param_wrapper_fill_scope(self, &inner, outer);
	if (rc != EOK)
		goto error;

	rc = bithenge_transform_apply(self->transform, &inner, in, out);
	in = NULL;

error:
	bithenge_scope_destroy(&inner);
	return rc;
}

static int param_wrapper_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *outer, bithenge_blob_t *in, aoff64_t *out)
{
	param_wrapper_t *self = transform_as_param_wrapper(base);
	bithenge_scope_t inner;
	bithenge_scope_init(&inner);
	int rc = param_wrapper_fill_scope(self, &inner, outer);
	if (rc != EOK)
		goto error;

	rc = bithenge_transform_prefix_length(self->transform, &inner, in,
	    out);
	in = NULL;

error:
	bithenge_scope_destroy(&inner);
	return rc;
}

static void param_wrapper_destroy(bithenge_transform_t *base)
{
	param_wrapper_t *self = transform_as_param_wrapper(base);
	int num_params = bithenge_transform_num_params(self->transform);
	bithenge_transform_dec_ref(self->transform);
	for (int i = 0; i < num_params; i++)
		bithenge_expression_dec_ref(self->params[i]);
	free(self->params);
	free(self);
}

static const bithenge_transform_ops_t param_wrapper_ops = {
	.apply = param_wrapper_apply,
	.prefix_length = param_wrapper_prefix_length,
	.destroy = param_wrapper_destroy,
};

/** Create a transform that calculates parameters for another transform. Takes
 * ownership of @a transform, @a params, and each element of @a params. The
 * number of parameters must be correct.
 * @param[out] out Holds the new transform.
 * @param transform The transform for which parameters are calculated.
 * @param params The expressions used to calculate the parameters.
 * @return EOK on success or an error code from errno.h. */
int bithenge_param_wrapper(bithenge_transform_t **out,
    bithenge_transform_t *transform, bithenge_expression_t **params)
{
	int rc;
	int num_params = bithenge_transform_num_params(transform);
	param_wrapper_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_init_transform(param_wrapper_as_transform(self),
	    &param_wrapper_ops, 0);
	if (rc != EOK)
		goto error;

	self->transform = transform;
	self->params = params;
	*out = param_wrapper_as_transform(self);
	return EOK;

error:
	free(self);
	for (int i = 0; i < num_params; i++)
		bithenge_expression_dec_ref(params[i]);
	free(params);
	bithenge_transform_dec_ref(transform);
	return rc;
}

/** @}
 */
