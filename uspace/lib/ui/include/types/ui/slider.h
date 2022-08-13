/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Slider
 */

#ifndef _UI_TYPES_SLIDER_H
#define _UI_TYPES_SLIDER_H

#include <gfx/coord.h>

struct ui_slider;
typedef struct ui_slider ui_slider_t;

/** Slider callbacks */
typedef struct ui_slider_cb {
	void (*moved)(ui_slider_t *, void *, gfx_coord_t);
} ui_slider_cb_t;

#endif

/** @}
 */
