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
 * @brief	FireHose Controller (FHC) driver.
 *
 * Note that this driver is a result of reverse engineering
 * rather than implementation of a specification. This
 * is due to the fact that the FHC documentation is not
 * publicly available.
 */

#include <arch/drivers/fhc.h>
#include <arch/mm/page.h>
#include <mm/slab.h>
#include <arch/types.h>
#include <typedefs.h>
#include <genarch/ofw/ofw_tree.h>
#include <genarch/kbd/z8530.h>

fhc_t *central_fhc = NULL;

/**
 * I suspect this must be hardcoded in the FHC.
 * If it is not, than we can read all IMAP registers
 * and get the complete mapping.
 */
#define FHC_UART_INO	0x39	

#define FHC_UART_IMAP	0x0
#define FHC_UART_ICLR	0x4

#define UART_IMAP_REG	4

fhc_t *fhc_init(ofw_tree_node_t *node)
{
	fhc_t *fhc;
	ofw_tree_property_t *prop;

	prop = ofw_tree_getprop(node, "reg");
	
	if (!prop || !prop->value)
		return NULL;
		
	count_t regs = prop->size / sizeof(ofw_central_reg_t);
	if (regs + 1 < UART_IMAP_REG)
		return NULL;

	ofw_central_reg_t *reg = &((ofw_central_reg_t *) prop->value)[UART_IMAP_REG];

	uintptr_t paddr;
	if (!ofw_central_apply_ranges(node->parent, reg, &paddr))
		return NULL;

	fhc = (fhc_t *) malloc(sizeof(fhc_t), FRAME_ATOMIC);
	if (!fhc)
		return NULL;

	fhc->uart_imap = (uint32_t *) hw_map(paddr, reg->size);
	
	return fhc;
}

void fhc_enable_interrupt(fhc_t *fhc, int ino)
{
	switch (ino) {
	case FHC_UART_INO:
		fhc->uart_imap[FHC_UART_ICLR] = 0x0;
		fhc->uart_imap[FHC_UART_IMAP] = 0x80000000;
		break;
	default:
		panic("Unexpected INO (%d)\n", ino);
		break;
	}
}

void fhc_clear_interrupt(fhc_t *fhc, int ino)
{
	ASSERT(fhc->uart_imap);

	switch (ino) {
	case FHC_UART_INO:
		fhc->uart_imap[FHC_UART_ICLR] = 0;
		break;
	default:
		panic("Unexpected INO (%d)\n", ino);
		break;
	}
}

/** @}
 */
