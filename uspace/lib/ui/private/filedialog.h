/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file File dialog structure
 *
 */

#ifndef _UI_PRIVATE_FILEDIALOG_H
#define _UI_PRIVATE_FILEDIALOG_H

/** Actual structure of file dialog.
 *
 * This is private to libui.
 */
struct ui_file_dialog {
	/** Dialog window */
	struct ui_window *window;
	/** File name entry */
	struct ui_entry *ename;
	/** File list */
	struct ui_file_list *flist;
	/** OK button */
	struct ui_pbutton *bok;
	/** Cancel button */
	struct ui_pbutton *bcancel;
	/** File dialog callbacks */
	struct ui_file_dialog_cb *cb;
	/** Callback argument */
	void *arg;
};

#endif

/** @}
 */
