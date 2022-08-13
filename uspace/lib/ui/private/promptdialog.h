/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Prompt dialog structure
 *
 */

#ifndef _UI_PRIVATE_PROMPTDIALOG_H
#define _UI_PRIVATE_PROMPTDIALOG_H

/** Actual structure of prompt dialog.
 *
 * This is private to libui.
 */
struct ui_prompt_dialog {
	/** Dialog window */
	struct ui_window *window;
	/** File name entry */
	struct ui_entry *ename;
	/** OK button */
	struct ui_pbutton *bok;
	/** Cancel button */
	struct ui_pbutton *bcancel;
	/** Prompt dialog callbacks */
	struct ui_prompt_dialog_cb *cb;
	/** Callback argument */
	void *arg;
};

#endif

/** @}
 */
