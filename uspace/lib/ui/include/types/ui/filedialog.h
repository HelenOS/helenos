/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file File dialog
 */

#ifndef _UI_TYPES_FILEDIALOG_H
#define _UI_TYPES_FILEDIALOG_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <types/ui/wdecor.h>

struct ui_file_dialog;
typedef struct ui_file_dialog ui_file_dialog_t;

/** File dialog parameters */
typedef struct {
	/** Window caption */
	const char *caption;
	/** Initial file name */
	const char *ifname;
} ui_file_dialog_params_t;

/** File dialog callback */
typedef struct ui_file_dialog_cb {
	/** OK button was pressed */
	void (*bok)(ui_file_dialog_t *, void *, const char *);
	/** Cancel button was pressed */
	void (*bcancel)(ui_file_dialog_t *, void *);
	/** Window closure requested (e.g. via close button) */
	void (*close)(ui_file_dialog_t *, void *);
} ui_file_dialog_cb_t;

#endif

/** @}
 */
