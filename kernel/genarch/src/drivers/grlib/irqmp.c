/*
 * Copyright (c) 2013 Jakub Klama
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

/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief Gaisler GRLIB interrupt controller.
 */

#include <genarch/drivers/grlib/irqmp.h>
#include <arch/asm.h>
#include <mm/km.h>

void grlib_irqmp_init(grlib_irqmp_t *irqc, bootinfo_t *bootinfo)
{
	irqc->regs = (void *) km_map(bootinfo->intc_base, PAGE_SIZE,
	    PAGE_NOT_CACHEABLE);
	
	/* Clear all pending interrupts */
	pio_write_32(&irqc->regs->clear, 0xffffffff);
	
	/* Mask all interrupts */
	pio_write_32((void *) irqc->regs + GRLIB_IRQMP_MASK_OFFSET, 0);
}

int grlib_irqmp_inum_get(grlib_irqmp_t *irqc)
{
	uint32_t pending = pio_read_32(&irqc->regs->pending);
	
	for (unsigned int i = 1; i < 16; i++) {
		if (pending & (1 << i))
			return i;
	}
	
	return -1;
}

void grlib_irqmp_clear(grlib_irqmp_t *irqc, unsigned int inum)
{
	pio_write_32(&irqc->regs->clear, (1 << inum));
}

void grlib_irqmp_mask(grlib_irqmp_t *irqc, unsigned int src)
{
	uint32_t mask = pio_read_32((void *) irqc->regs +
	    GRLIB_IRQMP_MASK_OFFSET);
	
	mask &= ~(1 << src);
	pio_write_32((void *) irqc->regs + GRLIB_IRQMP_MASK_OFFSET, mask);
}

void grlib_irqmp_unmask(grlib_irqmp_t *irqc, unsigned int src)
{
	uint32_t mask = pio_read_32((void *) irqc->regs +
	    GRLIB_IRQMP_MASK_OFFSET);
	
	mask |= (1 << src);
	pio_write_32((void *) irqc->regs + GRLIB_IRQMP_MASK_OFFSET, mask);
}

/** @}
 */
