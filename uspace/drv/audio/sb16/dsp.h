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

typedef struct sb_dsp_t {
	sb16_regs_t *regs;
	struct {
		uint8_t major;
		uint8_t minor;
	} version;
	struct {
		uint8_t *data;
		uint8_t *position;
		size_t size;
	} buffer;
	struct {
		const uint8_t *data;
		const uint8_t *position;
		size_t size;
		uint8_t mode;
	} playing;
} sb_dsp_t;

int sb_dsp_init(sb_dsp_t *dsp, sb16_regs_t *regs);
void sb_dsp_interrupt(sb_dsp_t *dsp);
int sb_dsp_play_direct(sb_dsp_t *dsp, const uint8_t *data, size_t size,
    unsigned sample_rate, unsigned channels, unsigned bit_depth);
int sb_dsp_play(sb_dsp_t *dsp, const uint8_t *data, size_t size,
    uint16_t sample_rate, unsigned channels, unsigned bit_depth);

#endif
/**
 * @}
 */
