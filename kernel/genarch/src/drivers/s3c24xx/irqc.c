/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Samsung S3C24xx on-chip interrupt controller.
 *
 * This IRQC is present on the Samsung S3C24xx CPU (on the gta02 platform).
 */

#include <genarch/drivers/s3c24xx/irqc.h>
#include <arch/asm.h>

/** Correspondence between interrupt sources and sub-sources. */
static unsigned s3c24xx_subsrc_src[][2] = {
	{ S3C24XX_SUBINT_CAM_P, S3C24XX_INT_CAM },
	{ S3C24XX_SUBINT_CAM_C, S3C24XX_INT_CAM },
	{ S3C24XX_SUBINT_ADC_S, S3C24XX_INT_ADC },
	{ S3C24XX_SUBINT_TC, S3C24XX_INT_ADC },
	{ S3C24XX_SUBINT_ERR2, S3C24XX_INT_UART2 },
	{ S3C24XX_SUBINT_TXD2, S3C24XX_INT_UART2 },
	{ S3C24XX_SUBINT_RXD2, S3C24XX_INT_UART2 },
	{ S3C24XX_SUBINT_ERR1, S3C24XX_INT_UART1 },
	{ S3C24XX_SUBINT_TXD1, S3C24XX_INT_UART1 },
	{ S3C24XX_SUBINT_RXD1, S3C24XX_INT_UART1 },
	{ S3C24XX_SUBINT_ERR0, S3C24XX_INT_UART0 },
	{ S3C24XX_SUBINT_TXD0, S3C24XX_INT_UART0 },
	{ S3C24XX_SUBINT_RXD0, S3C24XX_INT_UART0 }
};

/** Initialize S3C24xx interrupt controller.
 *
 * @param irqc	Instance structure
 * @param regs	Register I/O structure
 */
void s3c24xx_irqc_init(s3c24xx_irqc_t *irqc, s3c24xx_irqc_regs_t *regs)
{
	irqc->regs = regs;

	/* Make all interrupt sources use IRQ mode (not FIQ). */
	pio_write_32(&regs->intmod, 0x00000000);

	/* Disable all interrupt sources. */
	pio_write_32(&regs->intmsk, 0xffffffff);

	/* Disable interrupts from all sub-sources. */
	pio_write_32(&regs->intsubmsk, 0xffffffff);
}

/** Obtain number of pending interrupt. */
unsigned s3c24xx_irqc_inum_get(s3c24xx_irqc_t *irqc)
{
	return pio_read_32(&irqc->regs->intoffset);
}

/** Clear pending interrupt condition including sub-sources.
 *
 * Clear source and interrupt pending condition and also automatically clear
 * any sub-source pending condition pertaining to the source.
 */
void s3c24xx_irqc_clear(s3c24xx_irqc_t *irqc, unsigned inum)
{
	unsigned src, subsrc;
	unsigned entries, i;

	entries = sizeof(s3c24xx_subsrc_src) / sizeof(s3c24xx_subsrc_src[0]);

	for (i = 0; i < entries; i++) {
		subsrc = s3c24xx_subsrc_src[i][0];
		src = s3c24xx_subsrc_src[i][1];

		if (src == inum) {
			pio_write_32(&irqc->regs->subsrcpnd,
			    S3C24XX_SUBINT_BIT(subsrc));
		}
	}

	pio_write_32(&irqc->regs->srcpnd, S3C24XX_INT_BIT(inum));
	pio_write_32(&irqc->regs->intpnd, S3C24XX_INT_BIT(inum));
}

/** Enable interrupts from the specified source. */
void s3c24xx_irqc_src_enable(s3c24xx_irqc_t *irqc, unsigned src)
{
	pio_write_32(&irqc->regs->intmsk, pio_read_32(&irqc->regs->intmsk) &
	    ~S3C24XX_INT_BIT(src));
}

/** Disable interrupts from the specified source. */
void s3c24xx_irqc_src_disable(s3c24xx_irqc_t *irqc, unsigned src)
{
	pio_write_32(&irqc->regs->intmsk, pio_read_32(&irqc->regs->intmsk) |
	    S3C24XX_INT_BIT(src));
}

/** Enable interrupts from the specified sub-source. */
void s3c24xx_irqc_subsrc_enable(s3c24xx_irqc_t *irqc, unsigned subsrc)
{
	pio_write_32(&irqc->regs->intsubmsk,
	    pio_read_32(&irqc->regs->intsubmsk) &
	    ~S3C24XX_SUBINT_BIT(subsrc));
}

/** Disable interrupts from the specified sub-source. */
void s3c24xx_irqc_subsrc_disable(s3c24xx_irqc_t *irqc, unsigned subsrc)
{
	pio_write_32(&irqc->regs->intsubmsk,
	    pio_read_32(&irqc->regs->intsubmsk) |
	    S3C24XX_SUBINT_BIT(subsrc));
}

/** @}
 */
