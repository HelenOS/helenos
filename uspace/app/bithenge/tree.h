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
 * Trees and nodes.
 */

#ifndef BITHENGE_TREE_H_
#define BITHENGE_TREE_H_

#include <assert.h>
#include <bool.h>
#include <inttypes.h>
#include <sys/types.h>

#ifdef INTMAX_MAX
typedef intmax_t bithenge_int_t;
#define BITHENGE_PRId PRIdMAX
#else
typedef int64_t bithenge_int_t;
#define BITHENGE_PRId PRId64
#endif

typedef enum {
	BITHENGE_NODE_NONE,
	BITHENGE_NODE_INTERNAL,
	BITHENGE_NODE_BOOLEAN,
	BITHENGE_NODE_INTEGER,
	BITHENGE_NODE_STRING,
	// TODO: BITHENGE_NODE_BLOB,
} bithenge_node_type_t;

typedef struct bithenge_node_t {
	bithenge_node_type_t type;
} bithenge_node_t;

static inline bithenge_node_type_t bithenge_node_type(const bithenge_node_t *node)
{
	return node->type;
}

typedef int (*bithenge_for_each_func_t)(bithenge_node_t *key, bithenge_node_t *value, void *data);

typedef struct {
	bithenge_node_t base;
	const struct bithenge_internal_node_ops_t *ops;
} bithenge_internal_node_t;

typedef struct bithenge_internal_node_ops_t {
	int (*for_each)(bithenge_internal_node_t *node, bithenge_for_each_func_t func, void *data);
} bithenge_internal_node_ops_t;

typedef struct {
	bithenge_node_t base;
	bool value;
} bithenge_boolean_node_t;

typedef struct {
	bithenge_node_t base;
	bithenge_int_t value;
} bithenge_integer_node_t;

typedef struct {
	bithenge_node_t base;
	const char *value;
	bool needs_free;
} bithenge_string_node_t;

static inline bithenge_node_t *bithenge_internal_as_node(bithenge_internal_node_t *node)
{
	return &node->base;
}

static inline bithenge_internal_node_t *bithenge_as_internal_node(bithenge_node_t *node)
{
	assert(node->type == BITHENGE_NODE_INTERNAL);
	return (bithenge_internal_node_t *)node;
}

static inline int bithenge_node_for_each(bithenge_internal_node_t *node, bithenge_for_each_func_t func, void *data)
{
	return node->ops->for_each(node, func, data);
}

static inline bithenge_node_t *bithenge_boolean_as_node(bithenge_boolean_node_t *node)
{
	return &node->base;
}

static inline bithenge_boolean_node_t *bithenge_as_boolean_node(bithenge_node_t *node)
{
	assert(node->type == BITHENGE_NODE_BOOLEAN);
	return (bithenge_boolean_node_t *)node;
}

static inline bool bithenge_boolean_node_value(bithenge_boolean_node_t *node)
{
	return node->value;
}

static inline bithenge_node_t *bithenge_integer_as_node(bithenge_integer_node_t *node)
{
	return &node->base;
}

static inline bithenge_integer_node_t *bithenge_as_integer_node(bithenge_node_t *node)
{
	assert(node->type == BITHENGE_NODE_INTEGER);
	return (bithenge_integer_node_t *)node;
}

static inline bithenge_int_t bithenge_integer_node_value(bithenge_integer_node_t *node)
{
	return node->value;
}

static inline bithenge_node_t *bithenge_string_as_node(bithenge_string_node_t *node)
{
	return &node->base;
}

static inline bithenge_string_node_t *bithenge_as_string_node(bithenge_node_t *node)
{
	assert(node->type == BITHENGE_NODE_STRING);
	return (bithenge_string_node_t *)node;
}

static inline const char *bithenge_string_node_value(bithenge_string_node_t *node)
{
	return node->value;
}

int bithenge_new_simple_internal_node(bithenge_node_t **, bithenge_node_t **, bithenge_int_t, bool needs_free);
int bithenge_new_boolean_node(bithenge_node_t **, bool);
int bithenge_new_integer_node(bithenge_node_t **, bithenge_int_t);
int bithenge_new_string_node(bithenge_node_t **, const char *, bool);
int bithenge_node_destroy(bithenge_node_t *);

#endif

/** @}
 */
