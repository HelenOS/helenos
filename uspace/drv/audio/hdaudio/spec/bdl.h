/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio buffer descriptor list
 */

#ifndef SPEC_BDL_H
#define SPEC_BDL_H

/** Buffer descriptor */
typedef struct {
	uint64_t address;
	uint32_t length;
	uint32_t flags;
} hda_buffer_desc_t;

/** Buffer descriptor flags bits */
typedef enum {
	bdf_ioc = 0
} hda_bd_flags_bits_t;

#endif

/** @}
 */
