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

static void expression_indestructible(bithenge_expression_t *self)
{
	assert(false);
}

typedef struct {
	bithenge_expression_t base;
	bithenge_binary_op_t op;
	bithenge_expression_t *a, *b;
} binary_expression_t;

static inline binary_expression_t *expression_as_binary(
    bithenge_expression_t *base)
{
	return (binary_expression_t *)base;
}

static inline bithenge_expression_t *binary_as_expression(
    binary_expression_t *self)
{
	return &self->base;
}

static int binary_expression_evaluate(bithenge_expression_t *base,
    bithenge_scope_t *scope, bithenge_node_t **out)
{
	binary_expression_t *self = expression_as_binary(base);
	bithenge_node_t *a, *b;
	int rc = bithenge_expression_evaluate(self->a, scope, &a);
	if (rc != EOK)
		return rc;
	rc = bithenge_expression_evaluate(self->b, scope, &b);
	if (rc != EOK) {
		bithenge_node_dec_ref(a);
		return rc;
	}
	switch (self->op) {
	case BITHENGE_EXPRESSION_EQUALS:
		rc = bithenge_new_boolean_node(out, bithenge_node_equal(a, b));
		break;
	}
	bithenge_node_dec_ref(a);
	bithenge_node_dec_ref(b);
	return rc;
}

static void binary_expression_destroy(bithenge_expression_t *base)
{
	binary_expression_t *self = expression_as_binary(base);
	bithenge_expression_dec_ref(self->a);
	bithenge_expression_dec_ref(self->b);
	free(self);
}

static const bithenge_expression_ops_t binary_expression_ops = {
	.evaluate = binary_expression_evaluate,
	.destroy = binary_expression_destroy,
};

/** Create a binary expression. Takes ownership of @a a and @a b.
 * @param[out] out Holds the new expression.
 * @param op The operator to apply.
 * @param a The first operand.
 * @param b The second operand.
 * @return EOK on success or an error code from errno.h. */
int bithenge_binary_expression(bithenge_expression_t **out,
    bithenge_binary_op_t op, bithenge_expression_t *a,
    bithenge_expression_t *b)
{
	int rc;
	binary_expression_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_init_expression(binary_as_expression(self),
	    &binary_expression_ops);
	if (rc != EOK)
		goto error;

	self->op = op;
	self->a = a;
	self->b = b;
	*out = binary_as_expression(self);
	return EOK;

error:
	bithenge_expression_dec_ref(a);
	bithenge_expression_dec_ref(b);
	free(self);
	return rc;
}

static int current_node_evaluate(bithenge_expression_t *self,
    bithenge_scope_t *scope, bithenge_node_t **out)
{
	*out = bithenge_scope_get_current_node(scope);
	if (!*out)
		return EINVAL;
	return EOK;
}

static const bithenge_expression_ops_t current_node_ops = {
	.evaluate = current_node_evaluate,
	.destroy = expression_indestructible,
};

static bithenge_expression_t current_node_expression = {
	&current_node_ops, 1
};

/** Create an expression that gets the current node being created.
 * @param[out] out Holds the new expression.
 * @return EOK on success or an error code from errno.h. */
int bithenge_current_node_expression(bithenge_expression_t **out)
{
	bithenge_expression_inc_ref(&current_node_expression);
	*out = &current_node_expression;
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
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_init_expression(const_as_expression(self),
	    &const_expression_ops);
	if (rc != EOK)
		goto error;

	self->node = node;
	*out = const_as_expression(self);
	return EOK;

error:
	free(self);
	bithenge_node_dec_ref(node);
	return rc;
}

typedef struct {
	bithenge_expression_t base;
	bithenge_expression_t *expr;
	bithenge_node_t *key;
} member_expression_t;

static member_expression_t *expression_as_member(bithenge_expression_t *base)
{
	return (member_expression_t *)base;
}

static bithenge_expression_t *member_as_expression(member_expression_t *expr)
{
	return &expr->base;
}

