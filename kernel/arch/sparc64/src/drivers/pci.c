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
/**
 * @file
 * @brief	PCI driver.
 */

#include <arch/drivers/pci.h>
#include <genarch/ofw/ofw_tree.h>
#include <genarch/ofw/upa.h>
#include <arch/trap/interrupt.h>
#include <mm/km.h>
#include <mm/slab.h>
#include <typedefs.h>
#include <assert.h>
#include <log.h>
#include <str.h>
#include <arch/asm.h>
#include <sysinfo/sysinfo.h>

#define SABRE_INTERNAL_REG	0
#define PSYCHO_INTERNAL_REG	2

#define OBIO_IMR_BASE	0x200
#define OBIO_IMR(ino)	(OBIO_IMR_BASE + ((ino) & INO_MASK))

#define OBIO_CIR_BASE	0x300
#define OBIO_CIR(ino)	(OBIO_CIR_BASE + ((ino) & INO_MASK))

static void obio_enable_interrupt(pci_t *, int);
static void obio_clear_interrupt(pci_t *, int);

static pci_t *pci_sabre_init(ofw_tree_node_t *);
static pci_t *pci_psycho_init(ofw_tree_node_t *);

/** PCI operations for Sabre model. */
static pci_operations_t pci_sabre_ops = {
	.enable_interrupt = obio_enable_interrupt,
	.clear_interrupt = obio_clear_interrupt
};
/** PCI operations for Psycho model. */
static pci_operations_t pci_psycho_ops = {
	.enable_interrupt = obio_enable_interrupt,
	.clear_interrupt = obio_clear_interrupt
};

/** Initialize PCI controller (model Sabre).
 *
 * @param node		OpenFirmware device tree node of the Sabre.
 *
 * @return		Address of the initialized PCI structure.
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
	size_t regs = prop->size / sizeof(ofw_upa_reg_t);

	if (regs < SABRE_INTERNAL_REG + 1)
		return NULL;

	uintptr_t paddr;
	if (!ofw_upa_apply_ranges(node->parent, &reg[SABRE_INTERNAL_REG],
	    &paddr))
		return NULL;

	pci = (pci_t *) malloc(sizeof(pci_t), FRAME_ATOMIC);
	if (!pci)
		return NULL;

	pci->model = PCI_SABRE;
	pci->op = &pci_sabre_ops;
	pci->reg = (uint64_t *) km_map(paddr, reg[SABRE_INTERNAL_REG].size,
	    PAGE_WRITE | PAGE_NOT_CACHEABLE);

	return pci;
}


/** Initialize the Psycho PCI controller.
 *
 * @param node		OpenFirmware device tree node of the Psycho.
 *
 * @return		Address of the initialized PCI structure.
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
	size_t regs = prop->size / sizeof(ofw_upa_reg_t);

	if (regs < PSYCHO_INTERNAL_REG + 1)
		return NULL;

	uintptr_t paddr;
	if (!ofw_upa_apply_ranges(node->parent, &reg[PSYCHO_INTERNAL_REG],
	    &paddr))
		return NULL;

	pci = (pci_t *) malloc(sizeof(pci_t), FRAME_ATOMIC);
	if (!pci)
		return NULL;

	pci->model = PCI_PSYCHO;
	pci->op = &pci_psycho_ops;
	pci->reg = (uint64_t *) km_map(paddr, reg[PSYCHO_INTERNAL_REG].size,
	    PAGE_WRITE | PAGE_NOT_CACHEABLE);

	return pci;
}

void obio_enable_interrupt(pci_t *pci, int inr)
{
	pci->reg[OBIO_IMR(inr & INO_MASK)] |= IMAP_V_MASK;
}

void obio_clear_interrupt(pci_t *pci, int inr)
{
	pci->reg[OBIO_CIR(inr & INO_MASK)] = 0;	/* set IDLE */
}

/** Initialize PCI controller. */
pci_t *pci_init(ofw_tree_node_t *node)
{
	ofw_tree_property_t *prop;

	/*
	 * First, verify this is a PCI node.
	 */
	assert(str_cmp(ofw_tree_node_name(node), "pci") == 0);

	/*
	 * Determine PCI controller model.
	 */
	prop = ofw_tree_getprop(node, "model");
	if (!prop || !prop->value)
		return NULL;

	if (str_cmp(prop->value, "SUNW,sabre") == 0) {
		/*
		 * PCI controller Sabre.
		 * This model is found on UltraSPARC IIi based machines.
		 */
		return pci_sabre_init(node);
	} else if (str_cmp(prop->value, "SUNW,psycho") == 0) {
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
		log(LF_ARCH, LVL_WARN, "Unsupported PCI controller model (%s).",
		    (char *) prop->value);
	}

	return NULL;
}

void pci_enable_interrupt(pci_t *pci, int inr)
{
	assert(pci->op && pci->op->enable_interrupt);
	pci->op->enable_interrupt(pci, inr);
}

void pci_clear_interrupt(void *pcip, int inr)
{
	pci_t *pci = (pci_t *)pcip;

	assert(pci->op && pci->op->clear_interrupt);
	pci->op->clear_interrupt(pci, inr);
}

/** @}
 */
