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
