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
#include <bithenge/blob.h>
#include <bithenge/tree.h>
#include "common.h"

static void blob_destroy(bithenge_node_t *base)
{
	bithenge_blob_t *self = bithenge_node_as_blob(base);
	assert(self->base.blob_ops);
	self->base.blob_ops->destroy(self);
}

static void node_destroy(bithenge_node_t *self)
{
	switch (bithenge_node_type(self)) {
	case BITHENGE_NODE_BLOB:
		blob_destroy(self);
		return;
	case BITHENGE_NODE_STRING:
		if (self->string_value.needs_free)
			free((void *)self->string_value.ptr);
		break;
	case BITHENGE_NODE_INTERNAL:
		self->internal_ops->destroy(self);
		return;
	case BITHENGE_NODE_BOOLEAN:
		return; /* The boolean nodes are allocated statically below. */
	case BITHENGE_NODE_INTEGER: /* pass-through */
		break;
	}
	free(self);
}

/** Decrement a node's reference count and free it if appropriate.
 * @memberof bithenge_node_t
 * @param node The node to dereference, or NULL.
 */
void bithenge_node_dec_ref(bithenge_node_t *node)
{
	if (!node)
		return;
	assert (node->refs > 0);
	if (--node->refs == 0)
		node_destroy(node);
}

typedef struct {
	bithenge_node_t *key;
	bithenge_node_t **out;
} get_for_each_data_t;

static errno_t get_for_each_func(bithenge_node_t *key, bithenge_node_t *value,
    void *raw_data)
{
	get_for_each_data_t *data = (get_for_each_data_t *)raw_data;
	bool equal;
	errno_t rc = bithenge_node_equal(&equal, key, data->key);
	bithenge_node_dec_ref(key);
	if (rc != EOK)
		return rc;
	if (equal) {
		*data->out = value;
		return EEXIST;
	}
	bithenge_node_dec_ref(value);
	return EOK;
}

/** Get a child of a node. Takes ownership of the key. If the node does not
 * provide this function, for_each will be used as an alternative, which may be
 * very slow. Also works for blob nodes to find the byte value at a given
 * index.
 * @memberof bithenge_node_t
 * @param self The internal/blob node to find a child of.
 * @param key The key to search for.
 * @param[out] out Holds the found node.
 * @return EOK on success, ENOENT if not found, or another error code from
 * errno.h.
 */
errno_t bithenge_node_get(bithenge_node_t *self, bithenge_node_t *key,
    bithenge_node_t **out)
{
	if (self->type == BITHENGE_NODE_BLOB) {
		if (bithenge_node_type(key) != BITHENGE_NODE_INTEGER) {
			bithenge_node_dec_ref(key);
			return ENOENT;
		}
		bithenge_int_t offset = bithenge_integer_node_value(key);
		bithenge_node_dec_ref(key);
		uint8_t byte;
		aoff64_t size = 1;
		errno_t rc = bithenge_blob_read(bithenge_node_as_blob(self),
		    offset, (char *)&byte, &size);
		if (rc != EOK)
			return rc;
		if (size != 1)
			return ENOENT;

		return bithenge_new_integer_node(out, byte);
	}

	assert(self->type == BITHENGE_NODE_INTERNAL);
	if (self->internal_ops->get)
		return self->internal_ops->get(self, key, out);
	*out = NULL;
	get_for_each_data_t data = { key, out };
	errno_t rc = bithenge_node_for_each(self, get_for_each_func, &data);
	bithenge_node_dec_ref(key);
	if (rc == EEXIST && *out)
		return EOK;
	if (rc == EOK)
		rc = ENOENT;
	bithenge_node_dec_ref(*out);
	return rc;
}

/** Initialize an internal node.
 * @memberof bithenge_node_t
 * @param[out] self The node.
 * @param[in] ops The operations provided.
 * @return EOK on success or an error code from errno.h.
 */
errno_t bithenge_init_internal_node(bithenge_node_t *self,
    const bithenge_internal_node_ops_t *ops)
{
	self->type = BITHENGE_NODE_INTERNAL;
	self->refs = 1;
	self->internal_ops = ops;
	return EOK;
}

static void internal_node_indestructible(bithenge_node_t *self)
{
	assert(false);
}

static errno_t empty_internal_node_for_each(bithenge_node_t *base,
    bithenge_for_each_func_t func, void *data)
{
	return EOK;
}

static errno_t empty_internal_node_get(bithenge_node_t *self, bithenge_node_t *key,
    bithenge_node_t **out)
{
	return ENOENT;
}

static const bithenge_internal_node_ops_t empty_internal_node_ops = {
	.for_each = empty_internal_node_for_each,
	.get = empty_internal_node_get,
	.destroy = internal_node_indestructible,
};

static bithenge_node_t empty_internal_node = {
	BITHENGE_NODE_INTERNAL,
	1,
	{ .internal_ops = &empty_internal_node_ops },
};

/** Create an empty internal node.
 * @param[out] out Holds the created node.
 * @return EOK on success or an error code from errno.h.
 */
errno_t bithenge_new_empty_internal_node(bithenge_node_t **out)
{
	if (bithenge_should_fail())
		return ENOMEM;
	bithenge_node_inc_ref(&empty_internal_node);
	*out = &empty_internal_node;
	return EOK;
}

