/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libriff
 * @{
 */
/**
 * @file Waveform Audio File Format (WAVE).
 */

#ifndef RIFF_RWAVE_H
#define RIFF_RWAVE_H

#include <errno.h>
#include <types/riff/rwave.h>

extern errno_t rwave_wopen(const char *, rwave_params_t *params, rwavew_t **);
extern errno_t rwave_write_samples(rwavew_t *, void *, size_t);
extern errno_t rwave_wclose(rwavew_t *);

extern errno_t rwave_ropen(const char *, rwave_params_t *params, rwaver_t **);
extern errno_t rwave_read_samples(rwaver_t *, void *, size_t, size_t *);
extern errno_t rwave_rclose(rwaver_t *);

#endif

/** @}
 */
