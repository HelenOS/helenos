/*
 * SPDX-FileCopyrightText: 2005 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_CPU_NODE_H_
#define KERN_sparc64_CPU_NODE_H_

#include <genarch/ofw/ofw_tree.h>

/** Finds the parent node of all the CPU nodes (nodes named "cpu" or "cmp").
 *
 *  Depending on the machine type (and possibly the OFW version), CPUs can be
 *  at "/" or at "/ssm@0,0".
 */
static inline ofw_tree_node_t *cpus_parent(void)
{
	ofw_tree_node_t *parent;
	parent = ofw_tree_find_child(ofw_tree_lookup("/"), "ssm@0,0");
	if (parent == NULL)
		parent = ofw_tree_lookup("/");
	return parent;
}

#endif

/** @}
 */
