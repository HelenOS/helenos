/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Prompt dialog
 */

#ifndef _UI_TYPES_PROMPTDIALOG_H
#define _UI_TYPES_PROMPTDIALOG_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <types/ui/wdecor.h>

struct ui_prompt_dialog;
typedef struct ui_prompt_dialog ui_prompt_dialog_t;

/** Prompt dialog parameters */
typedef struct {
	/** Window caption */
	const char *caption;
	/** Prompt text */
	const char *prompt;
	/** Initial entry text */
	const char *itext;
} ui_prompt_dialog_params_t;

/** Prompt dialog callback */
typedef struct ui_prompt_dialog_cb {
	/** OK button was pressed */
	void (*bok)(ui_prompt_dialog_t *, void *, const char *);
	/** Cancel button was pressed */
	void (*bcancel)(ui_prompt_dialog_t *, void *);
	/** Window closure requested (e.g. via close button) */
	void (*close)(ui_prompt_dialog_t *, void *);
} ui_prompt_dialog_cb_t;

#endif

/** @}
 */
