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
	BITHENGE_NODE_BLOB,
} bithenge_node_type_t;

typedef struct bithenge_node_t {
	bithenge_node_type_t type;
	union {
		const struct bithenge_internal_node_ops_t *internal_ops;
		bool boolean_value;
		bithenge_int_t integer_value;
		struct {
			const char *ptr;
			bool needs_free;
		} string_value;
		const struct bithenge_random_access_blob_ops_t *blob_ops;
	};
} bithenge_node_t;

static inline bithenge_node_type_t bithenge_node_type(const bithenge_node_t *node)
{
	return node->type;
}

typedef int (*bithenge_for_each_func_t)(bithenge_node_t *key, bithenge_node_t *value, void *data);

typedef struct bithenge_internal_node_ops_t {
	int (*for_each)(bithenge_node_t *node, bithenge_for_each_func_t func, void *data);
	int (*destroy)(bithenge_node_t *node);
} bithenge_internal_node_ops_t;

static inline int bithenge_node_for_each(bithenge_node_t *node, bithenge_for_each_func_t func, void *data)
{
	assert(node->type == BITHENGE_NODE_INTERNAL);
	return node->internal_ops->for_each(node, func, data);
}

static inline bool bithenge_boolean_node_value(bithenge_node_t *node)
{
	assert(node->type == BITHENGE_NODE_BOOLEAN);
	return node->boolean_value;
}

static inline bithenge_int_t bithenge_integer_node_value(bithenge_node_t *node)
{
	assert(node->type == BITHENGE_NODE_INTEGER);
	return node->integer_value;
}

static inline const char *bithenge_string_node_value(bithenge_node_t *node)
{
	assert(node->type == BITHENGE_NODE_STRING);
	return node->string_value.ptr;
}

int bithenge_new_simple_internal_node(bithenge_node_t **, bithenge_node_t **, bithenge_int_t, bool needs_free);
int bithenge_new_boolean_node(bithenge_node_t **, bool);
int bithenge_new_integer_node(bithenge_node_t **, bithenge_int_t);
int bithenge_new_string_node(bithenge_node_t **, const char *, bool);
int bithenge_node_destroy(bithenge_node_t *);

#endif

/** @}
 */
