/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_PCI_H_
#define KERN_sparc64_PCI_H_

#include <stdint.h>
#include <genarch/ofw/ofw_tree.h>
#include <arch/arch.h>
#include <arch/asm.h>

typedef enum pci_model pci_model_t;
typedef struct pci pci_t;
typedef struct pci_operations pci_operations_t;

enum pci_model {
	PCI_UNKNOWN,
	PCI_SABRE,
	PCI_PSYCHO
};

struct pci_operations {
	void (*enable_interrupt)(pci_t *, int);
	void (*clear_interrupt)(pci_t *, int);
};

struct pci {
	pci_model_t model;
	pci_operations_t *op;
	volatile uint64_t *reg;		/**< Registers including interrupt registers. */
};

extern pci_t *pci_init(ofw_tree_node_t *);
extern void pci_enable_interrupt(pci_t *, int);
extern void pci_clear_interrupt(void *, int);

#endif

/** @}
 */
