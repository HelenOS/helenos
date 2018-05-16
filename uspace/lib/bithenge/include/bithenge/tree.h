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
#include <stdbool.h>
#include "os.h"

/** Indicates the type of a tree node. */
typedef enum {
	/** An internal node with labelled edges to other nodes. */
	BITHENGE_NODE_INTERNAL = 1,
	/** A leaf node holding a boolean value. */
	BITHENGE_NODE_BOOLEAN,
	/** A leaf node holding an integer. */
	BITHENGE_NODE_INTEGER,
	/** A leaf node holding a string. */
	BITHENGE_NODE_STRING,
	/** A leaf node holding a binary blob. */
	BITHENGE_NODE_BLOB,
} bithenge_node_type_t;

/** A tree node. It can have any of the types in @a bithenge_node_type_t. */
typedef struct bithenge_node_t {
	/** @privatesection */
	bithenge_node_type_t type;
	unsigned int refs;
	union {
		/** @privatesection */
		const struct bithenge_internal_node_ops_t *internal_ops;
		bool boolean_value;
		bithenge_int_t integer_value;
		struct {
			/** @privatesection */
			const char *ptr;
			bool needs_free;
		} string_value;
		const struct bithenge_random_access_blob_ops_t *blob_ops;
	};
} bithenge_node_t;

/** A callback function used to iterate over a node's children. It takes
 * ownership of a reference to both the key and the value.
 * @memberof bithenge_node_t
 * @param key The key.
 * @param value The value.
 * @param data Data provided to @a bithenge_node_t::bithenge_node_for_each.
 * @return EOK on success or an error code from errno.h.
 */
typedef errno_t (*bithenge_for_each_func_t)(bithenge_node_t *key, bithenge_node_t *value, void *data);

/** Operations providing access to an internal node. */
typedef struct bithenge_internal_node_ops_t {
	/** @copydoc bithenge_node_t::bithenge_node_for_each */
	errno_t (*for_each)(bithenge_node_t *self, bithenge_for_each_func_t func, void *data);
	/** @copydoc bithenge_node_t::bithenge_node_get */
	errno_t (*get)(bithenge_node_t *self, bithenge_node_t *key,
	    bithenge_node_t **out);
	/** Destroys the internal node.
	 * @param self The node to destroy.
	 */
	void (*destroy)(bithenge_node_t *self);
} bithenge_internal_node_ops_t;

/** Find the type of a node.
 * @memberof bithenge_node_t
 * @param node The node.
 * @return The type of the node.
 */
static inline bithenge_node_type_t bithenge_node_type(const bithenge_node_t *node)
{
	return node->type;
}

/** Increment a node's reference count.
 * @memberof bithenge_node_t
 * @param node The node to reference.
 */
static inline void bithenge_node_inc_ref(bithenge_node_t *node)
{
	assert(node);
	node->refs++;
}

/** @memberof bithenge_node_t */
void bithenge_node_dec_ref(bithenge_node_t *node);

/** Iterate over a node's children.
 * @memberof bithenge_node_t
 * @param self The internal node to iterate over.
 * @param func The callback function.
 * @param data Data to provide to the callback function.
 * @return EOK on success or an error code from errno.h.
 */
static inline errno_t bithenge_node_for_each(bithenge_node_t *self,
    bithenge_for_each_func_t func, void *data)
{
	assert(self->type == BITHENGE_NODE_INTERNAL);
	return self->internal_ops->for_each(self, func, data);
}

/** @memberof bithenge_node_t */
errno_t bithenge_node_get(bithenge_node_t *, bithenge_node_t *,
    bithenge_node_t **);

/** Get the value of a boolean node.
 * @memberof bithenge_node_t
 * @param self The boolean node.
 * @return The node's value.
 */
static inline bool bithenge_boolean_node_value(bithenge_node_t *self)
{
	assert(self->type == BITHENGE_NODE_BOOLEAN);
	return self->boolean_value;
}

/** Get the value of an integer node.
 * @memberof bithenge_node_t
 * @param self The integer node.
 * @return The node's value.
 */
static inline bithenge_int_t bithenge_integer_node_value(bithenge_node_t *self)
{
	assert(self->type == BITHENGE_NODE_INTEGER);
	return self->integer_value;
}

/** Get the value of an string node.
 * @memberof bithenge_node_t
 * @param self The string node.
 * @return The node's value.
 */
static inline const char *bithenge_string_node_value(bithenge_node_t *self)
{
	assert(self->type == BITHENGE_NODE_STRING);
	return self->string_value.ptr;
}

/** @memberof bithenge_node_t */
errno_t bithenge_init_internal_node(bithenge_node_t *,
    const bithenge_internal_node_ops_t *);
/** @memberof bithenge_node_t */
errno_t bithenge_new_empty_internal_node(bithenge_node_t **);
/** @memberof bithenge_node_t */
errno_t bithenge_new_simple_internal_node(bithenge_node_t **, bithenge_node_t **,
    bithenge_int_t, bool needs_free);
/** @memberof bithenge_node_t */
errno_t bithenge_new_boolean_node(bithenge_node_t **, bool);
/** @memberof bithenge_node_t */
errno_t bithenge_new_integer_node(bithenge_node_t **, bithenge_int_t);
/** @memberof bithenge_node_t */
errno_t bithenge_new_string_node(bithenge_node_t **, const char *, bool);
/** @memberof bithenge_node_t */
errno_t bithenge_node_equal(bool *, bithenge_node_t *, bithenge_node_t *);

#endif

/** @}
 */
