/*
 * Copyright (c) 2006 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
