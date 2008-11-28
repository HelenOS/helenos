/*
 * Copyright (c) 2008 Jakub Jermar
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
 * @brief	Driver for the UPA to PCI bridge (Psycho).
 */

#include <arch/drivers/psycho.h>
#include <arch/trap/interrupt.h>
#include <mm/page.h>
#include <mm/slab.h>
#include <arch/types.h>
#include <genarch/ofw/ofw_tree.h>
#include <byteorder.h>

#define PSYCHO_INTERNAL_REG	2

#define PSYCHO_OBIO_IMR_BASE	(0x1000 / sizeof(uint64_t))
#define PSYCHO_OBIO_IMR(ino)	(PSYCHO_OBIO_IMR_BASE + ((ino) & INO_MASK))

#define PSYCHO_OBIO_CIR_BASE	(0x1800 / sizeof(uint64_t))
#define PSYCHO_OBIO_CIR(ino)	(PSYCHO_OBIO_CIR_BASE + ((ino) & INO_MASK))

psycho_t *psycho_a = NULL;
psycho_t *psycho_b = NULL;

psycho_t *psycho_init(ofw_tree_node_t *node)
{
	psycho_t *psycho;
	ofw_tree_property_t *prop;

	prop = ofw_tree_getprop(node, "reg");
	
	if (!prop || !prop->value)
		return NULL;
	
	count_t regs = prop->size / sizeof(ofw_upa_reg_t);
	if (regs < PSYCHO_INTERNAL_REG + 1)
		return NULL;
		
	ofw_upa_reg_t *reg =
	    &((ofw_upa_reg_t *) prop->value)[PSYCHO_INTERNAL_REG];

	uintptr_t paddr;
	if (!ofw_upa_apply_ranges(node->parent, reg, &paddr))
		return NULL;

	psycho = (psycho_t *) malloc(sizeof(psycho_t), FRAME_ATOMIC);
	if (!psycho)
		return NULL;

	psycho->regs = (uint64_t *) hw_map(paddr, reg->size);
	
	return psycho;
}

void psycho_enable_interrupt(psycho_t *psycho, int inr)
{
	int ino = inr & ~IGN_MASK;
	uint64_t imr = psycho->regs[PSYCHO_OBIO_IMR(ino)];
	imr |= host2uint64_t_le(IMAP_V_MASK);
	psycho->regs[PSYCHO_OBIO_IMR(ino)] = imr;
}

void psycho_clear_interrupt(psycho_t *psycho, int inr)
{
	int ino = inr & ~IGN_MASK;
	uint64_t idle = host2uint64_t_le(0);
	psycho->regs[PSYCHO_OBIO_CIR(ino)] = idle;
}

/** @}
 */
