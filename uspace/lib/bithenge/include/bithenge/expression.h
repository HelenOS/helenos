/*
 * SPDX-FileCopyrightText: 2012 Sean Bartell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup bithenge
 * @{
 */
/**
 * @file
 * Expressions.
 */

#ifndef BITHENGE_EXPRESSION_H_
#define BITHENGE_EXPRESSION_H_

#include "blob.h"
#include "transform.h"
#include "tree.h"

/** An expression that calculates a value given a scope. */
typedef struct {
	/** @privatesection */
	const struct bithenge_expression_ops *ops;
	unsigned int refs;
} bithenge_expression_t;

/** Operations provided by an expression. */
typedef struct bithenge_expression_ops {
	/** @copydoc bithenge_expression_t::bithenge_expression_evaluate */
	errno_t (*evaluate)(bithenge_expression_t *self, bithenge_scope_t *scope,
	    bithenge_node_t **out);
	/** Destroy the expression.
	 * @param self The expression.
	 */
	void (*destroy)(bithenge_expression_t *self);
} bithenge_expression_ops_t;

/** Increment an expression's reference count.
 * @param self The expression to reference.
 */
static inline void bithenge_expression_inc_ref(bithenge_expression_t *self)
{
	assert(self);
	self->refs++;
}

/** Decrement an expression's reference count and free it if appropriate.
 * @param self The expression to dereference, or NULL.
 */
static inline void bithenge_expression_dec_ref(bithenge_expression_t *self)
{
	if (!self)
		return;
	assert(self->ops);
	assert(self->refs > 0);
	if (--self->refs == 0)
		self->ops->destroy(self);
}

/** Evaluate an expression. Takes ownership of nothing.
 * @memberof bithenge_expression_t
 * @param self The expression.
 * @param scope The scope.
 * @param[out] out Where the output tree will be stored.
 * @return EOK on success or an error code from errno.h.
 */
static inline errno_t bithenge_expression_evaluate(bithenge_expression_t *self,
    bithenge_scope_t *scope, bithenge_node_t **out)
{
	assert(self);
	assert(self->ops);
	return self->ops->evaluate(self, scope, out);
}

/** The binary operators supported by @a bithenge_binary_expression(). */
typedef enum {
	BITHENGE_EXPRESSION_INVALID_BINARY_OP,

	BITHENGE_EXPRESSION_ADD,
	BITHENGE_EXPRESSION_SUBTRACT,
	BITHENGE_EXPRESSION_MULTIPLY,
	BITHENGE_EXPRESSION_INTEGER_DIVIDE,
	BITHENGE_EXPRESSION_MODULO,

	BITHENGE_EXPRESSION_LESS_THAN,
	BITHENGE_EXPRESSION_GREATER_THAN,
	BITHENGE_EXPRESSION_LESS_THAN_OR_EQUAL,
	BITHENGE_EXPRESSION_GREATER_THAN_OR_EQUAL,
	BITHENGE_EXPRESSION_EQUALS,
	BITHENGE_EXPRESSION_NOT_EQUALS,

	BITHENGE_EXPRESSION_AND,
	BITHENGE_EXPRESSION_OR,

	BITHENGE_EXPRESSION_MEMBER,
	BITHENGE_EXPRESSION_CONCAT,
} bithenge_binary_op_t;

errno_t bithenge_init_expression(bithenge_expression_t *,
    const bithenge_expression_ops_t *);
errno_t bithenge_binary_expression(bithenge_expression_t **, bithenge_binary_op_t,
    bithenge_expression_t *, bithenge_expression_t *);
errno_t bithenge_in_node_expression(bithenge_expression_t **);
errno_t bithenge_current_node_expression(bithenge_expression_t **);
errno_t bithenge_param_expression(bithenge_expression_t **, int);
errno_t bithenge_const_expression(bithenge_expression_t **, bithenge_node_t *);
errno_t bithenge_scope_member_expression(bithenge_expression_t **,
    bithenge_node_t *);
errno_t bithenge_subblob_expression(bithenge_expression_t **,
    bithenge_expression_t *, bithenge_expression_t *, bithenge_expression_t *,
    bool);
errno_t bithenge_param_wrapper(bithenge_transform_t **, bithenge_transform_t *,
    bithenge_expression_t **);
errno_t bithenge_expression_transform(bithenge_transform_t **,
    bithenge_expression_t *);
errno_t bithenge_inputless_transform(bithenge_transform_t **,
    bithenge_expression_t *);

errno_t bithenge_concat_blob(bithenge_node_t **, bithenge_blob_t *,
    bithenge_blob_t *);
errno_t bithenge_concat_blob_lazy(bithenge_node_t **, bithenge_blob_t *,
    bithenge_expression_t *, bithenge_scope_t *);

#endif

/** @}
 */
