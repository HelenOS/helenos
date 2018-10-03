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
 * @brief Samsung S3C24xx on-chip interrupt controller driver.
 */

#ifndef KERN_S3C24XX_IRQC_H_
#define KERN_S3C24XX_IRQC_H_

#include <typedefs.h>

/** Physical address where S3C24XX Interrupt controller is mapped */
#define S3C24XX_IRQC_ADDRESS	0x4a000000

/** S3C24xx on-chip interrupt controller registers */
typedef struct {
	ioport32_t srcpnd;	/**< Source pending */
	ioport32_t intmod;	/**< Interrupt mode */
	ioport32_t intmsk;	/**< Interrupt mask */
	ioport32_t priority;	/**< Priority */
	ioport32_t intpnd;	/**< Interrupt pending */
	ioport32_t intoffset;	/**< Interrupt offset */
	ioport32_t subsrcpnd;	/**< Sub source pending */
	ioport32_t intsubmsk;	/** Interrupt sub mask */
} s3c24xx_irqc_regs_t;

/** S3C24xx Interrupt source numbers.
 *
 * These correspond to bit numbers in srcpnd, intmod, intmsk and intpnd
 * registers as well as to the values read from the intoffset register.
 */
enum s3c24xx_int_source {
	S3C24XX_INT_ADC		= 31,
	S3C24XX_INT_RTC		= 30,
	S3C24XX_INT_SPI1	= 29,
	S3C24XX_INT_UART0	= 28,
	S3C24XX_INT_IIC		= 27,
	S3C24XX_INT_USBH	= 26,
	S3C24XX_INT_USBD	= 25,
	S3C24XX_INT_NFCON	= 24,
	S3C24XX_INT_UART1	= 23,
	S3C24XX_INT_SPI0	= 22,
	S3C24XX_INT_SDI		= 21,
	S3C24XX_INT_DMA3	= 20,
	S3C24XX_INT_DMA2	= 19,
	S3C24XX_INT_DMA1	= 18,
	S3C24XX_INT_DMA0	= 17,
	S3C24XX_INT_LCD		= 16,
	S3C24XX_INT_UART2	= 15,
	S3C24XX_INT_TIMER4	= 14,
	S3C24XX_INT_TIMER3	= 13,
	S3C24XX_INT_TIMER2	= 12,
	S3C24XX_INT_TIMER1	= 11,
	S3C24XX_INT_TIMER0	= 10,
	S3C24XX_INT_WDT_AC97	= 9,
	S3C24XX_INT_TICK	= 8,
	S3C24XX_nBATT_FLT	= 7,
	S3C24XX_INT_CAM		= 6,
	S3C24XX_EINT8_23	= 5,
	S3C24XX_EINT4_7		= 4,
	S3C24XX_EINT3		= 3,
	S3C24XX_EINT2		= 2,
	S3C24XX_EINT1		= 1,
	S3C24XX_EINT0		= 0
};

/** S3C24xx Interrupt sub-source numbers.
 *
 * These correspond to bit numbers in the intsubmsk register.
 */
enum s3c24xx_int_subsource {
	S3C24XX_SUBINT_AC97	= 14,
	S3C24XX_SUBINT_WDT	= 13,
	S3C24XX_SUBINT_CAM_P	= 12,
	S3C24XX_SUBINT_CAM_C	= 11,
	S3C24XX_SUBINT_ADC_S	= 10,
	S3C24XX_SUBINT_TC	= 9,
	S3C24XX_SUBINT_ERR2	= 8,
	S3C24XX_SUBINT_TXD2	= 7,
	S3C24XX_SUBINT_RXD2	= 6,
	S3C24XX_SUBINT_ERR1	= 5,
	S3C24XX_SUBINT_TXD1	= 4,
	S3C24XX_SUBINT_RXD1	= 3,
	S3C24XX_SUBINT_ERR0	= 2,
	S3C24XX_SUBINT_TXD0	= 1,
	S3C24XX_SUBINT_RXD0	= 0
};

#define S3C24XX_INT_BIT(source) (1 << (source))
#define S3C24XX_SUBINT_BIT(subsource) (1 << (subsource))

typedef struct {
	s3c24xx_irqc_regs_t *regs;
} s3c24xx_irqc_t;

extern void s3c24xx_irqc_init(s3c24xx_irqc_t *, s3c24xx_irqc_regs_t *);
extern unsigned s3c24xx_irqc_inum_get(s3c24xx_irqc_t *);
extern void s3c24xx_irqc_clear(s3c24xx_irqc_t *, unsigned);
extern void s3c24xx_irqc_src_enable(s3c24xx_irqc_t *, unsigned);
extern void s3c24xx_irqc_src_disable(s3c24xx_irqc_t *, unsigned);
extern void s3c24xx_irqc_subsrc_enable(s3c24xx_irqc_t *, unsigned);
extern void s3c24xx_irqc_subsrc_disable(s3c24xx_irqc_t *, unsigned);

#endif

/** @}
 */
