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

#ifndef KERN_OFW_TREE_H_
#define KERN_OFW_TREE_H_

#include <stdbool.h>
#include <typedefs.h>

#define OFW_TREE_PROPERTY_MAX_NAMELEN  32

typedef uint32_t phandle;

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
	
	/**
	 * Pointer to a structure representing respective device.
	 * Its semantics is device dependent.
	 */
	void *device;
} ofw_tree_node_t;

/* Walker for visiting OpenFirmware device tree nodes. */
typedef bool (*ofw_tree_walker_t)(ofw_tree_node_t *, void *);

extern void ofw_tree_init(ofw_tree_node_t *);
extern void ofw_sysinfo_map(void);

extern const char *ofw_tree_node_name(const ofw_tree_node_t *);
extern ofw_tree_node_t *ofw_tree_lookup(const char *);
extern ofw_tree_property_t *ofw_tree_getprop(const ofw_tree_node_t *,
    const char *);
extern void ofw_tree_walk_by_device_type(const char *, ofw_tree_walker_t,
    void *);

extern ofw_tree_node_t *ofw_tree_find_child(ofw_tree_node_t *, const char *);
extern ofw_tree_node_t *ofw_tree_find_child_by_device_type(ofw_tree_node_t *,
    const char *);

extern ofw_tree_node_t *ofw_tree_find_peer_by_device_type(ofw_tree_node_t *,
    const char *);
extern ofw_tree_node_t *ofw_tree_find_peer_by_name(ofw_tree_node_t *,
    const char *);
extern ofw_tree_node_t *ofw_tree_find_node_by_handle(ofw_tree_node_t *,
    phandle);

#endif
