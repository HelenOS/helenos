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

/** @addtogroup sparc64	
 * @{
 */
/** @file
 */

#include <arch/drivers/kbd.h>
#include <genarch/ofw/ofw_tree.h>
#ifdef CONFIG_Z8530
#include <genarch/kbd/z8530.h>
#endif
#ifdef CONFIG_NS16550
#include <genarch/kbd/ns16550.h>
#endif
#include <ddi/device.h>
#include <ddi/irq.h>
#include <arch/mm/page.h>
#include <arch/types.h>
#include <typedefs.h>
#include <align.h>
#include <func.h>
#include <print.h>

kbd_type_t kbd_type = KBD_UNKNOWN;

/** Initialize keyboard.
 *
 * Traverse OpenFirmware device tree in order to find necessary
 * info about the keyboard device.
 *
 * @param node Keyboard device node.
 */
void kbd_init(ofw_tree_node_t *node)
{
	size_t offset;
	uintptr_t aligned_addr;
	ofw_tree_property_t *prop;
	const char *name;
	
	name = ofw_tree_node_name(node);
	
	/*
	 * Determine keyboard serial controller type.
	 */
	if (strcmp(name, "zs") == 0)
		kbd_type = KBD_Z8530;
	else if (strcmp(name, "su") == 0)
		kbd_type = KBD_NS16550;
	
	if (kbd_type == KBD_UNKNOWN) {
		printf("Unknown keyboard device.\n");
		return;
	}
	
	/*
	 * Read 'interrupts' property.
	 */
	uint32_t interrupts;
	prop = ofw_tree_getprop(node, "interrupts");
	if (!prop || !prop->value)
		panic("Can't find \"interrupts\" property.\n");
	interrupts = *((uint32_t *) prop->value);

	/*
	 * Read 'reg' property.
	 */
	prop = ofw_tree_getprop(node, "reg");
	if (!prop || !prop->value)
		panic("Can't find \"reg\" property.\n");
	
	uintptr_t pa;
	size_t size;
	inr_t inr;
	devno_t devno = device_assign_devno();
	
	switch (kbd_type) {
	case KBD_Z8530:
		size = ((ofw_fhc_reg_t *) prop->value)->size;
		if (!ofw_fhc_apply_ranges(node->parent, ((ofw_fhc_reg_t *) prop->value) , &pa)) {
			printf("Failed to determine keyboard address.\n");
			return;
		}
		if (!ofw_fhc_map_interrupt(node->parent, ((ofw_fhc_reg_t *) prop->value), interrupts, &inr)) {
			printf("Failed to determine keyboard interrupt.\n");
			return;
		}
		break;
		
	case KBD_NS16550:
		size = ((ofw_ebus_reg_t *) prop->value)->size;
		if (!ofw_ebus_apply_ranges(node->parent, ((ofw_ebus_reg_t *) prop->value) , &pa)) {
			printf("Failed to determine keyboard address.\n");
			return;
		}
		if (!ofw_ebus_map_interrupt(node->parent, ((ofw_ebus_reg_t *) prop->value), interrupts, &inr)) {
			printf("Failed to determine keyboard interrupt.\n");
			return;
		};
		break;

	default:
		panic("Unexpected type.\n");
	}
	
	/*
	 * We need to pass aligned address to hw_map().
	 * However, the physical keyboard address can
	 * be pretty much unaligned, depending on the
	 * underlying controller.
	 */
	aligned_addr = ALIGN_DOWN(pa, PAGE_SIZE);
	offset = pa - aligned_addr;
	uintptr_t vaddr = hw_map(aligned_addr, offset + size) + offset;

	switch (kbd_type) {
#ifdef CONFIG_Z8530
	case KBD_Z8530:
		z8530_init(devno, inr, vaddr);
		break;
#endif
#ifdef CONFIG_NS16550
	case KBD_NS16550:
		ns16550_init(devno, inr, vaddr);
		break;
#endif
	default:
		printf("Kernel is not compiled with the necessary keyboard driver this machine requires.\n");
	}
}

/** @}
 */
