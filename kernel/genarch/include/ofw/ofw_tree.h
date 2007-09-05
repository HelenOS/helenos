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

#include <arch/types.h>

#define OFW_TREE_PROPERTY_MAX_NAMELEN	32

typedef struct ofw_tree_node ofw_tree_node_t;
typedef struct ofw_tree_property ofw_tree_property_t;

/** Memory representation of OpenFirmware device tree node. */
struct ofw_tree_node {
	ofw_tree_node_t *parent;
	ofw_tree_node_t *peer;
	ofw_tree_node_t *child;

	uint32_t node_handle;			/**< Old OpenFirmware node handle. */

	char *da_name;				/**< Disambigued name. */

	unsigned properties;			/**< Number of properties. */
	ofw_tree_property_t *property;
	
	/**
	 * Pointer to a structure representing respective device.
	 * Its semantics is device dependent.
	 */
	void *device;
};

/** Memory representation of OpenFirmware device tree node property. */
struct ofw_tree_property {
	char name[OFW_TREE_PROPERTY_MAX_NAMELEN];
	size_t size;
	void *value;
};

/*
 * Definition of 'reg' and 'ranges' properties for various buses.
 */
 
struct ofw_fhc_reg {
	uint64_t addr;
	uint32_t size;
} __attribute__ ((packed));
typedef struct ofw_fhc_reg ofw_fhc_reg_t;
			
struct ofw_fhc_range {
	uint64_t child_base;
	uint64_t parent_base;
	uint32_t size;
} __attribute__ ((packed));
typedef struct ofw_fhc_range ofw_fhc_range_t;

struct ofw_central_reg {
	uint64_t addr;
	uint32_t size;
} __attribute__ ((packed));
typedef struct ofw_central_reg ofw_central_reg_t;

struct ofw_central_range {
	uint64_t child_base;
	uint64_t parent_base;
	uint32_t size;
} __attribute__ ((packed));
typedef struct ofw_central_range ofw_central_range_t;

struct ofw_ebus_reg {
	uint32_t space;
	uint32_t addr;
	uint32_t size;
} __attribute__ ((packed));
typedef struct ofw_ebus_reg ofw_ebus_reg_t;

struct ofw_ebus_range {
	uint32_t child_space;
	uint32_t child_base;
	uint32_t parent_space;
	uint64_t parent_base;		/* group phys.mid and phys.lo together */
	uint32_t size;
} __attribute__ ((packed));
typedef struct ofw_ebus_range ofw_ebus_range_t;

struct ofw_ebus_intr_map {
	uint32_t space;
	uint32_t addr;
	uint32_t intr;
	uint32_t controller_handle;
	uint32_t controller_ino;
} __attribute__ ((packed));
typedef struct ofw_ebus_intr_map ofw_ebus_intr_map_t;

struct ofw_ebus_intr_mask {
	uint32_t space_mask;
	uint32_t addr_mask;
	uint32_t intr_mask;
} __attribute__ ((packed));
typedef struct ofw_ebus_intr_mask ofw_ebus_intr_mask_t;

struct ofw_pci_reg {
	uint32_t space;			/* needs to be masked to obtain pure space id */
	uint64_t addr;			/* group phys.mid and phys.lo together */
	uint64_t size;
} __attribute__ ((packed));
typedef struct ofw_pci_reg ofw_pci_reg_t;

struct ofw_pci_range {
	uint32_t space;
	uint64_t child_base;		/* group phys.mid and phys.lo together */
	uint64_t parent_base;
	uint64_t size;
} __attribute__ ((packed));
typedef struct ofw_pci_range ofw_pci_range_t;

struct ofw_sbus_reg {
	uint64_t addr;
	uint32_t size;
} __attribute__ ((packed));
typedef struct ofw_sbus_reg ofw_sbus_reg_t;

struct ofw_sbus_range {
	uint64_t child_base;
	uint64_t parent_base;
	uint32_t size;
} __attribute__ ((packed));
typedef struct ofw_sbus_range ofw_sbus_range_t;

struct ofw_upa_reg {
	uint64_t addr;
	uint64_t size;
} __attribute__ ((packed));
typedef struct ofw_upa_reg ofw_upa_reg_t;

extern void ofw_tree_init(ofw_tree_node_t *root);
extern void ofw_tree_print(void);
extern const char *ofw_tree_node_name(const ofw_tree_node_t *node);
extern ofw_tree_node_t *ofw_tree_lookup(const char *path);
extern ofw_tree_property_t *ofw_tree_getprop(const ofw_tree_node_t *node, const char *name);
extern ofw_tree_node_t *ofw_tree_find_child(ofw_tree_node_t *node, const char *name);
extern ofw_tree_node_t *ofw_tree_find_child_by_device_type(ofw_tree_node_t *node, const char *device_type);
extern ofw_tree_node_t *ofw_tree_find_peer_by_device_type(ofw_tree_node_t *node, const char *device_type);
extern ofw_tree_node_t *ofw_tree_find_node_by_handle(ofw_tree_node_t *root, uint32_t handle);

extern bool ofw_fhc_apply_ranges(ofw_tree_node_t *node, ofw_fhc_reg_t *reg, uintptr_t *pa);
extern bool ofw_central_apply_ranges(ofw_tree_node_t *node, ofw_central_reg_t *reg, uintptr_t *pa);
extern bool ofw_ebus_apply_ranges(ofw_tree_node_t *node, ofw_ebus_reg_t *reg, uintptr_t *pa);
extern bool ofw_pci_apply_ranges(ofw_tree_node_t *node, ofw_pci_reg_t *reg, uintptr_t *pa);
extern bool ofw_sbus_apply_ranges(ofw_tree_node_t *node, ofw_sbus_reg_t *reg, uintptr_t *pa);
extern bool ofw_upa_apply_ranges(ofw_tree_node_t *node, ofw_upa_reg_t *reg, uintptr_t *pa);

extern bool ofw_pci_reg_absolutize(ofw_tree_node_t *node, ofw_pci_reg_t *reg, ofw_pci_reg_t *out);

extern bool ofw_fhc_map_interrupt(ofw_tree_node_t *node, ofw_fhc_reg_t *reg, uint32_t interrupt, int *inr);
extern bool ofw_ebus_map_interrupt(ofw_tree_node_t *node, ofw_ebus_reg_t *reg, uint32_t interrupt, int *inr);
extern bool ofw_pci_map_interrupt(ofw_tree_node_t *node, ofw_pci_reg_t *reg, int ino, int *inr);

#endif
