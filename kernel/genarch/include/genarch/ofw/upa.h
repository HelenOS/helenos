/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_UPA_H_
#define KERN_UPA_H_

#include <genarch/ofw/ofw_tree.h>
#include <typedefs.h>
#include <ddi/irq.h>
#include <typedefs.h>

typedef struct {
	uint64_t addr;
	uint64_t size;
} __attribute__((packed)) ofw_upa_reg_t;

extern bool ofw_upa_apply_ranges(ofw_tree_node_t *, ofw_upa_reg_t *,
    uintptr_t *);

#endif
