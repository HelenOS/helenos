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

/** @addtogroup arm32boot
 * @{
 */
/** @file
 * @brief bootloader output logic
 */

#include <typedefs.h>
#include <arch/main.h>
#include <putchar.h>
#include <str.h>

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

#ifdef MACHINE_testarm

/** Send a byte to the GXemul testarm serial console.
 *
 * @param byte		Byte to send.
 */
static void scons_sendb_testarm(uint8_t byte)
{
	*((volatile uint8_t *) TESTARM_SCONS_ADDR) = byte;
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

/** Send a byte to the serial console.
 *
 * @param byte		Byte to send.
 */
static void scons_sendb(uint8_t byte)
{
#ifdef MACHINE_gta02
	scons_sendb_gta02(byte);
#endif
#ifdef MACHINE_testarm
	scons_sendb_testarm(byte);
#endif
#ifdef MACHINE_integratorcp
	scons_sendb_icp(byte);
#endif
}

/** Display a character
 *
 * @param ch	Character to display
 */
void putchar(const wchar_t ch)
{
	if (ch == '\n')
		scons_sendb('\r');

	if (ascii_check(ch))
		scons_sendb((uint8_t) ch);
	else
		scons_sendb(U_SPECIAL);
}

/** @}
 */
