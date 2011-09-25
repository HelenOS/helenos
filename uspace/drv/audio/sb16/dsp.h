/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup drvaudiosb16
 * @{
 */
/** @file
 * @brief Sound Blaster Digital Sound Processor (DSP) helper functions.
 */
#ifndef DRV_AUDIO_SB16_DSP_H
#define DRV_AUDIO_SB16_DSP_H

#include <libarch/ddi.h>
#include <errno.h>

#include "registers.h"
#include "dsp_commands.h"

#ifndef DSP_PIO_DELAY
#define DSP_PIO_DELAY udelay(10)
#endif

#ifndef DSP_RETRY_COUNT
#define DSP_RETRY_COUNT 100
#endif

#define DSP_RESET_RESPONSE 0xaa

static inline int dsp_write(sb16_regs_t *regs, uint8_t data)
{
	uint8_t status;
	size_t attempts = DSP_RETRY_COUNT;
	do {
		DSP_PIO_DELAY;
		status = pio_read_8(&regs->dsp_write);
	} while (--attempts && ((status & DSP_WRITE_BUSY) != 0));
	if ((status & DSP_WRITE_BUSY))
		return EIO;
	DSP_PIO_DELAY;
	pio_write_8(&regs->dsp_write, data);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static inline int dsp_read(sb16_regs_t *regs, uint8_t *data)
{
	assert(data);
	uint8_t status;
	size_t attempts = DSP_RETRY_COUNT;
	do {
		DSP_PIO_DELAY;
		status = pio_read_8(&regs->dsp_read_status);
	} while (--attempts && ((status & DSP_READ_READY) == 0));

	if ((status & DSP_READ_READY) == 0)
		return EIO;

	DSP_PIO_DELAY;
	*data = pio_read_8(&regs->dsp_data_read);
	return EOK;
}
/*----------------------------------------------------------------------------*/
static inline void dsp_reset(sb16_regs_t *regs)
{
	/* Reset DSP, see Chapter 2 of Sound Blaster HW programming guide */
	pio_write_8(&regs->dsp_reset, 1);
	udelay(3); /* Keep reset for 3 us */
	pio_write_8(&regs->dsp_reset, 0);
}
/*----------------------------------------------------------------------------*/
int dsp_play_direct(sb16_regs_t *regs, const uint8_t *data, size_t size,
    unsigned sample_rate, unsigned channels, unsigned bit_depth);
#endif
/**
 * @}
 */
