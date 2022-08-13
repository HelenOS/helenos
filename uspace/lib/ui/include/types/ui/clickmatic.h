/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Clickmatic
 */

#ifndef _UI_TYPES_CLICKMATIC_H
#define _UI_TYPES_CLICKMATIC_H

struct ui_clickmatic;
typedef struct ui_clickmatic ui_clickmatic_t;

/** Clickmatic callbacks */
typedef struct ui_clickmatic_cb {
	void (*clicked)(ui_clickmatic_t *, void *);
} ui_clickmatic_cb_t;

#endif

/** @}
 */
