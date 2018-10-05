/*
 * Copyright (c) 2009 Pavel Rimsky
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

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4v_MD_H_
#define KERN_sparc64_sun4v_MD_H_

#include <stdbool.h>

/**
 * Data type used to iterate through MD nodes. Internally represented as
 * an index to the first element of the node.
 */
typedef unsigned int md_node_t;

/** used to iterate over children of a given node */
typedef unsigned int md_child_iter_t;

md_node_t md_get_root(void);
md_node_t md_get_child(md_node_t node, char *name);
md_child_iter_t md_get_child_iterator(md_node_t node);
bool md_next_child(md_child_iter_t *it);
md_node_t md_get_child_node(md_child_iter_t it);
const char *md_get_node_name(md_node_t node);
bool md_get_integer_property(md_node_t node, const char *key,
    uint64_t *result);
bool md_get_string_property(md_node_t node, const char *key,
    const char **result);
bool md_next_node(md_node_t *node, const char *name);
void md_init(void);

#endif

/** @}
 */
