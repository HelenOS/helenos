/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
