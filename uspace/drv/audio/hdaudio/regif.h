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

#ifndef REGIF_H
#define REGIF_H

#include <stdint.h>

extern uint8_t hda_reg8_read(uint8_t *);
extern uint16_t hda_reg16_read(uint16_t *);
extern uint32_t hda_reg32_read(uint32_t *);
extern void hda_reg8_write(uint8_t *, uint8_t);
extern void hda_reg16_write(uint16_t *, uint16_t);
extern void hda_reg32_write(uint32_t *, uint32_t);

#endif

/** @}
 */
