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
 * Compound transforms.
 */

#include <stdlib.h>
#include <bithenge/compound.h>
#include <bithenge/expression.h>
#include <bithenge/transform.h>
#include <bithenge/tree.h>
#include "common.h"



/***************** compose_transform                         *****************/

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

static errno_t compose_apply(bithenge_transform_t *base, bithenge_scope_t *scope,
    bithenge_node_t *in, bithenge_node_t **out)
{
	errno_t rc;
	compose_transform_t *self = transform_as_compose(base);
	bithenge_node_inc_ref(in);

	/* i ranges from (self->num - 1) to 0 inside the loop. */
	size_t i = self->num;
	while (i-- != 0) {
		bithenge_node_t *tmp;
		rc = bithenge_transform_apply(self->xforms[i], scope, in,
		    &tmp);
		bithenge_node_dec_ref(in);
		if (rc != EOK)
			return rc;
		in = tmp;
	}

	*out = in;
	return EOK;
}

static errno_t compose_prefix_length(bithenge_transform_t *base,
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
errno_t bithenge_new_composed_transform(bithenge_transform_t **out,
    bithenge_transform_t **xforms, size_t num)
{
	if (num == 0) {
		/* TODO: optimize */
	} else if (num == 1) {
		*out = xforms[0];
		free(xforms);
		return EOK;
	}

	errno_t rc;
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



/***************** if_transform                              *****************/

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

static errno_t if_transform_choose(if_transform_t *self, bithenge_scope_t *scope,
    bool *out)
{
	bithenge_node_t *cond_node;
	errno_t rc = bithenge_expression_evaluate(self->expr, scope, &cond_node);
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

static errno_t if_transform_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	if_transform_t *self = transform_as_if(base);
	bool cond;
	errno_t rc = if_transform_choose(self, scope, &cond);
	if (rc != EOK)
		return rc;
	return bithenge_transform_apply(
	    cond ? self->true_xform : self->false_xform, scope, in, out);
}

static errno_t if_transform_prefix_length(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_blob_t *in, aoff64_t *out)
{
	if_transform_t *self = transform_as_if(base);
	bool cond;
	errno_t rc = if_transform_choose(self, scope, &cond);
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
errno_t bithenge_if_transform(bithenge_transform_t **out,
    bithenge_expression_t *expr, bithenge_transform_t *true_xform,
    bithenge_transform_t *false_xform)
{
	errno_t rc;
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



/***************** partial_transform                         *****************/

typedef struct {
	bithenge_transform_t base;
	bithenge_transform_t *xform;
} partial_transform_t;

static inline bithenge_transform_t *partial_as_transform(
    partial_transform_t *self)
{
	return &self->base;
}

static inline partial_transform_t *transform_as_partial(
    bithenge_transform_t *base)
{
	return (partial_transform_t *)base;
}

static errno_t partial_transform_apply(bithenge_transform_t *base,
    bithenge_scope_t *scope, bithenge_node_t *in, bithenge_node_t **out)
{
	partial_transform_t *self = transform_as_partial(base);
	if (bithenge_node_type(in) != BITHENGE_NODE_BLOB)
		return EINVAL;
	return bithenge_transform_prefix_apply(self->xform, scope,
	    bithenge_node_as_blob(in), out, NULL);
}

static void partial_transform_destroy(bithenge_transform_t *base)
{
	partial_transform_t *self = transform_as_partial(base);
	bithenge_transform_dec_ref(self->xform);
	free(self);
}

static const bithenge_transform_ops_t partial_transform_ops = {
	.apply = partial_transform_apply,
	.destroy = partial_transform_destroy,
};

/** Create a transform that doesn't require its subtransform to use the whole
 * input. Takes a reference to @a xform.
 * @param[out] out Holds the new transform.
 * @param xform The subtransform to apply.
 * @return EOK on success or an error code from errno.h. */
errno_t bithenge_partial_transform(bithenge_transform_t **out,
    bithenge_transform_t *xform)
{
	errno_t rc;
	partial_transform_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}

	rc = bithenge_init_transform(partial_as_transform(self),
	    &partial_transform_ops, 0);
	if (rc != EOK)
		goto error;

	self->xform = xform;
	*out = partial_as_transform(self);
	return EOK;

error:
	free(self);
	bithenge_transform_dec_ref(xform);
	return rc;
}

/** @}
 */
