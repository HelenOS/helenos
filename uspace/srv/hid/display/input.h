/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/** @file Input events
 */

#ifndef INPUT_H
#define INPUT_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include "types/display/display.h"

extern errno_t ds_input_open(ds_display_t *);
extern void ds_input_close(ds_display_t *);

#endif

/** @}
 */
