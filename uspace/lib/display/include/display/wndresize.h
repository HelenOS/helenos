/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdisplay
 * @{
 */
/** @file
 */

#ifndef _LIBDISPLAY_DISPLAY_WNDRESIZE_H_
#define _LIBDISPLAY_DISPLAY_WNDRESIZE_H_

#include <stdbool.h>
#include "../types/display/cursor.h"
#include "../types/display/wndresize.h"

extern display_stock_cursor_t display_cursor_from_wrsz(display_wnd_rsztype_t);
extern bool display_wndrsz_valid(display_wnd_rsztype_t);

#endif

/** @}
 */
