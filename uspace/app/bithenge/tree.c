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
#include "blob.h"
#include "os.h"
#include "tree.h"

static int blob_destroy(bithenge_node_t *base)
{
	bithenge_blob_t *blob = bithenge_node_as_blob(base);
	assert(blob->base.blob_ops);
	return blob->base.blob_ops->destroy(blob);
}

static int node_destroy(bithenge_node_t *node)
{
	switch (bithenge_node_type(node)) {
	case BITHENGE_NODE_BLOB:
		return blob_destroy(node);
	case BITHENGE_NODE_STRING:
		if (node->string_value.needs_free)
			free((void *)node->string_value.ptr);
		break;
	case BITHENGE_NODE_INTERNAL:
		return node->internal_ops->destroy(node);
	case BITHENGE_NODE_BOOLEAN:
		return EOK; // the boolean nodes are allocated statically below
	case BITHENGE_NODE_INTEGER: /* pass-through */
		break;
	}
	free(node);
	return EOK;
}

/** Decrement a node's reference count and free it if appropriate.
 * @memberof bithenge_node_t
 * @param node The node to dereference, or NULL.
 * @return EOK on success or an error code from errno.h. */
int bithenge_node_dec_ref(bithenge_node_t *node)
{
	if (!node)
		return EOK;
	if (--node->refs == 0)
		return node_destroy(node);
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

static bithenge_node_t *simple_as_node(simple_internal_node_t *node)
{
	return &node->base;
}

static int simple_internal_node_for_each(bithenge_node_t *base,
    bithenge_for_each_func_t func, void *data)
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

static int simple_internal_node_destroy(bithenge_node_t *base)
{
	int rc;
	simple_internal_node_t *node = node_as_simple(base);
	for (bithenge_int_t i = 0; i < 2 * node->len; i++) {
		rc = bithenge_node_dec_ref(node->nodes[i]);
		if (rc != EOK)
			return rc;
	}
	if (node->needs_free)
		free(node->nodes);
	free(node);
	return EOK;
}

static bithenge_internal_node_ops_t simple_internal_node_ops = {
	.for_each = simple_internal_node_for_each,
	.destroy = simple_internal_node_destroy,
};

/** Create an internal node from a set of keys and values. This function takes
 * ownership of a reference to the key and value nodes, and optionally the
 * array @a nodes.
 * @memberof bithenge_node_t
 * @param[out] out Stores the created internal node.
 * @param nodes The array of key-value pairs. Keys are stored at even indices
 * and values are stored at odd indices.
 * @param len The number of key-value pairs in the node array.
 * @param needs_free If true, when the internal node is destroyed it will free
 * the nodes array rather than just dereferencing each node inside it.
 * @return EOK on success or an error code from errno.h. */
int bithenge_new_simple_internal_node(bithenge_node_t **out,
    bithenge_node_t **nodes, bithenge_int_t len, bool needs_free)
{
	assert(out);
	simple_internal_node_t *node = malloc(sizeof(*node));
	if (!node) {
		for (bithenge_int_t i = 0; i < 2 * len; i++)
			bithenge_node_dec_ref(nodes[i]);
		if (needs_free)
			free(nodes);
		return ENOMEM;
	}
	node->base.type = BITHENGE_NODE_INTERNAL;
	node->base.refs = 1;
	node->base.internal_ops = &simple_internal_node_ops;
	node->nodes = nodes;
	node->len = len;
	node->needs_free = needs_free;
	*out = simple_as_node(node);
	return EOK;
}

static bithenge_node_t false_node = { BITHENGE_NODE_BOOLEAN, 1, .boolean_value = false };
static bithenge_node_t true_node = { BITHENGE_NODE_BOOLEAN, 1, .boolean_value = true };

/** Create a boolean node.
 * @memberof bithenge_node_t
 * @param[out] out Stores the created boolean node.
 * @param value The value for the node to hold.
 * @return EOK on success or an error code from errno.h. */
int bithenge_new_boolean_node(bithenge_node_t **out, bool value)
{
	assert(out);
	*out = value ? &true_node : &false_node;
	(*out)->refs++;
	return EOK;
}

/** Create an integer node.
 * @memberof bithenge_node_t
 * @param[out] out Stores the created integer node.
 * @param value The value for the node to hold.
 * @return EOK on success or an error code from errno.h. */
int bithenge_new_integer_node(bithenge_node_t **out, bithenge_int_t value)
{
	assert(out);
	bithenge_node_t *node = malloc(sizeof(*node));
	if (!node)
		return ENOMEM;
	node->type = BITHENGE_NODE_INTEGER;
	node->refs = 1;
	node->integer_value = value;
	*out = node;
	return EOK;
}

/** Create a string node.
 * @memberof bithenge_node_t
 * @param[out] out Stores the created string node.
 * @param value The value for the node to hold.
 * @param needs_free Whether the string should be freed when the node is
 * destroyed.
 * @return EOK on success or an error code from errno.h. */
int bithenge_new_string_node(bithenge_node_t **out, const char *value, bool needs_free)
{
	assert(out);
	bithenge_node_t *node = malloc(sizeof(*node));
	if (!node)
		return ENOMEM;
	node->type = BITHENGE_NODE_STRING;
	node->refs = 1;
	node->string_value.ptr = value;
	node->string_value.needs_free = needs_free;
	*out = node;
	return EOK;
}

/** Check whether the contents of two nodes are equal. Does not yet work for
 * internal nodes.
 * @memberof bithenge_node_t
 * @param a, b Nodes to compare.
 * @return Whether the nodes are equal. If an error occurs, returns false.
 * @todo Add support for internal nodes.
 */
bool bithenge_node_equal(bithenge_node_t *a, bithenge_node_t *b)
{
	if (a->type != b->type)
		return false;
	switch (a->type) {
	case BITHENGE_NODE_INTERNAL:
		return false;
	case BITHENGE_NODE_BOOLEAN:
		return a->boolean_value == b->boolean_value;
	case BITHENGE_NODE_INTEGER:
		return a->integer_value == b->integer_value;
	case BITHENGE_NODE_STRING:
		return !str_cmp(a->string_value.ptr, b->string_value.ptr);
	case BITHENGE_NODE_BLOB:
		return bithenge_blob_equal(bithenge_node_as_blob(a),
		    bithenge_node_as_blob(b));
	}
	return false;
}

/** @}
 */
