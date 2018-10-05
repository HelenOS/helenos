/*
 * Copyright (c) 2007 Michal Kebrt
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

/** @addtogroup boot_arm32
 * @{
 */
/** @file
 * @brief Boot related declarations.
 */

#ifndef BOOT_arm32_MAIN_H
#define BOOT_arm32_MAIN_H

/** Address where characters to be printed are expected. */

/** BeagleBoard-xM UART register address
 *
 * This is UART3 of AM/DM37x CPU
 */
#define BBXM_SCONS_THR          0x49020000
#define BBXM_SCONS_SSR          0x49020044

/* Check this bit before writing (tx fifo full) */
#define BBXM_THR_FULL           0x00000001

/** Beaglebone UART register addresses
 *
 * This is UART0 of AM335x CPU
 */
#define BBONE_SCONS_THR         0x44E09000
#define BBONE_SCONS_SSR         0x44E09044

/** Check this bit before writing (tx fifo full) */
#define BBONE_TXFIFO_FULL       0x00000001

/** GTA02 serial console UART register addresses.
 *
 * This is UART channel 2 of the S3C24xx CPU
 */
#define GTA02_SCONS_UTRSTAT	0x50008010
#define GTA02_SCONS_UTXH	0x50008020

/* Bits in UTXH register */
#define S3C24XX_UTXH_TX_EMPTY	0x00000004

/** IntegratorCP serial console output register */
#define ICP_SCONS_ADDR		0x16000000

/** Raspberry PI serial console registers */
#define BCM2835_UART0_BASE	0x20201000
#define BCM2835_UART0_DR	(BCM2835_UART0_BASE + 0x00)
#define BCM2835_UART0_FR	(BCM2835_UART0_BASE + 0x18)
#define BCM2835_UART0_ILPR	(BCM2835_UART0_BASE + 0x20)
#define BCM2835_UART0_IBRD	(BCM2835_UART0_BASE + 0x24)
#define BCM2835_UART0_FBRD	(BCM2835_UART0_BASE + 0x28)
#define BCM2835_UART0_LCRH	(BCM2835_UART0_BASE + 0x2C)
#define BCM2835_UART0_CR	(BCM2835_UART0_BASE + 0x30)
#define BCM2835_UART0_ICR	(BCM2835_UART0_BASE + 0x44)

#define BCM2835_UART0_FR_TXFF	(1 << 5)
#define BCM2835_UART0_LCRH_FEN	(1 << 4)
#define BCM2835_UART0_LCRH_WL8	((1 << 5) | (1 << 6))
#define BCM2835_UART0_CR_UARTEN	(1 << 0)
#define BCM2835_UART0_CR_TXE	(1 << 8)
#define BCM2835_UART0_CR_RXE	(1 << 9)

extern void bootstrap(void);

#endif

/** @}
 */
