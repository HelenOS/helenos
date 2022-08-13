/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef KERN_PCI_H_
#define KERN_PCI_H_

#include <genarch/ofw/ofw_tree.h>
#include <typedefs.h>
#include <ddi/irq.h>
#include <typedefs.h>

typedef struct {
	/* Needs to be masked to obtain pure space id */
	uint32_t space;

	/* Group phys.mid and phys.lo together */
	uint64_t addr;
	uint64_t size;
} __attribute__((packed)) ofw_pci_reg_t;

typedef struct {
	uint32_t space;

	/* Group phys.mid and phys.lo together */
	uint64_t child_base;
	uint64_t parent_base;
	uint64_t size;
} __attribute__((packed)) ofw_pci_range_t;

extern bool ofw_pci_apply_ranges(ofw_tree_node_t *, ofw_pci_reg_t *,
    uintptr_t *);

extern bool ofw_pci_reg_absolutize(ofw_tree_node_t *, ofw_pci_reg_t *,
    ofw_pci_reg_t *);

extern bool ofw_pci_map_interrupt(ofw_tree_node_t *, ofw_pci_reg_t *,
    int, int *, cir_t *, void **);

#endif