static int member_expression_evaluate(bithenge_expression_t *base, 
    bithenge_scope_t *scope, bithenge_node_t **out)
{
	member_expression_t *self = expression_as_member(base);
	bithenge_node_t *node;
	int rc = bithenge_expression_evaluate(self->expr, scope, &node);
	if (rc != EOK)
		return rc;
	bithenge_node_inc_ref(self->key);
	rc = bithenge_node_get(node, self->key, out);
	bithenge_node_dec_ref(node);
	return rc;
}

static void member_expression_destroy(bithenge_expression_t *base)
{
	member_expression_t *self = expression_as_member(base);
	bithenge_expression_dec_ref(self->expr);
	bithenge_node_dec_ref(self->key);
	free(self);
}

static const bithenge_expression_ops_t member_expression_ops = {
	.evaluate = member_expression_evaluate,
	.destroy = member_expression_destroy,
};

/** Create an expression that gets a member from a node. Takes references to
 * @a expr and @a key.
 * @param[out] out Holds the new expression.
 * @param expr Calculates the node to get the member of.
 * @param key The member to get.
 * @return EOK on success or an error code from errno.h. */
int bithenge_member_expression(bithenge_expression_t **out,
    bithenge_expression_t *expr, bithenge_node_t *key)
{
	int rc;
	member_expression_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_init_expression(member_as_expression(self),
	    &member_expression_ops);
	if (rc != EOK)
		goto error;

	self->expr = expr;
	self->key = key;
	*out = member_as_expression(self);
	return EOK;

error:
	bithenge_expression_dec_ref(expr);
	bithenge_node_dec_ref(key);
	free(self);
	return rc;
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
	bithenge_scope_t *inner;
	int rc = bithenge_scope_new(&inner);
	if (rc != EOK)
		return rc;
	rc = param_wrapper_fill_scope(self, inner, outer);
	if (rc != EOK)
		goto error;

	rc = bithenge_transform_apply(self->transform, inner, in, out);
	in = NULL;

error:
	bithenge_scope_dec_ref(inner);
	return rc;
}

static int param_wrapper_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *outer, bithenge_blob_t *in, aoff64_t *out)
{
	param_wrapper_t *self = transform_as_param_wrapper(base);
	bithenge_scope_t *inner;
	int rc = bithenge_scope_new(&inner);
	if (rc != EOK)
		return rc;
	rc = param_wrapper_fill_scope(self, inner, outer);
	if (rc != EOK)
		goto error;

	rc = bithenge_transform_prefix_length(self->transform, inner, in, out);
	in = NULL;

error:
	bithenge_scope_dec_ref(inner);
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

typedef struct {
	bithenge_transform_t base;
	bithenge_expression_t *expr;
} expression_transform_t;

static inline bithenge_transform_t *expression_as_transform(
    expression_transform_t *self)
{
	return &self->base;
}

static inline expression_transform_t *transform_as_expression(
    bithenge_transform_t *base)
{
	return (expression_transform_t *)base;
}

static int expression_transform_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	expression_transform_t *self = transform_as_expression(base);
	if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)
		return EINVAL;
	aoff64_t len;
	int rc = bithenge_blob_size(bithenge_node_as_blob(in), &len);
	if (rc != EOK)
		return rc;
	if (len != 0)
		return EINVAL;
	return bithenge_expression_evaluate(self->expr, scope, out);
}

static int expression_transform_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *in, aoff64_t *out)
{
	*out = 0;
	return EOK;
}

static void expression_transform_destroy(bithenge_transform_t *base)
{
	expression_transform_t *self = transform_as_expression(base);
	bithenge_expression_dec_ref(self->expr);
	free(self);
}

static const bithenge_transform_ops_t expression_transform_ops = {
	.apply = expression_transform_apply,
	.prefix_length = expression_transform_prefix_length,
	.destroy = expression_transform_destroy,
};

