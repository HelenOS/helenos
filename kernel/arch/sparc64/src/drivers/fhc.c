/*
 * Copyright (C) 2006 Jakub Jermar
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
 * @brief	FireHose Controller (FHC) driver.
 *
 * Note that this driver is a result of reverse engineering
 * rather than implementation of a specification. This
 * is due to the fact that the FHC documentation is not
 * publicly available.
 */

#include <arch/drivers/fhc.h>
#include <arch/mm/page.h>
#include <arch/types.h>
#include <typedefs.h>

#include <genarch/kbd/z8530.h>

volatile uint32_t *fhc = NULL;

#define FHC_UART_ADDR	0x1fff8808000ULL		/* hardcoded for Simics simulation */

#define FHC_UART_IMAP	0x0
#define FHC_UART_ICLR	0x4

void fhc_init(void)
{
	fhc = (void *) hw_map(FHC_UART_ADDR, PAGE_SIZE);

	(void) fhc[FHC_UART_ICLR];
	fhc[FHC_UART_ICLR] = 0;
	(void) fhc[FHC_UART_IMAP];
	fhc[FHC_UART_IMAP] = Z8530_INTRCV_DATA0;	/* hardcoded for Simics simulation */
	(void) fhc[FHC_UART_IMAP];
	fhc[FHC_UART_IMAP] = 0x80000000;		/* hardcoded for Simics simulation */
}

void fhc_uart_reset(void)
{
	(void) fhc[FHC_UART_ICLR];
	fhc[FHC_UART_ICLR] = 0;
}

/** @}
 */
