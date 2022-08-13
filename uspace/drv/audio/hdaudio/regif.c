/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio register interface
 */

#include <byteorder.h>
#include <ddi.h>
#include <stdint.h>

#include "regif.h"

uint8_t hda_reg8_read(uint8_t *r)
{
	return pio_read_8(r);
}

uint16_t hda_reg16_read(uint16_t *r)
{
	return uint16_t_le2host(pio_read_16(r));
}

uint32_t hda_reg32_read(uint32_t *r)
{
	return uint32_t_le2host(pio_read_32(r));
}

void hda_reg8_write(uint8_t *r, uint8_t val)
{
	pio_write_8(r, val);
}

void hda_reg16_write(uint16_t *r, uint16_t val)
{
	pio_write_16(r, host2uint16_t_le(val));
}

void hda_reg32_write(uint32_t *r, uint32_t val)
{
	pio_write_32(r, host2uint32_t_le(val));
}

/** @}
 */
