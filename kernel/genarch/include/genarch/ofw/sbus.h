/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_SBUS_H_
#define KERN_SBUS_H_

#include <genarch/ofw/ofw_tree.h>
#include <typedefs.h>
#include <ddi/irq.h>
#include <typedefs.h>

typedef struct {
	uint64_t addr;
	uint32_t size;
} __attribute__((packed)) ofw_sbus_reg_t;

typedef struct {
	uint64_t child_base;
	uint64_t parent_base;
	uint32_t size;
} __attribute__((packed)) ofw_sbus_range_t;

extern bool ofw_sbus_apply_ranges(ofw_tree_node_t *, ofw_sbus_reg_t *,
    uintptr_t *);

#endif
