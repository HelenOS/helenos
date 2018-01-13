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
 * @brief SB16 main structure combining all functionality
 */
#ifndef DRV_AUDIO_SB16_SB16_H
#define DRV_AUDIO_SB16_SB16_H

#include <ddf/driver.h>
#include <ddi.h>
#include <device/hw_res_parsed.h>

#include "dsp.h"
#include "mixer.h"
#include "registers.h"

typedef struct sb16 {
	sb16_regs_t *regs;
	mpu_regs_t *mpu_regs;
	sb_dsp_t dsp;
	sb_mixer_t mixer;
} sb16_t;

size_t sb16_irq_code_size(void);
void sb16_irq_code(addr_range_t *regs, int dma8, int dma16, irq_cmd_t cmds[], irq_pio_range_t ranges[]);
errno_t sb16_init_sb16(sb16_t *sb, addr_range_t *regs, ddf_dev_t *dev, int dma8, int dma16);
errno_t sb16_init_mpu(sb16_t *sb, addr_range_t *regs);
void sb16_interrupt(sb16_t *sb);

#endif
/**
 * @}
 */
