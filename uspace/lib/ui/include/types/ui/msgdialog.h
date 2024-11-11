/*
 * Copyright (c) 2024 Jiri Svoboda
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
 * @file Message Dialog
 */

#ifndef _UI_TYPES_MSGDIALOG_H
#define _UI_TYPES_MSGDIALOG_H

#include <errno.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <types/ui/wdecor.h>

struct ui_msg_dialog;
typedef struct ui_msg_dialog ui_msg_dialog_t;

enum {
	/** Maximum number of buttons in message dialog. */
	ui_msg_dialog_maxbtn = 2
};

/** Which choices the user can select from. */
typedef enum {
	/** OK (the default) */
	umdc_ok,
	/** OK, Cancel */
	umdc_ok_cancel
} ui_msg_dialog_choice_t;

/** Message dialog parameters */
typedef struct {
	/** Window caption */
	const char *caption;
	/** Message text */
	const char *text;
	/** The choice that the user is given */
	ui_msg_dialog_choice_t choice;
} ui_msg_dialog_params_t;

/** Message dialog callback */
typedef struct ui_msg_dialog_cb {
	/** Dialog button was pressed */
	void (*button)(ui_msg_dialog_t *, void *, unsigned);
	/** Window closure requested (e.g. via close button) */
	void (*close)(ui_msg_dialog_t *, void *);
} ui_msg_dialog_cb_t;

#endif

/** @}
 */
