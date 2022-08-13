/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Radio button
 */

#ifndef _UI_TYPES_RBUTTON_H
#define _UI_TYPES_RBUTTON_H

struct ui_rbutton_group;
typedef struct ui_rbutton_group ui_rbutton_group_t;

struct ui_rbutton;
typedef struct ui_rbutton ui_rbutton_t;

/** Radio button callbacks */
typedef struct ui_rbutton_group_cb {
	void (*selected)(ui_rbutton_group_t *, void *, void *);
} ui_rbutton_group_cb_t;

#endif

/** @}
 */
