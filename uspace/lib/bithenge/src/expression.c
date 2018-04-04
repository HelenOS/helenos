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
#include "common.h"
#include <bithenge/blob.h>
#include <bithenge/expression.h>
#include <bithenge/transform.h>
#include <bithenge/tree.h>

/** Initialize a new expression.
 * @param[out] self Expression to initialize.
 * @param[in] ops Operations provided by the expression.
 * @return EOK or an error code from errno.h. */
errno_t bithenge_init_expression(bithenge_expression_t *self,
    const bithenge_expression_ops_t *ops)
{
	assert(ops);
	assert(ops->evaluate);
	assert(ops->destroy);
	if (bithenge_should_fail())
		return ENOMEM;
	self->ops = ops;
	self->refs = 1;
	return EOK;
}

static void expression_indestructible(bithenge_expression_t *self)
{
	assert(false);
}



/***************** binary_expression                         *****************/

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

static errno_t binary_expression_evaluate(bithenge_expression_t *base,
    bithenge_scope_t *scope, bithenge_node_t **out)
{
	errno_t rc;
	binary_expression_t *self = expression_as_binary(base);
	bithenge_node_t *a, *b;
	rc = bithenge_expression_evaluate(self->a, scope, &a);
	if (rc != EOK)
		return rc;
	if (self->op != BITHENGE_EXPRESSION_CONCAT) {
		rc = bithenge_expression_evaluate(self->b, scope, &b);
		if (rc != EOK) {
			bithenge_node_dec_ref(a);
			return rc;
		}
	}

	/* Check types and get values. */
	/* Assigning 0 only to make the compiler happy. */
	bithenge_int_t a_int = 0, b_int = 0;
	bool a_bool = false, b_bool = false, out_bool = false;
	switch (self->op) {
	case BITHENGE_EXPRESSION_ADD: /* fallthrough */
	case BITHENGE_EXPRESSION_SUBTRACT: /* fallthrough */
	case BITHENGE_EXPRESSION_MULTIPLY: /* fallthrough */
	case BITHENGE_EXPRESSION_INTEGER_DIVIDE: /* fallthrough */
	case BITHENGE_EXPRESSION_MODULO: /* fallthrough */
	case BITHENGE_EXPRESSION_LESS_THAN: /* fallthrough */
	case BITHENGE_EXPRESSION_LESS_THAN_OR_EQUAL: /* fallthrough */
	case BITHENGE_EXPRESSION_GREATER_THAN: /* fallthrough */
	case BITHENGE_EXPRESSION_GREATER_THAN_OR_EQUAL:
		rc = EINVAL;
		if (bithenge_node_type(a) != BITHENGE_NODE_INTEGER)
			goto error;
		if (bithenge_node_type(b) != BITHENGE_NODE_INTEGER)
			goto error;
		a_int = bithenge_integer_node_value(a);
		b_int = bithenge_integer_node_value(b);
		break;
	case BITHENGE_EXPRESSION_AND: /* fallthrough */
	case BITHENGE_EXPRESSION_OR:
		rc = EINVAL;
		if (bithenge_node_type(a) != BITHENGE_NODE_BOOLEAN)
			goto error;
		if (bithenge_node_type(b) != BITHENGE_NODE_BOOLEAN)
			goto error;
		a_bool = bithenge_boolean_node_value(a);
		b_bool = bithenge_boolean_node_value(b);
		break;
	case BITHENGE_EXPRESSION_CONCAT:
		if (bithenge_node_type(a) != BITHENGE_NODE_BLOB)
			goto error;
		break;
	default:
		break;
	}

	switch (self->op) {
	case BITHENGE_EXPRESSION_ADD:
		rc = bithenge_new_integer_node(out, a_int + b_int);
		break;
	case BITHENGE_EXPRESSION_SUBTRACT:
		rc = bithenge_new_integer_node(out, a_int - b_int);
		break;
	case BITHENGE_EXPRESSION_MULTIPLY:
		rc = bithenge_new_integer_node(out, a_int * b_int);
		break;
	case BITHENGE_EXPRESSION_INTEGER_DIVIDE:
		/* Integer division can behave in three major ways when the
		  operands are signed: truncated, floored, or Euclidean. When
		 * b > 0, we give the same result as floored and Euclidean;
		 * otherwise, we currently raise an error. See
		 * https://en.wikipedia.org/wiki/Modulo_operation and its
		 * references. */
		if (b_int <= 0) {
			rc = EINVAL;
			break;
		}
		rc = bithenge_new_integer_node(out,
		    (a_int / b_int) + (a_int % b_int < 0 ? -1 : 0));
		break;
	case BITHENGE_EXPRESSION_MODULO:
		/* This is consistent with division; see above. */
		if (b_int <= 0) {
			rc = EINVAL;
			break;
		}
		rc = bithenge_new_integer_node(out,
		    (a_int % b_int) + (a_int % b_int < 0 ? b_int : 0));
		break;
	case BITHENGE_EXPRESSION_LESS_THAN:
		rc = bithenge_new_boolean_node(out, a_int < b_int);
		break;
	case BITHENGE_EXPRESSION_LESS_THAN_OR_EQUAL:
		rc = bithenge_new_boolean_node(out, a_int <= b_int);
		break;
	case BITHENGE_EXPRESSION_GREATER_THAN:
		rc = bithenge_new_boolean_node(out, a_int > b_int);
		break;
	case BITHENGE_EXPRESSION_GREATER_THAN_OR_EQUAL:
		rc = bithenge_new_boolean_node(out, a_int >= b_int);
		break;
	case BITHENGE_EXPRESSION_EQUALS:
		rc = bithenge_node_equal(&out_bool, a, b);
		if (rc != EOK)
			break;
		rc = bithenge_new_boolean_node(out, out_bool);
		break;
	case BITHENGE_EXPRESSION_NOT_EQUALS:
		rc = bithenge_node_equal(&out_bool, a, b);
		if (rc != EOK)
			break;
		rc = bithenge_new_boolean_node(out, !out_bool);
		break;
	case BITHENGE_EXPRESSION_AND:
		rc = bithenge_new_boolean_node(out, a_bool && b_bool);
		break;
	case BITHENGE_EXPRESSION_OR:
		rc = bithenge_new_boolean_node(out, a_bool || b_bool);
		break;
	case BITHENGE_EXPRESSION_MEMBER:
		rc = bithenge_node_get(a, b, out);
		b = NULL;
		break;
	case BITHENGE_EXPRESSION_CONCAT:
		bithenge_expression_inc_ref(self->b);
		bithenge_scope_inc_ref(scope);
		rc = bithenge_concat_blob_lazy(out, bithenge_node_as_blob(a),
		    self->b, scope);
		a = NULL;
		b = NULL;
		break;
	case BITHENGE_EXPRESSION_INVALID_BINARY_OP:
		assert(false);
		break;
	}

error:
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
errno_t bithenge_binary_expression(bithenge_expression_t **out,
    bithenge_binary_op_t op, bithenge_expression_t *a,
    bithenge_expression_t *b)
{
	errno_t rc;
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



/***************** in_node_expression                        *****************/

static errno_t in_node_evaluate(bithenge_expression_t *self,
    bithenge_scope_t *scope, bithenge_node_t **out)
{
	for (; scope; scope = bithenge_scope_outer(scope)) {
		*out = bithenge_scope_in_node(scope);
		if (*out)
			return EOK;
	}
	return EINVAL;
}

static const bithenge_expression_ops_t in_node_ops = {
	.evaluate = in_node_evaluate,
	.destroy = expression_indestructible,
};

static bithenge_expression_t in_node_expression = {
	&in_node_ops, 1
};

/** Create an expression that gets the current input node.
 * @param[out] out Holds the new expression.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_in_node_expression(bithenge_expression_t **out)
{
	if (bithenge_should_fail())
		return ENOMEM;
	bithenge_expression_inc_ref(&in_node_expression);
	*out = &in_node_expression;
	return EOK;
}



/***************** current_node_expression                   *****************/

static errno_t current_node_evaluate(bithenge_expression_t *self,
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
errno_t bithenge_current_node_expression(bithenge_expression_t **out)
{
	bithenge_expression_inc_ref(&current_node_expression);
	*out = &current_node_expression;
	return EOK;
}



/***************** param_expression                          *****************/

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

static errno_t param_expression_evaluate(bithenge_expression_t *base,
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
errno_t bithenge_param_expression(bithenge_expression_t **out, int index)
{
	errno_t rc;
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



/***************** const_expression                          *****************/

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

static errno_t const_expression_evaluate(bithenge_expression_t *base,
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
errno_t bithenge_const_expression(bithenge_expression_t **out,
    bithenge_node_t *node)
{
	errno_t rc;
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



/***************** scope_member_expression                   *****************/

typedef struct {
	bithenge_expression_t base;
	bithenge_node_t *key;
} scope_member_expression_t;

static scope_member_expression_t *expression_as_scope_member(
    bithenge_expression_t *base)
{
	return (scope_member_expression_t *)base;
}

static bithenge_expression_t *scope_member_as_expression(
    scope_member_expression_t *expr)
{
	return &expr->base;
}

static errno_t scope_member_expression_evaluate(bithenge_expression_t *base,
    bithenge_scope_t *scope, bithenge_node_t **out)
{
	scope_member_expression_t *self = expression_as_scope_member(base);
	for (; scope && !bithenge_scope_is_barrier(scope);
	    scope = bithenge_scope_outer(scope)) {
		bithenge_node_t *cur = bithenge_scope_get_current_node(scope);
		if (!cur)
			continue;
		bithenge_node_inc_ref(self->key);
		errno_t rc = bithenge_node_get(cur, self->key, out);
		bithenge_node_dec_ref(cur);
		if (rc != ENOENT) /* EOK or error */
			return rc;
	}
	return bithenge_scope_error(scope, "No scope member %t", self->key);
}

static void scope_member_expression_destroy(bithenge_expression_t *base)
{
	scope_member_expression_t *self = expression_as_scope_member(base);
	bithenge_node_dec_ref(self->key);
	free(self);
}

static const bithenge_expression_ops_t scope_member_expression_ops = {
	.evaluate = scope_member_expression_evaluate,
	.destroy = scope_member_expression_destroy,
};

/** Create an expression that gets a member from one of the current nodes being
 * created. It searches from the current scope outwards, stopping at barrier
 * scopes.
 * @param[out] out Holds the new expression.
 * @param key The key to search for in nodes being created.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_scope_member_expression(bithenge_expression_t **out,
    bithenge_node_t *key)
{
	errno_t rc;
	scope_member_expression_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_init_expression(scope_member_as_expression(self),
	    &scope_member_expression_ops);
	if (rc != EOK)
		goto error;

	self->key = key;
	*out = scope_member_as_expression(self);
	return EOK;

error:
	bithenge_node_dec_ref(key);
	free(self);
	return rc;
}



/***************** subblob_expression                        *****************/

typedef struct {
	bithenge_expression_t base;
	bithenge_expression_t *blob, *start, *limit;
	bool absolute_limit;
} subblob_expression_t;

static subblob_expression_t *expression_as_subblob(bithenge_expression_t *base)
{
	return (subblob_expression_t *)base;
}

static bithenge_expression_t *subblob_as_expression(subblob_expression_t *expr)
{
	return &expr->base;
}

static errno_t subblob_expression_evaluate(bithenge_expression_t *base,
    bithenge_scope_t *scope, bithenge_node_t **out)
{
	subblob_expression_t *self = expression_as_subblob(base);
	bithenge_node_t *start_node;
	errno_t rc = bithenge_expression_evaluate(self->start, scope, &start_node);
	if (rc != EOK)
		return rc;
	if (bithenge_node_type(start_node) != BITHENGE_NODE_INTEGER) {
		bithenge_node_dec_ref(start_node);
		return EINVAL;
	}
	bithenge_int_t start = bithenge_integer_node_value(start_node);
	bithenge_node_dec_ref(start_node);

	bithenge_int_t limit = -1;
	if (self->limit) {
		bithenge_node_t *limit_node;
		rc = bithenge_expression_evaluate(self->limit, scope,
		    &limit_node);
		if (rc != EOK)
			return rc;
		if (bithenge_node_type(limit_node) != BITHENGE_NODE_INTEGER) {
			bithenge_node_dec_ref(limit_node);
			return EINVAL;
		}
		limit = bithenge_integer_node_value(limit_node);
		bithenge_node_dec_ref(limit_node);
		if (self->absolute_limit)
			limit -= start;
	}

	if (start < 0 || (self->limit && limit < 0))
		return EINVAL;

	bithenge_node_t *blob;
	rc = bithenge_expression_evaluate(self->blob, scope, &blob);
	if (rc != EOK)
		return rc;
	if (bithenge_node_type(blob) != BITHENGE_NODE_BLOB) {
		bithenge_node_dec_ref(blob);
		return EINVAL;
	}

	if (self->limit)
		return bithenge_new_subblob(out, bithenge_node_as_blob(blob),
		    start, limit);
	else
		return bithenge_new_offset_blob(out,
		    bithenge_node_as_blob(blob), start);
}

static void subblob_expression_destroy(bithenge_expression_t *base)
{
	subblob_expression_t *self = expression_as_subblob(base);
	bithenge_expression_dec_ref(self->blob);
	bithenge_expression_dec_ref(self->start);
	bithenge_expression_dec_ref(self->limit);
	free(self);
}

static const bithenge_expression_ops_t subblob_expression_ops = {
	.evaluate = subblob_expression_evaluate,
	.destroy = subblob_expression_destroy,
};

/** Create an expression that gets a subblob. Takes references to @a blob,
 * @a start, and @a limit.
 * @param[out] out Holds the new expression.
 * @param blob Calculates the blob.
 * @param start Calculates the start offset within the blob.
 * @param limit Calculates the limit. Can be NULL, in which case an offset blob
 * is returned.
 * @param absolute_limit If true, the limit is an absolute offset; otherwise,
 * it is relative to the start.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_subblob_expression(bithenge_expression_t **out,
    bithenge_expression_t *blob, bithenge_expression_t *start,
    bithenge_expression_t *limit, bool absolute_limit)
{
	errno_t rc;
	subblob_expression_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_init_expression(subblob_as_expression(self),
	    &subblob_expression_ops);
	if (rc != EOK)
		goto error;

	self->blob = blob;
	self->start = start;
	self->limit = limit;
	self->absolute_limit = absolute_limit;
	*out = subblob_as_expression(self);
	return EOK;

error:
	bithenge_expression_dec_ref(blob);
	bithenge_expression_dec_ref(start);
	bithenge_expression_dec_ref(limit);
	free(self);
	return rc;
}

/***************** param_wrapper                             *****************/

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

static errno_t param_wrapper_fill_scope(param_wrapper_t *self, bithenge_scope_t
    *inner, bithenge_scope_t *outer)
{
	errno_t rc;
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

static errno_t param_wrapper_apply(bithenge_transform_t *base,
    bithenge_scope_t *outer, bithenge_node_t *in, bithenge_node_t **out)
{
	param_wrapper_t *self = transform_as_param_wrapper(base);
	bithenge_scope_t *inner;
	errno_t rc = bithenge_scope_new(&inner, outer);
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

static errno_t param_wrapper_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *outer, bithenge_blob_t *in, aoff64_t *out)
{
	param_wrapper_t *self = transform_as_param_wrapper(base);
	bithenge_scope_t *inner;
	errno_t rc = bithenge_scope_new(&inner, outer);
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

static errno_t param_wrapper_prefix_apply(bithenge_transform_t *base,
    bithenge_scope_t *outer, bithenge_blob_t *in, bithenge_node_t **out_node,
    aoff64_t *out_length)
{
	param_wrapper_t *self = transform_as_param_wrapper(base);
	bithenge_scope_t *inner;
	errno_t rc = bithenge_scope_new(&inner, outer);
	if (rc != EOK)
		return rc;
	rc = param_wrapper_fill_scope(self, inner, outer);
	if (rc != EOK)
		goto error;

	rc = bithenge_transform_prefix_apply(self->transform, inner, in,
	    out_node, out_length);

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
	.prefix_apply = param_wrapper_prefix_apply,
	.destroy = param_wrapper_destroy,
};

/** Create a transform that calculates parameters for another transform. Takes
 * ownership of @a transform, @a params, and each element of @a params. The
 * number of parameters must be correct.
 * @param[out] out Holds the new transform.
 * @param transform The transform for which parameters are calculated.
 * @param params The expressions used to calculate the parameters.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_param_wrapper(bithenge_transform_t **out,
    bithenge_transform_t *transform, bithenge_expression_t **params)
{
	errno_t rc;
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



/***************** expression_transform           *****************/

/* Also used by inputless_transform. */
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

static errno_t expression_transform_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	expression_transform_t *self = transform_as_expression(base);
	bithenge_scope_t *inner;
	errno_t rc = bithenge_scope_new(&inner, scope);
	if (rc != EOK)
		return rc;
	bithenge_scope_set_in_node(inner, in);
	rc = bithenge_expression_evaluate(self->expr, inner, out);
	bithenge_scope_dec_ref(inner);
	return rc;
}

/* Also used by inputless_transform. */
static void expression_transform_destroy(bithenge_transform_t *base)
{
	expression_transform_t *self = transform_as_expression(base);
	bithenge_expression_dec_ref(self->expr);
	free(self);
}

static const bithenge_transform_ops_t expression_transform_ops = {
	.apply = expression_transform_apply,
	.destroy = expression_transform_destroy,
};

/** Create a transform that evaluates an expression on the input node. Takes a
 * reference to the expression.
 * @param[out] out Holds the new transform.
 * @param expr The expression to evaluate.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_expression_transform(bithenge_transform_t **out,
    bithenge_expression_t *expr)
{
	errno_t rc;
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



/***************** inputless_transform            *****************/

static errno_t inputless_transform_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *in, aoff64_t *out)
{
	*out = 0;
	return EOK;
}

static errno_t inputless_transform_prefix_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *in, bithenge_node_t **out_node,
    aoff64_t *out_size)
{
	expression_transform_t *self = transform_as_expression(base);
	if (out_size)
		*out_size = 0;
	return bithenge_expression_evaluate(self->expr, scope, out_node);
}

static const bithenge_transform_ops_t inputless_transform_ops = {
	.prefix_length = inputless_transform_prefix_length,
	.prefix_apply = inputless_transform_prefix_apply,
	.destroy = expression_transform_destroy,
};

/** Create a transform that takes an empty blob and produces the result of an
 * expression. Takes a reference to the expression.
 * @param[out] out Holds the new transform.
 * @param expr The expression to evaluate.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_inputless_transform(bithenge_transform_t **out,
    bithenge_expression_t *expr)
{
	errno_t rc;
	expression_transform_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_init_transform(expression_as_transform(self),
	    &inputless_transform_ops, 0);
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



/***************** concat_blob                    *****************/

typedef struct {
	bithenge_blob_t base;
	bithenge_blob_t *a, *b;
	aoff64_t a_size;
	bithenge_expression_t *b_expr;
	bithenge_scope_t *scope;
} concat_blob_t;

static inline concat_blob_t *blob_as_concat(bithenge_blob_t *base)
{
	return (concat_blob_t *)base;
}

static inline bithenge_blob_t *concat_as_blob(concat_blob_t *blob)
{
	return &blob->base;
}

static errno_t concat_blob_evaluate_b(concat_blob_t *self)
{
	if (self->b)
		return EOK;
	bithenge_node_t *b_node;
	errno_t rc = bithenge_expression_evaluate(self->b_expr, self->scope,
	    &b_node);
	if (rc != EOK)
		return rc;
	if (bithenge_node_type(b_node) != BITHENGE_NODE_BLOB) {
		bithenge_node_dec_ref(b_node);
		return bithenge_scope_error(self->scope,
		    "Concatenation arguments must be blobs");
	}
	self->b = bithenge_node_as_blob(b_node);
	bithenge_expression_dec_ref(self->b_expr);
	bithenge_scope_dec_ref(self->scope);
	self->b_expr = NULL;
	self->scope = NULL;
	return EOK;
}

static errno_t concat_blob_size(bithenge_blob_t *base, aoff64_t *size)
{
	concat_blob_t *self = blob_as_concat(base);
	errno_t rc = concat_blob_evaluate_b(self);
	if (rc != EOK)
		return rc;
	rc = bithenge_blob_size(self->b, size);
	*size += self->a_size;
	return rc;
}

static errno_t concat_blob_read(bithenge_blob_t *base, aoff64_t offset,
    char *buffer, aoff64_t *size)
{
	errno_t rc;
	concat_blob_t *self = blob_as_concat(base);

	aoff64_t a_size = 0, b_size = 0;
	if (offset < self->a_size) {
		a_size = *size;
		rc = bithenge_blob_read(self->a, offset, buffer, &a_size);
		if (rc != EOK)
			return rc;
	}
	if (offset + *size > self->a_size) {
		rc = concat_blob_evaluate_b(self);
		if (rc != EOK)
			return rc;
		b_size = *size - a_size;
		rc = bithenge_blob_read(self->b,
		    offset + a_size - self->a_size, buffer + a_size, &b_size);
		if (rc != EOK)
			return rc;
	}
	assert(a_size + b_size <= *size);
	*size = a_size + b_size;
	return EOK;
}

static errno_t concat_blob_read_bits(bithenge_blob_t *base, aoff64_t offset,
    char *buffer, aoff64_t *size, bool little_endian)
{
	errno_t rc;
	concat_blob_t *self = blob_as_concat(base);

	aoff64_t a_size = 0, b_size = 0;
	if (offset < self->a_size) {
		a_size = *size;
		rc = bithenge_blob_read_bits(self->a, offset, buffer, &a_size,
		    little_endian);
		if (rc != EOK)
			return rc;
	}
	if (offset + *size > self->a_size) {
		rc = concat_blob_evaluate_b(self);
		if (rc != EOK)
			return rc;
		b_size = *size - a_size;
		assert(a_size % 8 == 0); /* TODO: don't require this */
		rc = bithenge_blob_read_bits(self->b,
		    offset + a_size - self->a_size, buffer + a_size / 8,
		    &b_size, little_endian);
		if (rc != EOK)
			return rc;
	}
	assert(a_size + b_size <= *size);
	*size = a_size + b_size;
	return EOK;
}

static void concat_blob_destroy(bithenge_blob_t *base)
{
	concat_blob_t *self = blob_as_concat(base);
	bithenge_blob_dec_ref(self->a);
	bithenge_blob_dec_ref(self->b);
	bithenge_expression_dec_ref(self->b_expr);
	bithenge_scope_dec_ref(self->scope);
	free(self);
}

static const bithenge_random_access_blob_ops_t concat_blob_ops = {
	.size = concat_blob_size,
	.read = concat_blob_read,
	.read_bits = concat_blob_read_bits,
	.destroy = concat_blob_destroy,
};

/** Create a concatenated blob. Takes references to @a a and @a b.
 * @param[out] out Holds the new blob.
 * @param a The first blob.
 * @param b The second blob.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_concat_blob(bithenge_node_t **out, bithenge_blob_t *a,
    bithenge_blob_t *b)
{
	assert(out);
	assert(a);
	assert(b);
	errno_t rc;
	concat_blob_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_blob_size(a, &self->a_size);
	if (rc != EOK)
		goto error;

	rc = bithenge_init_random_access_blob(concat_as_blob(self),
	    &concat_blob_ops);
	if (rc != EOK)
		goto error;
	self->a = a;
	self->b = b;
	self->b_expr = NULL;
	self->scope = NULL;
	*out = bithenge_blob_as_node(concat_as_blob(self));
	return EOK;

error:
	bithenge_blob_dec_ref(a);
	bithenge_blob_dec_ref(b);
	free(self);
	return rc;
}

/** Create a lazy concatenated blob. Takes references to @a a, @a b_expr, and
 * @a scope.
 * @param[out] out Holds the new blob.
 * @param a The first blob.
 * @param b_expr An expression to calculate the second blob.
 * @param scope The scope in which @a b_expr should be evaluated.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_concat_blob_lazy(bithenge_node_t **out, bithenge_blob_t *a,
    bithenge_expression_t *b_expr, bithenge_scope_t *scope)
{
	assert(out);
	assert(a);
	assert(b_expr);
	assert(scope);
	errno_t rc;
	concat_blob_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_blob_size(a, &self->a_size);
	if (rc != EOK)
		goto error;

	rc = bithenge_init_random_access_blob(concat_as_blob(self),
	    &concat_blob_ops);
	if (rc != EOK)
		goto error;
	self->a = a;
	self->b = NULL;
	self->b_expr = b_expr;
	self->scope = scope;
	*out = bithenge_blob_as_node(concat_as_blob(self));
	return EOK;

error:
	bithenge_blob_dec_ref(a);
	bithenge_expression_dec_ref(b_expr);
	bithenge_scope_dec_ref(scope);
	free(self);
	return rc;
}

/** @}
 */