typedef struct {
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

static errno_t simple_internal_node_for_each(bithenge_node_t *base,
    bithenge_for_each_func_t func, void *data)
{
	errno_t rc;
	simple_internal_node_t *self = node_as_simple(base);
	for (bithenge_int_t i = 0; i < self->len; i++) {
		bithenge_node_inc_ref(self->nodes[2 * i + 0]);
		bithenge_node_inc_ref(self->nodes[2 * i + 1]);
		rc = func(self->nodes[2 * i + 0], self->nodes[2 * i + 1], data);
		if (rc != EOK)
			return rc;
	}
	return EOK;
}

static void simple_internal_node_destroy(bithenge_node_t *base)
{
	simple_internal_node_t *self = node_as_simple(base);
	for (bithenge_int_t i = 0; i < 2 * self->len; i++)
		bithenge_node_dec_ref(self->nodes[i]);
	if (self->needs_free)
		free(self->nodes);
	free(self);
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
 * @return EOK on success or an error code from errno.h.
 */
errno_t bithenge_new_simple_internal_node(bithenge_node_t **out,
    bithenge_node_t **nodes, bithenge_int_t len, bool needs_free)
{
	errno_t rc;
	assert(out);
	simple_internal_node_t *self = malloc(sizeof(*self));
	if (!self) {
		rc = ENOMEM;
		goto error;
	}
	rc = bithenge_init_internal_node(simple_as_node(self),
	    &simple_internal_node_ops);
	if (rc != EOK)
		goto error;
	self->nodes = nodes;
	self->len = len;
	self->needs_free = needs_free;
	*out = simple_as_node(self);
	return EOK;
error:
	for (bithenge_int_t i = 0; i < 2 * len; i++)
		bithenge_node_dec_ref(nodes[i]);
	if (needs_free)
		free(nodes);
	free(self);
	return rc;
}

static bithenge_node_t false_node = { BITHENGE_NODE_BOOLEAN, 1, .boolean_value = false };
static bithenge_node_t true_node = { BITHENGE_NODE_BOOLEAN, 1, .boolean_value = true };

/** Create a boolean node.
 * @memberof bithenge_node_t
 * @param[out] out Stores the created boolean node.
 * @param value The value for the node to hold.
 * @return EOK on success or an error code from errno.h.
 */
errno_t bithenge_new_boolean_node(bithenge_node_t **out, bool value)
{
	assert(out);
	if (bithenge_should_fail())
		return ENOMEM;
	*out = value ? &true_node : &false_node;
	(*out)->refs++;
	return EOK;
}

/** Create an integer node.
 * @memberof bithenge_node_t
 * @param[out] out Stores the created integer node.
 * @param value The value for the node to hold.
 * @return EOK on success or an error code from errno.h.
 */
errno_t bithenge_new_integer_node(bithenge_node_t **out, bithenge_int_t value)
{
	assert(out);
	bithenge_node_t *self = malloc(sizeof(*self));
	if (!self)
		return ENOMEM;
	self->type = BITHENGE_NODE_INTEGER;
	self->refs = 1;
	self->integer_value = value;
	*out = self;
	return EOK;
}

/** Create a string node.
 * @memberof bithenge_node_t
 * @param[out] out Stores the created string node. On error, this is unchanged.
 * @param value The value for the node to hold.
 * @param needs_free Whether the string should be freed when the node is
 * destroyed.
 * @return EOK on success or an error code from errno.h.
 */
errno_t bithenge_new_string_node(bithenge_node_t **out, const char *value, bool needs_free)
{
	assert(out);
	bithenge_node_t *self = malloc(sizeof(*self));
	if (!self) {
		if (needs_free)
			free((void *)value);
		return ENOMEM;
	}
	self->type = BITHENGE_NODE_STRING;
	self->refs = 1;
	self->string_value.ptr = value;
	self->string_value.needs_free = needs_free;
	*out = self;
	return EOK;
}

/** Check whether the contents of two nodes are equal. Does not yet work for
 * internal nodes. Takes ownership of nothing.
 * @memberof bithenge_node_t
 * @param[out] out Holds whether the nodes are equal.
 * @param a, b Nodes to compare.
 * @return EOK on success or an error code from errno.h.
 * @todo Add support for internal nodes.
 */
errno_t bithenge_node_equal(bool *out, bithenge_node_t *a, bithenge_node_t *b)
{
	if (a->type != b->type) {
		*out = false;
		return EOK;
	}
	switch (a->type) {
	case BITHENGE_NODE_INTERNAL:
		*out = false;
		return EOK;
	case BITHENGE_NODE_BOOLEAN:
		*out = a->boolean_value == b->boolean_value;
		return EOK;
	case BITHENGE_NODE_INTEGER:
		*out = a->integer_value == b->integer_value;
		return EOK;
	case BITHENGE_NODE_STRING:
		*out = !str_cmp(a->string_value.ptr, b->string_value.ptr);
		return EOK;
	case BITHENGE_NODE_BLOB:
		return bithenge_blob_equal(out, bithenge_node_as_blob(a),
		    bithenge_node_as_blob(b));
	}
	return EINVAL;
}

/** @}
 */
