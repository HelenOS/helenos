/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Message dialog structure
 *
 */

#ifndef _UI_PRIVATE_MSGDIALOG_H
#define _UI_PRIVATE_MSGDIALOG_H

/** Actual structure of message dialog.
 *
 * This is private to libui.
 */
struct ui_msg_dialog {
	/** Dialog window */
	struct ui_window *window;
	/** OK button */
	struct ui_pbutton *bok;
	/** Message dialog callbacks */
	struct ui_msg_dialog_cb *cb;
	/** Callback argument */
	void *arg;
};

#endif

/** @}
 */
