/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_OFW_TREE_H_
#define BOOT_OFW_TREE_H_

#include <stddef.h>
#include <genarch/ofw.h>

/** Memory representation of OpenFirmware device tree node property. */
typedef struct {
	char name[OFW_TREE_PROPERTY_MAX_NAMELEN];
	size_t size;
	void *value;
} ofw_tree_property_t;

/** Memory representation of OpenFirmware device tree node. */
typedef struct ofw_tree_node {
	struct ofw_tree_node *parent;
	struct ofw_tree_node *peer;
	struct ofw_tree_node *child;

	phandle node_handle;            /**< Old OpenFirmware node handle. */

	char *da_name;                  /**< Disambigued name. */

	size_t properties;              /**< Number of properties. */
	ofw_tree_property_t *property;

	void *device;                   /**< Member used solely by the kernel. */
} ofw_tree_node_t;

extern ofw_tree_node_t *ofw_tree_build(void);

#endif
