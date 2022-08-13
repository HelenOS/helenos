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
#ifndef DRV_AUDIO_SB16_MIXER_H
#define DRV_AUDIO_SB16_MIXER_H

#include "registers.h"

typedef enum mixer_type {
	SB_MIXER_NONE,
	SB_MIXER_CT1335,
	SB_MIXER_CT1345,
	SB_MIXER_CT1745,
	SB_MIXER_UNKNOWN,
} sb_mixer_type_t;

typedef struct sb_mixer {
	sb16_regs_t *regs;
	sb_mixer_type_t type;
} sb_mixer_t;

const char *sb_mixer_type_str(sb_mixer_type_t type);
errno_t sb_mixer_init(sb_mixer_t *mixer, sb16_regs_t *regs, sb_mixer_type_t type);
int sb_mixer_get_control_item_count(const sb_mixer_t *mixer);
errno_t sb_mixer_get_control_item_info(const sb_mixer_t *mixer, unsigned index,
    const char **name, unsigned *levels);
errno_t sb_mixer_get_control_item_value(const sb_mixer_t *mixer, unsigned index,
    unsigned *value);
errno_t sb_mixer_set_control_item_value(const sb_mixer_t *mixer, unsigned index,
    unsigned value);
#endif
/**
 * @}
 */
