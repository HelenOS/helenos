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
 * @file Progress dialog
 */

#ifndef TYPES_DLG_PROGRESS_H
#define TYPES_DLG_PROGRESS_H

#include <errno.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/window.h>

/** Progress dialog */
typedef struct progress_dlg {
	/** Dialog window */
	ui_window_t *window;
	/** Label with current file progress */
	ui_label_t *lcurf_prog;
	/** Abort button */
	ui_pbutton_t *babort;
	/** New file dialog callbacks */
	struct progress_dlg_cb *cb;
	/** Callback argument */
	void *arg;
} progress_dlg_t;

/** Progress dialog callbacks */
typedef struct progress_dlg_cb {
	/** Abort button was pressed */
	void (*babort)(progress_dlg_t *, void *);
	/** Window closure requested (e.g. via close button) */
	void (*close)(progress_dlg_t *, void *);
} progress_dlg_cb_t;

/** Progress dialog parameters */
typedef struct {
	/** Window caption */
	const char *caption;
} progress_dlg_params_t;

#endif

/** @}
 */
