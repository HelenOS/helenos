/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief	UPA 'reg' and 'ranges' properties handling.
 *
 */

#include <genarch/ofw/ofw_tree.h>
#include <genarch/ofw/upa.h>
#include <halt.h>
#include <panic.h>
#include <macros.h>
#include <debug.h>

bool ofw_upa_apply_ranges(ofw_tree_node_t *node, ofw_upa_reg_t *reg, uintptr_t *pa)
{
	*pa = reg->addr;
	return true;
}

/** @}
 */
