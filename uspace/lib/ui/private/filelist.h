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

#ifndef _UI_PRIVATE_FILELIST_H
#define _UI_PRIVATE_FILELIST_H

#include <gfx/color.h>
#include <ipc/loc.h>
#include <ui/list.h>
#include <ui/window.h>
#include <stdint.h>
#include <types/ui/filelist.h>

/** File list entry attributes */
struct ui_file_list_entry_attr {
	/** File name */
	const char *name;
	/** File size */
	uint64_t size;
	/** @c true iff entry is a directory */
	bool isdir;
	/** Service number for service special entries */
	service_id_t svc;
};

/** File list entry */
struct ui_file_list_entry {
	/** Containing file list */
	struct ui_file_list *flist;
	/** List entry */
	ui_list_entry_t *entry;
	/** File name */
	char *name;
	/** File size */
	uint64_t size;
	/** @c true iff entry is a directory */
	bool isdir;
	/** Service number for service special entries */
	service_id_t svc;
};

/** File list.
 *
 * Allows browsing files and directories.
 */
typedef struct ui_file_list {
	/** Base control object */
	struct ui_control *control;

	/** Containing window */
	struct ui_window *window;

	/** UI list */
	ui_list_t *list;

	/** Callbacks */
	struct ui_file_list_cb *cb;

	/** Callback argument */
	void *cb_arg;

	/** Directory-type entry color */
	gfx_color_t *dir_color;

	/** Service-type entry color */
	gfx_color_t *svc_color;

	/** Directory */
	char *dir;
} ui_file_list_t;

extern bool ui_file_list_is_active(ui_file_list_t *);
extern void ui_file_list_entry_destroy(ui_file_list_entry_t *);
extern void ui_file_list_clear_entries(ui_file_list_t *);
extern errno_t ui_file_list_sort(ui_file_list_t *);
extern int ui_file_list_entry_ptr_cmp(const void *, const void *);
extern void ui_file_list_entry_attr_init(ui_file_list_entry_attr_t *);
extern errno_t ui_file_list_entry_append(ui_file_list_t *,
    ui_file_list_entry_attr_t *);
extern ui_file_list_entry_t *ui_file_list_first(ui_file_list_t *);
extern ui_file_list_entry_t *ui_file_list_last(ui_file_list_t *);
extern ui_file_list_entry_t *ui_file_list_next(ui_file_list_entry_t *);
extern ui_file_list_entry_t *ui_file_list_prev(ui_file_list_entry_t *);
extern errno_t ui_file_list_open_dir(ui_file_list_t *, ui_file_list_entry_t *);
extern errno_t ui_file_list_open_file(ui_file_list_t *, ui_file_list_entry_t *);
extern void ui_file_list_activate_req(ui_file_list_t *);
extern void ui_file_list_selected(ui_file_list_t *, const char *);
extern errno_t ui_file_list_paint(ui_file_list_t *);
extern int ui_file_list_list_compare(ui_list_entry_t *, ui_list_entry_t *);

#endif

/** @}
 */
