/*
 * SPDX-FileCopyrightText: 2009 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
