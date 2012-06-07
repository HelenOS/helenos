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

#include <errno.h>
#include <stdlib.h>
#include "tree.h"

int bithenge_node_destroy(bithenge_node_t *node)
{
	switch (bithenge_node_type(node)) {
	case BITHENGE_NODE_STRING:
		if (node->string_value.needs_free)
			free(node->string_value.ptr);
		break;
	case BITHENGE_NODE_INTERNAL:
		/* TODO */
		break;
	case BITHENGE_NODE_BOOLEAN:
		return EOK; // the boolean nodes are allocated statically below
	case BITHENGE_NODE_INTEGER: /* pass-through */
	case BITHENGE_NODE_NONE:
		break;
	}
	free(node);
	return EOK;
}

typedef struct
{
	bithenge_node_t base;
	bithenge_node_t **nodes;
	bithenge_int_t len;
	bool needs_free;
} simple_internal_node_t;

static simple_internal_node_t *node_as_simple(bithenge_node_t *node)
{
	return (simple_internal_node_t *)node;
}

static int simple_internal_node_for_each(bithenge_node_t *base, bithenge_for_each_func_t func, void *data)
{
	int rc;
	simple_internal_node_t *node = node_as_simple(base);
	for (bithenge_int_t i = 0; i < node->len; i++) {
		rc = func(node->nodes[2*i+0], node->nodes[2*i+1], data);
		if (rc != EOK)
			return rc;
	}
	return EOK;
}

static bithenge_internal_node_ops_t simple_internal_node_ops = {
	.for_each = simple_internal_node_for_each
};

static bithenge_node_t *simple_internal_as_node(simple_internal_node_t *node)
{
	return &node->base;
}

int bithenge_new_simple_internal_node(bithenge_node_t **out, bithenge_node_t **nodes, bithenge_int_t len, bool needs_free)
{
	assert(out);
	simple_internal_node_t *node = malloc(sizeof(*node));
	if (!node)
		return ENOMEM;
	node->base.type = BITHENGE_NODE_INTERNAL;
	node->base.internal_ops = &simple_internal_node_ops;
	node->nodes = nodes;
	node->len = len;
	node->needs_free = needs_free;
	*out = simple_internal_as_node(node);
	return EOK;
}

static bithenge_node_t false_node = { BITHENGE_NODE_BOOLEAN, .boolean_value = false };
static bithenge_node_t true_node = { BITHENGE_NODE_BOOLEAN, .boolean_value = true };

int bithenge_new_boolean_node(bithenge_node_t **out, bool value)
{
	assert(out);
	*out = value ? &true_node : &false_node;
	return EOK;
}

int bithenge_new_integer_node(bithenge_node_t **out, bithenge_int_t value)
{
	assert(out);
	bithenge_node_t *node = malloc(sizeof(*node));
	if (!node)
		return ENOMEM;
	node->type = BITHENGE_NODE_INTEGER;
	node->integer_value = value;
	*out = node;
	return EOK;
}

int bithenge_new_string_node(bithenge_node_t **out, const char *value, bool needs_free)
{
	assert(out);
	bithenge_node_t *node = malloc(sizeof(*node));
	if (!node)
		return ENOMEM;
	node->type = BITHENGE_NODE_STRING;
	node->string_value.ptr = value;
	node->string_value.needs_free = needs_free;
	*out = node;
	return EOK;
}
