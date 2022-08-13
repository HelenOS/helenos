/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup hdaudio
 * @{
 */
/** @file High Definition Audio PCM interface
 */

#ifndef PCM_IFACE_H
#define PCM_IFACE_H

#include <audio_pcm_iface.h>

#include "hdaudio.h"

extern audio_pcm_iface_t hda_pcm_iface;
extern void hda_pcm_event(hda_t *, pcm_event_t);

#endif

/** @}
 */
