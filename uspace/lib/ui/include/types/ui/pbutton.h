/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Push button
 */

#ifndef _UI_TYPES_PBUTTON_H
#define _UI_TYPES_PBUTTON_H

#include <errno.h>
#include <gfx/coord.h>

struct ui_pbutton;
typedef struct ui_pbutton ui_pbutton_t;

/** UI push button flags */
typedef enum {
	/** Do not depress the button in text mode */
	ui_pbf_no_text_depress = 0x1
} ui_pbutton_flags_t;

/** Push button callbacks */
typedef struct ui_pbutton_cb {
	void (*clicked)(ui_pbutton_t *, void *);
	void (*down)(ui_pbutton_t *, void *);
	void (*up)(ui_pbutton_t *, void *);
} ui_pbutton_cb_t;

/** Push button decoration ops */
typedef struct ui_pbutton_decor_ops {
	errno_t (*paint)(ui_pbutton_t *, void *, gfx_coord2_t *);
} ui_pbutton_decor_ops_t;

#endif

/** @}
 */
