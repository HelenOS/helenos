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

/** @addtogroup sparc64
 * @{
 */
/**
 * @file
 * @brief	PCI driver.
 */

#include <arch/drivers/pci.h>
#include <genarch/ofw/ofw_tree.h>
#include <arch/trap/interrupt.h>
#include <arch/mm/page.h>
#include <mm/slab.h>
#include <arch/types.h>
#include <typedefs.h>
#include <debug.h>
#include <print.h>
#include <func.h>
#include <arch/asm.h>

#define PCI_SABRE_REGS_REG	0

#define PCI_SABRE_IMAP_BASE	0x200
#define PCI_SABRE_ICLR_BASE	0x300

#define PCI_PSYCHO_REGS_REG	2	

#define PCI_PSYCHO_IMAP_BASE	0x200
#define PCI_PSYCHO_ICLR_BASE	0x300	

static pci_t *pci_sabre_init(ofw_tree_node_t *node);
static void pci_sabre_enable_interrupt(pci_t *pci, int inr);
static void pci_sabre_clear_interrupt(pci_t *pci, int inr);

static pci_t *pci_psycho_init(ofw_tree_node_t *node);
static void pci_psycho_enable_interrupt(pci_t *pci, int inr);
static void pci_psycho_clear_interrupt(pci_t *pci, int inr);

/** PCI operations for Sabre model. */
static pci_operations_t pci_sabre_ops = {
	.enable_interrupt = pci_sabre_enable_interrupt,
	.clear_interrupt = pci_sabre_clear_interrupt
};
/** PCI operations for Psycho model. */
static pci_operations_t pci_psycho_ops = {
	.enable_interrupt = pci_psycho_enable_interrupt,
	.clear_interrupt = pci_psycho_clear_interrupt
};

/** Initialize PCI controller (model Sabre).
 *
 * @param node OpenFirmware device tree node of the Sabre.
 *
 * @return Address of the initialized PCI structure.
 */ 
pci_t *pci_sabre_init(ofw_tree_node_t *node)
{
	pci_t *pci;
	ofw_tree_property_t *prop;

	/*
	 * Get registers.
	 */
	prop = ofw_tree_getprop(node, "reg");
	if (!prop || !prop->value)
		return NULL;

	ofw_upa_reg_t *reg = prop->value;
	count_t regs = prop->size / sizeof(ofw_upa_reg_t);

	if (regs < PCI_SABRE_REGS_REG + 1)
		return NULL;

	uintptr_t paddr;
	if (!ofw_upa_apply_ranges(node->parent, &reg[PCI_SABRE_REGS_REG], &paddr))
		return NULL;

	pci = (pci_t *) malloc(sizeof(pci_t), FRAME_ATOMIC);
	if (!pci)
		return NULL;

	pci->model = PCI_SABRE;
	pci->op = &pci_sabre_ops;
	pci->reg = (uint64_t *) hw_map(paddr, reg[PCI_SABRE_REGS_REG].size);

	return pci;
}


/** Initialize the Psycho PCI controller.
 *
 * @param node OpenFirmware device tree node of the Psycho.
 *
 * @return Address of the initialized PCI structure.
 */ 
pci_t *pci_psycho_init(ofw_tree_node_t *node)
{
	pci_t *pci;
	ofw_tree_property_t *prop;

	/*
	 * Get registers.
	 */
	prop = ofw_tree_getprop(node, "reg");
	if (!prop || !prop->value)
		return NULL;

	ofw_upa_reg_t *reg = prop->value;
	count_t regs = prop->size / sizeof(ofw_upa_reg_t);

	if (regs < PCI_PSYCHO_REGS_REG + 1)
		return NULL;

	uintptr_t paddr;
	if (!ofw_upa_apply_ranges(node->parent, &reg[PCI_PSYCHO_REGS_REG], &paddr))
		return NULL;

	pci = (pci_t *) malloc(sizeof(pci_t), FRAME_ATOMIC);
	if (!pci)
		return NULL;

	pci->model = PCI_PSYCHO;
	pci->op = &pci_psycho_ops;
	pci->reg = (uint64_t *) hw_map(paddr, reg[PCI_PSYCHO_REGS_REG].size);

	return pci;
}

void pci_sabre_enable_interrupt(pci_t *pci, int inr)
{
	pci->reg[PCI_SABRE_IMAP_BASE + (inr & INO_MASK)] |= IMAP_V_MASK;
}

void pci_sabre_clear_interrupt(pci_t *pci, int inr)
{
	pci->reg[PCI_SABRE_ICLR_BASE + (inr & INO_MASK)] = 0;
}

void pci_psycho_enable_interrupt(pci_t *pci, int inr)
{
	pci->reg[PCI_PSYCHO_IMAP_BASE + (inr & INO_MASK)] |= IMAP_V_MASK;
}

void pci_psycho_clear_interrupt(pci_t *pci, int inr)
{
	pci->reg[PCI_PSYCHO_ICLR_BASE + (inr & INO_MASK)] = 0;
}

/** Initialize PCI controller. */
pci_t *pci_init(ofw_tree_node_t *node)
{
	ofw_tree_property_t *prop;

	/*
	 * First, verify this is a PCI node.
	 */
	ASSERT(strcmp(ofw_tree_node_name(node), "pci") == 0);

	/*
	 * Determine PCI controller model.
	 */
	prop = ofw_tree_getprop(node, "model");
	if (!prop || !prop->value)
		return NULL;
	
	if (strcmp(prop->value, "SUNW,sabre") == 0) {
		/*
		 * PCI controller Sabre.
		 * This model is found on UltraSPARC IIi based machines.
		 */
		return pci_sabre_init(node);
	} else if (strcmp(prop->value, "SUNW,psycho") == 0) {
		/*
		 * PCI controller Psycho.
		 * Used on UltraSPARC II based processors, for instance,
		 * on Ultra 60.
		 */
		return pci_psycho_init(node);
	} else {
		/*
		 * Unsupported model.
		 */
		printf("Unsupported PCI controller model (%s).\n", prop->value);
	}

	return NULL;
}

void pci_enable_interrupt(pci_t *pci, int inr)
{
	ASSERT(pci->model);
	ASSERT(pci->op && pci->op->enable_interrupt);
	pci->op->enable_interrupt(pci, inr);
}

void pci_clear_interrupt(pci_t *pci, int inr)
{
	ASSERT(pci->model);
	ASSERT(pci->op && pci->op->clear_interrupt);
	pci->op->clear_interrupt(pci, inr);
}

/** @}
 */
