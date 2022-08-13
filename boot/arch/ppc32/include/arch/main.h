/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_ppc32_MAIN_H_
#define BOOT_ppc32_MAIN_H_

#include <stddef.h>
#include <balloc.h>
#include <genarch/ofw_tree.h>
#include <arch/types.h>

typedef struct {
	memmap_t memmap;
	taskmap_t taskmap;
	ballocs_t ballocs;
	ofw_tree_node_t *ofw_root;
} bootinfo_t;

extern void bootstrap(void);

#endif
