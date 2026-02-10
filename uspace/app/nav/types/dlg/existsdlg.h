/*
 * Copyright (c) 2026 Jiri Svoboda
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
 * @file File/directory exists Dialog
 */

#ifndef TYPES_DLG_EXISTSDLG_H
#define TYPES_DLG_EXISTSDLG_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <ui/entry.h>
#include <ui/pbutton.h>
#include <ui/window.h>

struct exists_dlg;
typedef struct exists_dlg exists_dlg_t;

enum {
	/** Maximum number of buttons in file/directory exists dialog. */
	exists_dlg_maxbtn = 2
};

/** File/directory exists dialog parameters */
typedef struct {
	/** First line of text */
	const char *text1;
} exists_dlg_params_t;

/** File/directory exists dialog callback */
typedef struct exists_dlg_cb {
	/** Overwrite button was pressed */
	void (*boverwrite)(exists_dlg_t *, void *);
	/** Skip button was pressed */
	void (*bskip)(exists_dlg_t *, void *);
	/** Abort button was pressed */
	void (*babort)(exists_dlg_t *, void *);
	/** Window closure requested (e.g. via close button) */
	void (*close)(exists_dlg_t *, void *);
} exists_dlg_cb_t;

/** File/directory exists dialog */
typedef struct exists_dlg {
	/** Dialog window */
	ui_window_t *window;
	/** Overwrite button */
	ui_pbutton_t *boverwrite;
	/** Skip button */
	ui_pbutton_t *bskip;
	/** Abort button */
	ui_pbutton_t *babort;
	/** New file dialog callbacks */
	exists_dlg_cb_t *cb;
	/** Callback argument */
	void *arg;
} exists_dlg_t;

#endif

/** @}
 */
