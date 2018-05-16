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
 * Transforms.
 */

#ifndef BITHENGE_TRANSFORM_H_
#define BITHENGE_TRANSFORM_H_

#include "blob.h"
#include "tree.h"

/** A transform that creates a new tree from an old tree. */
typedef struct {
	/** @privatesection */
	const struct bithenge_transform_ops *ops;
	unsigned int refs;
	int num_params;
} bithenge_transform_t;

/** Context and parameters used when applying transforms. */
typedef struct bithenge_scope {
	/** @privatesection */
	unsigned int refs;
	struct bithenge_scope *outer;
	char *error;
	bool barrier;
	int num_params;
	bithenge_node_t **params;
	bithenge_node_t *current_node;
	bithenge_node_t *in_node;
} bithenge_scope_t;

/** Increment a scope's reference count.
 * @memberof bithenge_scope_t
 * @param self The scope to reference.
 */
static inline void bithenge_scope_inc_ref(bithenge_scope_t *self)
{
	assert(self);
	self->refs++;
}

/** Operations that may be provided by a transform. All transforms must provide
 * apply and/or prefix_apply. To be used in struct transforms and repeat
 * transforms, transforms must provide prefix_length and/or prefix_apply.
 */
typedef struct bithenge_transform_ops {
	/** @copydoc bithenge_transform_t::bithenge_transform_apply */
	errno_t (*apply)(bithenge_transform_t *self, bithenge_scope_t *scope,
	    bithenge_node_t *in, bithenge_node_t **out);
	/** @copydoc bithenge_transform_t::bithenge_transform_prefix_length */
	errno_t (*prefix_length)(bithenge_transform_t *self,
	    bithenge_scope_t *scope, bithenge_blob_t *blob, aoff64_t *out);
	/** @copydoc bithenge_transform_t::bithenge_transform_prefix_apply */
	errno_t (*prefix_apply)(bithenge_transform_t *self,
	    bithenge_scope_t *scope, bithenge_blob_t *blob,
	    bithenge_node_t **out_node, aoff64_t *out_size);
	/** Destroy the transform.
	 * @param self The transform.
	 */
	void (*destroy)(bithenge_transform_t *self);
} bithenge_transform_ops_t;

/** Get the number of parameters required by a transform. This number is used
 * by the parser and param-wrapper. Takes ownership of nothing.
 * @param self The transform.
 * @return The number of parameters required.
 */
static inline int bithenge_transform_num_params(bithenge_transform_t *self)
{
	assert(self);
	return self->num_params;
}

/** Increment a transform's reference count.
 * @param self The transform to reference.
 */
static inline void bithenge_transform_inc_ref(bithenge_transform_t *self)
{
	assert(self);
	self->refs++;
}

/** Decrement a transform's reference count and free it if appropriate.
 * @param self The transform to dereference, or NULL.
 */
static inline void bithenge_transform_dec_ref(bithenge_transform_t *self)
{
	if (!self)
		return;
	assert(self->ops);
	assert(self->refs > 0);
	if (--self->refs == 0)
		self->ops->destroy(self);
}

/** A transform with a name. */
typedef struct {
	/** The transform's name. */
	const char *name;
	/** The transform. */
	bithenge_transform_t *transform;
} bithenge_named_transform_t;

/** Transform that decodes an 8-bit unsigned integer */
extern bithenge_transform_t bithenge_uint8_transform;
/** Transform that decodes a 16-bit little-endian unsigned integer */
extern bithenge_transform_t bithenge_uint16le_transform;
/** Transform that decodes a 16-bit big-endian unsigned integer */
extern bithenge_transform_t bithenge_uint16be_transform;
/** Transform that decodes a 32-bit little-endian unsigned integer */
extern bithenge_transform_t bithenge_uint32le_transform;
/** Transform that decodes a 32-bit big-endian unsigned integer */
extern bithenge_transform_t bithenge_uint32be_transform;
/** Transform that decodes a 64-bit little-endian unsigned integer */
extern bithenge_transform_t bithenge_uint64le_transform;
/** Transform that decodes a 64-bit big-endian unsigned integer */
extern bithenge_transform_t bithenge_uint64be_transform;

/** @cond */
extern bithenge_transform_t bithenge_ascii_transform;
extern bithenge_transform_t bithenge_bit_transform;
extern bithenge_transform_t bithenge_bits_be_transform;
extern bithenge_transform_t bithenge_bits_le_transform;
extern bithenge_transform_t bithenge_invalid_transform;
extern bithenge_transform_t bithenge_known_length_transform;
extern bithenge_transform_t bithenge_nonzero_boolean_transform;
extern bithenge_transform_t bithenge_uint_le_transform;
extern bithenge_transform_t bithenge_uint_be_transform;
extern bithenge_transform_t bithenge_zero_terminated_transform;
extern bithenge_named_transform_t *bithenge_primitive_transforms;
/** @endcond */

/** @memberof bithenge_transform_t */
errno_t bithenge_init_transform(bithenge_transform_t *,
    const bithenge_transform_ops_t *, int);
/** @memberof bithenge_transform_t */
errno_t bithenge_transform_apply(bithenge_transform_t *, bithenge_scope_t *,
    bithenge_node_t *, bithenge_node_t **);
/** @memberof bithenge_transform_t */
errno_t bithenge_transform_prefix_length(bithenge_transform_t *,
    bithenge_scope_t *, bithenge_blob_t *, aoff64_t *);
/** @memberof bithenge_transform_t */
errno_t bithenge_transform_prefix_apply(bithenge_transform_t *, bithenge_scope_t *,
    bithenge_blob_t *, bithenge_node_t **, aoff64_t *);
errno_t bithenge_new_barrier_transform(bithenge_transform_t **, int);
errno_t bithenge_barrier_transform_set_subtransform(bithenge_transform_t *,
    bithenge_transform_t *);

/** @memberof bithenge_scope_t */
errno_t bithenge_scope_new(bithenge_scope_t **, bithenge_scope_t *);
/** @memberof bithenge_scope_t */
void bithenge_scope_dec_ref(bithenge_scope_t *);
/** @memberof bithenge_scope_t */
bithenge_scope_t *bithenge_scope_outer(bithenge_scope_t *);
/** @memberof bithenge_scope_t */
const char *bithenge_scope_get_error(bithenge_scope_t *);
/** @memberof bithenge_scope_t */
errno_t bithenge_scope_error(bithenge_scope_t *, const char *, ...);
/** @memberof bithenge_scope_t */
bithenge_node_t *bithenge_scope_get_current_node(bithenge_scope_t *);
/** @memberof bithenge_scope_t */
void bithenge_scope_set_current_node(bithenge_scope_t *, bithenge_node_t *);
/** @memberof bithenge_scope_t */
bithenge_node_t *bithenge_scope_in_node(bithenge_scope_t *);
/** @memberof bithenge_scope_t */
void bithenge_scope_set_in_node(bithenge_scope_t *, bithenge_node_t *);
/** @memberof bithenge_scope_t */
void bithenge_scope_set_barrier(bithenge_scope_t *);
/** @memberof bithenge_scope_t */
bool bithenge_scope_is_barrier(bithenge_scope_t *);
/** @memberof bithenge_scope_t */
errno_t bithenge_scope_alloc_params(bithenge_scope_t *, int);
/** @memberof bithenge_scope_t */
errno_t bithenge_scope_set_param(bithenge_scope_t *, int, bithenge_node_t *);
/** @memberof bithenge_scope_t */
errno_t bithenge_scope_get_param(bithenge_scope_t *, int, bithenge_node_t **);

#endif

/** @}
 */
