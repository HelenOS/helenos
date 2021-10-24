/*
 * Copyright (c) 2021 Jiri Svoboda
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
