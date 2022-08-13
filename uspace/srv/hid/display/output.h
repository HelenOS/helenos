/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server output
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include <gfx/context.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include "types/display/output.h"

extern errno_t ds_output_create(ds_output_t **);
extern errno_t ds_output_start_discovery(ds_output_t *);
extern void ds_output_destroy(ds_output_t *);

#endif

/** @}
 */
