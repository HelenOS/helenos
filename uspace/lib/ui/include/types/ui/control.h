/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file UI control
 */

#ifndef _UI_TYPES_CONTROL_H
#define _UI_TYPES_CONTROL_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <types/ui/event.h>

struct ui_control;
typedef struct ui_control ui_control_t;

/** UI control ops */
typedef struct ui_control_ops {
	/** Destroy control */
	void (*destroy)(void *);
	/** Paint */
	errno_t (*paint)(void *);
	/** Keyboard event */
	ui_evclaim_t (*kbd_event)(void *, kbd_event_t *);
	/** Position event */
	ui_evclaim_t (*pos_event)(void *, pos_event_t *);
	/** Unfocus */
	void (*unfocus)(void *);
} ui_control_ops_t;

#endif

/** @}
 */
