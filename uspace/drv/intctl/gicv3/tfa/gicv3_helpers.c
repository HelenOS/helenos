/*
 * Copyright (c) 2015-2023 Arm Limited and Contributors.
 * Copyright (c) 2023 NVIDIA Corporation.
 * Copyright (c) 2026 Jakub Kodym
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

/** @addtogroup uspace_drv_gicv3
 * @{
 */
/** @file GICv3 driver.
 * @brief ARM Generic Interrupt Controller v3 helper functions.
 */

#include "gic_common.h"
#include "gicv3_private.h"
#include "gicv3_common_private.h"

/*
 * Helper function to get the maximum SPI INTID + 1.
 */
unsigned int gicv3_get_spi_limit(uintptr_t gicd_base)
{
	unsigned int spi_limit;
	unsigned int typer_reg = gicd_read_typer(gicd_base);

	/* (maximum SPI INTID + 1) is equal to 32 * (GICD_TYPER.ITLinesNumber+1) */
	spi_limit = ((typer_reg & TYPER_IT_LINES_NO_MASK) + 1U) << 5;

	/* Filter out special INTIDs 1020-1023 */
	if (spi_limit > (MAX_SPI_ID + 1U)) {
		return MAX_SPI_ID + 1U;
	}

	return spi_limit;
}
