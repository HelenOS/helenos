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
