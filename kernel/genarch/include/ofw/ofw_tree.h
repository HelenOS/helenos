/*
 * Copyright (C) 2006 Jakub Jermar
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

#ifndef KERN_OFW_TREE_H_
#define KERN_OFW_TREE_H_

#include <arch/types.h>
#include <typedefs.h>

#define OFW_TREE_PROPERTY_MAX_NAMELEN	32

typedef struct ofw_tree_node ofw_tree_node_t;
typedef struct ofw_tree_property ofw_tree_property_t;

/** Memory representation of OpenFirmware device tree node. */
struct ofw_tree_node {
	ofw_tree_node_t *parent;
	ofw_tree_node_t *peer;
	ofw_tree_node_t *child;

	char *da_name;					/**< Disambigued name. */

	unsigned properties;				/**< Number of properties. */
	ofw_tree_property_t *property;
};

/** Memory representation of OpenFirmware device tree node property. */
struct ofw_tree_property {
	char name[OFW_TREE_PROPERTY_MAX_NAMELEN];
	size_t size;
	void *value;
};

extern void ofw_tree_init(ofw_tree_node_t *root);
extern void ofw_tree_print(void);
extern const char *ofw_tree_node_name(const ofw_tree_node_t *node);
extern ofw_tree_node_t *ofw_tree_lookup(const char *path);

#endif
