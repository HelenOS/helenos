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
 * @file Navigator types
 */

#ifndef TYPES_NAV_H
#define TYPES_NAV_H

#include <fibril.h>
#include <fmgt.h>
#include <stdbool.h>
#include <ui/fixed.h>
#include <ui/ui.h>
#include <ui/window.h>

enum {
	navigator_panels = 2
};

/** Navigator */
typedef struct navigator {
	/** User interface */
	ui_t *ui;
	/** Window */
	ui_window_t *window;
	/** Fixed layout */
	ui_fixed_t *fixed;
	/** Menu */
	struct nav_menu *menu;
	/** Panels */
	struct panel *panel[navigator_panels];
	/** Progress dialog */
	struct progress_dlg *progress_dlg;
	/** Worker fibril ID */
	fid_t worker_fid;
	/** Abort current file management operation */
	bool abort_op;

	/** @c true if user selected I/O error recovery action */
	bool io_err_act_sel;
	/** Selected I/O error recovery action */
	fmgt_error_action_t io_err_act;
	/** Signalled when user selects I/O error recovery action */
	fibril_condvar_t io_err_act_cv;
	/** Synchronizes access to I/O error recovery action */
	fibril_mutex_t io_err_act_lock;
} navigator_t;

/** Navigator worker job */
typedef struct {
	/** Navigator */
	navigator_t *navigator;
	/** Worker function */
	void (*wfunc)(void *);
	/** Worker argument */
	void *arg;
} navigator_worker_job_t;

#endif

/** @}
 */
