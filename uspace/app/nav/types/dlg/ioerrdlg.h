/*
 * Copyright (c) 2025 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup nav
 * @{
 */
/**
 * @file I/O Error Dialog
 */

#ifndef TYPES_DLG_IOERRDLG_H
#define TYPES_DLG_IOERRDLG_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <ui/entry.h>
#include <ui/pbutton.h>
#include <ui/window.h>

struct io_err_dlg;
typedef struct io_err_dlg io_err_dlg_t;

enum {
	/** Maximum number of buttons in I/O error dialog. */
	io_err_dlg_maxbtn = 2
};

/** Which choices the user can select from. */
typedef enum {
	/** Abort, Retry */
	iedc_abort_retry
} io_err_dlg_choice_t;

/** I/O error dialog parameters */
typedef struct {
	/** First line of text */
	const char *text1;
	/** Second line of text */
	const char *text2;
	/** The choice that the user is given */
	io_err_dlg_choice_t choice;
} io_err_dlg_params_t;

/** I/O error dialog callback */
typedef struct io_err_dlg_cb {
	/** Abort button was pressed */
	void (*babort)(io_err_dlg_t *, void *);
	/** Retry button was pressed */
	void (*bretry)(io_err_dlg_t *, void *);
	/** Window closure requested (e.g. via close button) */
	void (*close)(io_err_dlg_t *, void *);
} io_err_dlg_cb_t;

/** I/O error dialog */
typedef struct io_err_dlg {
	/** Dialog window */
	ui_window_t *window;
	/** Abort button */
	ui_pbutton_t *babort;
	/** Retry button */
	ui_pbutton_t *bretry;
	/** New file dialog callbacks */
	io_err_dlg_cb_t *cb;
	/** Callback argument */
	void *arg;
} io_err_dlg_t;

#endif

/** @}
 */
