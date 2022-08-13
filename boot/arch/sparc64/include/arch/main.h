/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_sparc64_MAIN_H_
#define BOOT_sparc64_MAIN_H_

#include <stdint.h>
#include <balloc.h>
#include <genarch/ofw_tree.h>
#include <arch/types.h>

typedef struct {
	uintptr_t physmem_start;
	taskmap_t taskmap;
	memmap_t memmap;
	ballocs_t ballocs;
	ofw_tree_node_t *ofw_root;
} bootinfo_t;

extern void bootstrap(void);

#endif