/** Create a transform that takes an empty blob and produces the result of an
 * expression. Takes a reference to the expression.
 * @param[out] out Holds the new transform.
 * @param expr The expression to evaluate.
 * @return EOK on success or an error code from errno.h. */
int bithenge_expression_transform(bithenge_transform_t ** out,
    bithenge_expression_t *expr)
{
	int rc;
	expression_transform_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_init_transform(expression_as_transform(self),
	    &expression_transform_ops, 0);
	if (rc != EOK)
		goto error;

	self->expr = expr;
	*out = expression_as_transform(self);
	return EOK;

error:
	free(self);
	bithenge_expression_dec_ref(expr);
	return rc;
}

typedef struct {
	bithenge_transform_t base;
	bithenge_expression_t *expr;
	bithenge_transform_t *true_xform, *false_xform;
} if_transform_t;

static inline bithenge_transform_t *if_as_transform(if_transform_t *self)
{
	return &self->base;
}

static inline if_transform_t *transform_as_if(bithenge_transform_t *base)
{
	return (if_transform_t *)base;
}

static int if_transform_choose(if_transform_t *self, bithenge_scope_t *scope,
    bool *out)
{
	bithenge_node_t *cond_node;
	int rc = bithenge_expression_evaluate(self->expr, scope, &cond_node);
	if (rc != EOK)
		return rc;
	if (bithenge_node_type(cond_node) != BITHENGE_NODE_BOOLEAN) {
		bithenge_node_dec_ref(cond_node);
		return EINVAL;
	}
	*out = bithenge_boolean_node_value(cond_node);
	bithenge_node_dec_ref(cond_node);
	return EOK;
}

static int if_transform_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	if_transform_t *self = transform_as_if(base);
	bool cond;
	int rc = if_transform_choose(self, scope, &cond);
	if (rc != EOK)
		return rc;
	return bithenge_transform_apply(
	    cond ? self->true_xform : self->false_xform, scope, in, out);
}

static int if_transform_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *in, aoff64_t *out)
{
	if_transform_t *self = transform_as_if(base);
	bool cond;
	int rc = if_transform_choose(self, scope, &cond);
	if (rc != EOK)
		return rc;
	return bithenge_transform_prefix_length(
	    cond ? self->true_xform : self->false_xform, scope, in, out);
}

static void if_transform_destroy(bithenge_transform_t *base)
{
	if_transform_t *self = transform_as_if(base);
	bithenge_expression_dec_ref(self->expr);
	bithenge_transform_dec_ref(self->true_xform);
	bithenge_transform_dec_ref(self->false_xform);
	free(self);
}

static const bithenge_transform_ops_t if_transform_ops = {
	.apply = if_transform_apply,
	.prefix_length = if_transform_prefix_length,
	.destroy = if_transform_destroy,
};

/** Create a transform that applies either of two transforms depending on a
 * boolean expression. Takes references to @a expr, @a true_xform, and
 * @a false_xform.
 * @param[out] out Holds the new transform.
 * @param expr The boolean expression to evaluate.
 * @param true_xform The transform to apply if the expression is true.
 * @param false_xform The transform to apply if the expression is false.
 * @return EOK on success or an error code from errno.h. */
int bithenge_if_transform(bithenge_transform_t **out,
    bithenge_expression_t *expr, bithenge_transform_t *true_xform,
    bithenge_transform_t *false_xform)
{
	int rc;
	if_transform_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_init_transform(if_as_transform(self), &if_transform_ops,
	    0);
	if (rc != EOK)
		goto error;

	self->expr = expr;
	self->true_xform = true_xform;
	self->false_xform = false_xform;
	*out = if_as_transform(self);
	return EOK;

error:
	free(self);
	bithenge_expression_dec_ref(expr);
	bithenge_transform_dec_ref(true_xform);
	bithenge_transform_dec_ref(false_xform);
	return rc;
}

/** @}
 */
