/*
 * Copyright (c) 2007 Michal Kebrt
 * Copyright (c) 2009 Vineeth Pillai
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
 * @brief bootloader output logic
 */

#include <arch/main.h>
#include <putchar.h>
#include <stddef.h>
#include <stdint.h>
#include <str.h>

#ifdef MACHINE_beaglebone

/** Send a byte to the am335x serial console.
 *
 * @param byte		Byte to send.
 */
static void scons_sendb_bbone(uint8_t byte)
{
	volatile uint32_t *thr =
	    (volatile uint32_t *) BBONE_SCONS_THR;
	volatile uint32_t *ssr =
	    (volatile uint32_t *) BBONE_SCONS_SSR;

	/* Wait until transmitter is empty */
	while (*ssr & BBONE_TXFIFO_FULL)
		;

	/* Transmit byte */
	*thr = (uint32_t) byte;
}

#endif

#ifdef MACHINE_beagleboardxm

/** Send a byte to the amdm37x serial console.
 *
 * @param byte		Byte to send.
 */
static void scons_sendb_bbxm(uint8_t byte)
{
	volatile uint32_t *thr =
	    (volatile uint32_t *)BBXM_SCONS_THR;
	volatile uint32_t *ssr =
	    (volatile uint32_t *)BBXM_SCONS_SSR;

	/* Wait until transmitter is empty. */
	while ((*ssr & BBXM_THR_FULL) == 1)
		;

	/* Transmit byte. */
	*thr = (uint32_t) byte;
}

#endif

#ifdef MACHINE_gta02

/** Send a byte to the gta02 serial console.
 *
 * @param byte		Byte to send.
 */
static void scons_sendb_gta02(uint8_t byte)
{
	volatile uint32_t *utrstat;
	volatile uint32_t *utxh;

	utrstat = (volatile uint32_t *) GTA02_SCONS_UTRSTAT;
	utxh    = (volatile uint32_t *) GTA02_SCONS_UTXH;

	/* Wait until transmitter is empty. */
	while ((*utrstat & S3C24XX_UTXH_TX_EMPTY) == 0)
		;

	/* Transmit byte. */
	*utxh = (uint32_t) byte;
}

#endif

#ifdef MACHINE_integratorcp

/** Send a byte to the IntegratorCP serial console.
 *
 * @param byte		Byte to send.
 */
static void scons_sendb_icp(uint8_t byte)
{
	*((volatile uint8_t *) ICP_SCONS_ADDR) = byte;
}

#endif

#ifdef MACHINE_raspberrypi

static int raspi_init;

static inline void write32(uint32_t addr, uint32_t data)
{
	*(volatile uint32_t *)(addr) = data;
}

static inline uint32_t read32(uint32_t addr)
{
	return *(volatile uint32_t *)(addr);
}

static void scons_init_raspi(void)
{
	write32(BCM2835_UART0_CR, 0x0);		/* Disable UART */
	write32(BCM2835_UART0_ICR, 0x7f);	/* Clear interrupts */
	write32(BCM2835_UART0_IBRD, 1);		/* Set integer baud rate */
	write32(BCM2835_UART0_FBRD, 40);	/* Set fractional baud rate */
	write32(BCM2835_UART0_LCRH,
	    BCM2835_UART0_LCRH_FEN |		/* Enable FIFOs */
	    BCM2835_UART0_LCRH_WL8);		/* Word length: 8 */
	write32(BCM2835_UART0_CR,
	    BCM2835_UART0_CR_UARTEN |		/* Enable UART */
	    BCM2835_UART0_CR_TXE |		/* Enable TX */
	    BCM2835_UART0_CR_RXE);		/* Enable RX */
}

static void scons_sendb_raspi(uint8_t byte)
{
	if (!raspi_init) {
		scons_init_raspi();
		raspi_init = 1;
	}

	while (read32(BCM2835_UART0_FR) & BCM2835_UART0_FR_TXFF)
		;

	write32(BCM2835_UART0_DR, byte);
}
#endif

/** Send a byte to the serial console.
 *
 * @param byte		Byte to send.
 */
static void scons_sendb(uint8_t byte)
{
#ifdef MACHINE_beaglebone
	scons_sendb_bbone(byte);
#endif
#ifdef MACHINE_beagleboardxm
	scons_sendb_bbxm(byte);
#endif
#ifdef MACHINE_gta02
	scons_sendb_gta02(byte);
#endif
#ifdef MACHINE_integratorcp
	scons_sendb_icp(byte);
#endif
#ifdef MACHINE_raspberrypi
	scons_sendb_raspi(byte);
#endif
}

/** Display a character
 *
 * @param ch Character to display
 *
 */
void putuchar(const char32_t ch)
{
	if (ch == '\n')
		scons_sendb('\r');

	if (ascii_check(ch))
		scons_sendb((uint8_t) ch);
	else
		scons_sendb('?');
}

/** @}
 */
