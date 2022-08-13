/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Samsung S3C24xx on-chip UART driver.
 */

#ifndef KERN_S3C24XX_UART_H_
#define KERN_S3C24XX_UART_H_

#include <ddi/ddi.h>
#include <ddi/irq.h>
#include <console/chardev.h>
#include <typedefs.h>

/** S3C24xx UART I/O */
typedef struct {
	uint32_t ulcon;
	uint32_t ucon;
	uint32_t ufcon;
	uint32_t umcon;

	uint32_t utrstat;
	uint32_t uerstat;
	uint32_t ufstat;
	uint32_t umstat;

	uint32_t utxh;
	uint32_t urxh;

	uint32_t ubrdiv;
} s3c24xx_uart_io_t;

/* Bits in UTRSTAT register */
#define S3C24XX_UTRSTAT_TX_EMPTY	0x4
#define S3C24XX_UTRSTAT_RDATA		0x1

/* Bits in UFSTAT register */
#define S3C24XX_UFSTAT_TX_FULL		0x4000
#define S3C24XX_UFSTAT_RX_FULL		0x0040
#define S3C24XX_UFSTAT_RX_COUNT		0x002f

/* Bits in UCON register */
#define UCON_RX_INT_LEVEL		0x100

/* Bits in UFCON register */
#define UFCON_TX_FIFO_TLEVEL_EMPTY	0x00
#define UFCON_RX_FIFO_TLEVEL_1B		0x00
#define UFCON_FIFO_ENABLE		0x01

/** S3C24xx UART instance */
typedef struct {
	s3c24xx_uart_io_t *io;
	indev_t *indev;
	irq_t irq;
	parea_t parea;
} s3c24xx_uart_t;

extern outdev_t *s3c24xx_uart_init(uintptr_t, inr_t inr);
extern void s3c24xx_uart_input_wire(s3c24xx_uart_t *,
    indev_t *);

#endif

/** @}
 */
