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

/** @addtogroup uspace_srv_s3c24xx_uart
 * @{
 */
/**
 * @file
 * @brief Samsung S3C24xx on-chip UART driver.
 */

#ifndef S3C24XX_UART_H_
#define S3C24XX_UART_H_

#include <adt/circ_buf.h>
#include <async.h>
#include <fibril_synch.h>
#include <io/chardev_srv.h>
#include <stdint.h>

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

enum {
	s3c24xx_uart_buf_size = 64
};

/** S3C24xx UART instance */
typedef struct {
	/** Physical device address */
	uintptr_t paddr;

	/** Device I/O structure */
	s3c24xx_uart_io_t *io;

	/** Character device service */
	chardev_srvs_t cds;

	/** Service ID */
	service_id_t service_id;

	/** Circular buffer */
	circ_buf_t cbuf;
	/** Buffer */
	uint8_t buf[s3c24xx_uart_buf_size];
	/** Buffer lock */
	fibril_mutex_t buf_lock;
	/** Signal newly added data in buffer */
	fibril_condvar_t buf_cv;
} s3c24xx_uart_t;

#endif

/** @}
 */
