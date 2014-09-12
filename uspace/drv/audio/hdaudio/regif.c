/*
 * Copyright (c) 2014 Jiri Svoboda
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
