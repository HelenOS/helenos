/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_EBUS_H_
#define KERN_EBUS_H_

#include <genarch/ofw/ofw_tree.h>
#include <typedefs.h>
#include <ddi/irq.h>
#include <typedefs.h>

typedef struct {
	uint32_t space;
	uint32_t addr;
	uint32_t size;
} __attribute__((packed)) ofw_ebus_reg_t;

typedef struct {
	uint32_t child_space;
	uint32_t child_base;
	uint32_t parent_space;

	/* Group phys.mid and phys.lo together */
	uint64_t parent_base;
	uint32_t size;
} __attribute__((packed)) ofw_ebus_range_t;

typedef struct {
	uint32_t space;
	uint32_t addr;
	uint32_t intr;
	uint32_t controller_handle;
	uint32_t controller_ino;
} __attribute__((packed)) ofw_ebus_intr_map_t;

typedef struct {
	uint32_t space_mask;
	uint32_t addr_mask;
	uint32_t intr_mask;
} __attribute__((packed)) ofw_ebus_intr_mask_t;

extern bool ofw_ebus_apply_ranges(ofw_tree_node_t *, ofw_ebus_reg_t *,
    uintptr_t *);
extern bool ofw_ebus_map_interrupt(ofw_tree_node_t *, ofw_ebus_reg_t *,
    uint32_t, int *, cir_t *, void **);

#endif
