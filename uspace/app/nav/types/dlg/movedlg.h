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
 * @file Move dialog
 */

#ifndef TYPES_DLG_MOVEDLG_H
#define TYPES_DLG_MOVEDLG_H

#include <errno.h>
#include <fmgt.h>
#include <ui/entry.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/window.h>
#include <stdbool.h>

/** Move dialog */
typedef struct move_dlg {
	/** Dialog window */
	ui_window_t *window;
	/** "Move 'xxx'" */
	ui_label_t *lmove;
	/** File */
	ui_entry_t *esize;
	/** Destination text entry */
	ui_entry_t *edest;
	/** OK button */
	ui_pbutton_t *bok;
	/** Cancel button */
	ui_pbutton_t *bcancel;
	/** Move dialog callbacks */
	struct move_dlg_cb *cb;
	/** Callback argument */
	void *arg;
	/** File list */
	fmgt_flist_t *flist;
} move_dlg_t;

/** Move dialog callbacks */
typedef struct move_dlg_cb {
	/** OK button was pressed */
	void (*bok)(move_dlg_t *, void *);
	/** Cancel button was pressed */
	void (*bcancel)(move_dlg_t *, void *);
	/** Window closure requested (e.g. via close button) */
	void (*close)(move_dlg_t *, void *);
} move_dlg_cb_t;

#endif

/** @}
 */
