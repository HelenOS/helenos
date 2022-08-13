/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup trackmod
 * @{
 */
/**
 * @file Tracker module handling library.
 */

#ifndef TRACKMOD_H
#define TRACKMOD_H

#include "types/trackmod.h"

extern trackmod_module_t *trackmod_module_new(void);
extern errno_t trackmod_module_load(char *, trackmod_module_t **);
extern void trackmod_module_destroy(trackmod_module_t *);
extern errno_t trackmod_modplay_create(trackmod_module_t *, unsigned,
    trackmod_modplay_t **);
extern void trackmod_modplay_destroy(trackmod_modplay_t *);
extern void trackmod_modplay_get_samples(trackmod_modplay_t *, void *, size_t);
extern int trackmod_sample_get_frame(trackmod_sample_t *, size_t);

#endif

/** @}
 */
