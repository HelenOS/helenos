/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Message Dialog
 */

#ifndef _UI_TYPES_MSGDIALOG_H
#define _UI_TYPES_MSGDIALOG_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <types/ui/wdecor.h>

struct ui_msg_dialog;
typedef struct ui_msg_dialog ui_msg_dialog_t;

/** Message dialog parameters */
typedef struct {
	/** Window caption */
	const char *caption;
	/** Message text */
	const char *text;
} ui_msg_dialog_params_t;

/** Message dialog callback */
typedef struct ui_msg_dialog_cb {
	/** Dialog button was pressed */
	void (*button)(ui_msg_dialog_t *, void *, unsigned);
	/** Window closure requested (e.g. via close button) */
	void (*close)(ui_msg_dialog_t *, void *);
} ui_msg_dialog_cb_t;

#endif

/** @}
 */
