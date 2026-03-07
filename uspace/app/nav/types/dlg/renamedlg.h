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
 * @file Rename dialog
 */

#ifndef TYPES_DLG_RENAMEDLG_H
#define TYPES_DLG_RENAMEDLG_H

#include <errno.h>
#include <fmgt.h>
#include <ui/entry.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/window.h>
#include <stdbool.h>

/** Rename dialog */
typedef struct rename_dlg {
	/** Dialog window */
	ui_window_t *window;
	/** "Rename 'xxx'" */
	ui_label_t *lrename;
	/** New name text entry */
	ui_entry_t *enew_name;
	/** OK button */
	ui_pbutton_t *bok;
	/** Cancel button */
	ui_pbutton_t *bcancel;
	/** Rename dialog callbacks */
	struct rename_dlg_cb *cb;
	/** Callback argument */
	void *arg;
	/** Old file path */
	char *old_path;
} rename_dlg_t;

/** Rename dialog callbacks */
typedef struct rename_dlg_cb {
	/** OK button was pressed */
	void (*bok)(rename_dlg_t *, void *, const char *);
	/** Cancel button was pressed */
	void (*bcancel)(rename_dlg_t *, void *);
	/** Window closure requested (e.g. via close button) */
	void (*close)(rename_dlg_t *, void *);
} rename_dlg_cb_t;

#endif

/** @}
 */
