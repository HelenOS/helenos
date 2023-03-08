/*
 * Copyright (c) 2023 Jiri Svoboda
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
 * @file File list
 */

#ifndef _UI_FILELIST_H
#define _UI_FILELIST_H

#include <errno.h>
#include <gfx/coord.h>
#include <ui/control.h>
#include <ui/window.h>
#include <stdbool.h>
#include <types/ui/filelist.h>

extern errno_t ui_file_list_create(ui_window_t *, bool, ui_file_list_t **);
extern void ui_file_list_destroy(ui_file_list_t *);
extern ui_control_t *ui_file_list_ctl(ui_file_list_t *);
extern void ui_file_list_set_cb(ui_file_list_t *, ui_file_list_cb_t *, void *);
extern void ui_file_list_set_rect(ui_file_list_t *, gfx_rect_t *);
extern errno_t ui_file_list_read_dir(ui_file_list_t *, const char *);
extern errno_t ui_file_list_activate(ui_file_list_t *);
extern void ui_file_list_deactivate(ui_file_list_t *);
extern errno_t ui_file_list_open(ui_file_list_t *, ui_file_list_entry_t *);
extern ui_file_list_entry_t *ui_file_list_get_cursor(ui_file_list_t *);

#endif

/** @}
 */
