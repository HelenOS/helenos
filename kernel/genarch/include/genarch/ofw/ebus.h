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
} __attribute__ ((packed)) ofw_ebus_reg_t;

typedef struct {
	uint32_t child_space;
	uint32_t child_base;
	uint32_t parent_space;

	/* Group phys.mid and phys.lo together */
	uint64_t parent_base;
	uint32_t size;
} __attribute__ ((packed)) ofw_ebus_range_t;

typedef struct {
	uint32_t space;
	uint32_t addr;
	uint32_t intr;
	uint32_t controller_handle;
	uint32_t controller_ino;
} __attribute__ ((packed)) ofw_ebus_intr_map_t;

typedef struct {
	uint32_t space_mask;
	uint32_t addr_mask;
	uint32_t intr_mask;
} __attribute__ ((packed)) ofw_ebus_intr_mask_t;

extern bool ofw_ebus_apply_ranges(ofw_tree_node_t *, ofw_ebus_reg_t *,
    uintptr_t *);
extern bool ofw_ebus_map_interrupt(ofw_tree_node_t *, ofw_ebus_reg_t *,
    uint32_t, int *, cir_t *, void **);

#endif
