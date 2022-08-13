/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
