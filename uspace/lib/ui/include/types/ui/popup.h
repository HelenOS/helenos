/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Popup window
 */

#ifndef _UI_TYPES_POPUP_H
#define _UI_TYPES_POPUP_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>

struct ui_popup;
typedef struct ui_popup ui_popup_t;

/** Popup window parameters */
typedef struct {
	/** Popup rectangle */
	gfx_rect_t rect;
	/** Placement rectangle close to which popup should be placed */
	gfx_rect_t place;
} ui_popup_params_t;

/** Popup callbacks */
typedef struct ui_popup_cb {
	void (*close)(ui_popup_t *, void *);
	void (*kbd)(ui_popup_t *, void *, kbd_event_t *);
	errno_t (*paint)(ui_popup_t *, void *);
	void (*pos)(ui_popup_t *, void *, pos_event_t *);
} ui_popup_cb_t;

#endif

/** @}
 */
