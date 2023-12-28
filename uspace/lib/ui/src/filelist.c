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
/** @file File list control.
 *
 * Displays a file listing.
 */

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <ui/control.h>
#include <ui/filelist.h>
#include <ui/list.h>
#include <ui/resource.h>
#include <vfs/vfs.h>
#include <qsort.h>
#include "../private/filelist.h"
#include "../private/list.h"
#include "../private/resource.h"

static void ui_file_list_ctl_destroy(void *);
static errno_t ui_file_list_ctl_paint(void *);
static ui_evclaim_t ui_file_list_ctl_kbd_event(void *, kbd_event_t *);
static ui_evclaim_t ui_file_list_ctl_pos_event(void *, pos_event_t *);

/** List control ops */
ui_control_ops_t ui_file_list_ctl_ops = {
	.destroy = ui_file_list_ctl_destroy,
	.paint = ui_file_list_ctl_paint,
	.kbd_event = ui_file_list_ctl_kbd_event,
	.pos_event = ui_file_list_ctl_pos_event
};

static void ui_file_list_list_activate_req(ui_list_t *, void *);
static void ui_file_list_list_selected(ui_list_entry_t *, void *);

/** List callbacks */
ui_list_cb_t ui_file_list_list_cb = {
	.activate_req = ui_file_list_list_activate_req,
	.selected = ui_file_list_list_selected,
	.compare = ui_file_list_list_compare,
};

/** Create file list.
 *
 * @param window Containing window
 * @param active @c true iff file list should be active
 * @param rflist Place to store pointer to new file list
 * @return EOK on success or an error code
 */
errno_t ui_file_list_create(ui_window_t *window, bool active,
    ui_file_list_t **rflist)
{
	ui_file_list_t *flist;
	errno_t rc;

	flist = calloc(1, sizeof(ui_file_list_t));
	if (flist == NULL)
		return ENOMEM;

	rc = ui_control_new(&ui_file_list_ctl_ops, (void *)flist,
	    &flist->control);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x0f, &flist->dir_color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x0a, &flist->svc_color);
	if (rc != EOK)
		goto error;

	rc = ui_list_create(window, active, &flist->list);
	if (rc != EOK)
		goto error;

	ui_list_set_cb(flist->list, &ui_file_list_list_cb, (void *)flist);

	flist->window = window;
	*rflist = flist;
	return EOK;
error:
	ui_control_delete(flist->control);
	if (flist->dir_color != NULL)
		gfx_color_delete(flist->dir_color);
	if (flist->svc_color != NULL)
		gfx_color_delete(flist->svc_color);
	free(flist);
	return rc;
}

/** Destroy file list.
 *
 * @param flist File list
 */
void ui_file_list_destroy(ui_file_list_t *flist)
{
	ui_file_list_clear_entries(flist);
	ui_list_destroy(flist->list);
	ui_control_delete(flist->control);
	free(flist);
}

/** Set file list callbacks.
 *
 * @param flist File list
 * @param cb Callbacks
 * @param arg Argument to callback functions
 */
void ui_file_list_set_cb(ui_file_list_t *flist, ui_file_list_cb_t *cb, void *arg)
{
	flist->cb = cb;
	flist->cb_arg = arg;
}

/** Get base control for file list.
 *
 * @param flist File list
 * @return Base UI control
 */
ui_control_t *ui_file_list_ctl(ui_file_list_t *flist)
{
	return flist->control;
}

/** Set file list rectangle.
 *
 * @param flist File list
 * @param rect Rectangle
 */
void ui_file_list_set_rect(ui_file_list_t *flist, gfx_rect_t *rect)
{
	ui_list_set_rect(flist->list, rect);
}

/** Determine if file list is active.
 *
 * @param flist File list
 * @return @c true iff file list is active
 */
bool ui_file_list_is_active(ui_file_list_t *flist)
{
	return ui_list_is_active(flist->list);
}

/** Activate file list.
 *
 * @param flist File list
 *
 * @return EOK on success or an error code
 */
errno_t ui_file_list_activate(ui_file_list_t *flist)
{
	errno_t rc;

	if (flist->dir != NULL) {
		rc = vfs_cwd_set(flist->dir);
		if (rc != EOK)
			return rc;
	}

	return ui_list_activate(flist->list);
}

/** Deactivate file list.
 *
 * @param flist File list
 */
void ui_file_list_deactivate(ui_file_list_t *flist)
{
	ui_list_deactivate(flist->list);
}

/** Initialize file list entry attributes.
 *
 * @param attr Attributes
 */
void ui_file_list_entry_attr_init(ui_file_list_entry_attr_t *attr)
{
	memset(attr, 0, sizeof(*attr));
}

/** Append new file list entry.
 *
 * @param flist File list
 * @param attr Entry attributes
 * @return EOK on success or an error code
 */
errno_t ui_file_list_entry_append(ui_file_list_t *flist, ui_file_list_entry_attr_t *attr)
{
	ui_file_list_entry_t *entry;
	ui_list_entry_attr_t lattr;
	ui_list_entry_t *lentry;
	ui_resource_t *res;
	char *caption;
	errno_t rc;
	int rv;

	res = ui_window_get_res(flist->window);

	entry = calloc(1, sizeof(ui_file_list_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->flist = flist;
	entry->name = str_dup(attr->name);
	if (entry->name == NULL) {
		free(entry);
		return ENOMEM;
	}

	entry->size = attr->size;
	entry->isdir = attr->isdir;
	entry->svc = attr->svc;

	if (attr->isdir && !res->textmode) {
		rv = asprintf(&caption, "[%s]", attr->name);
		if (rv < 0)
			caption = NULL;
	} else {
		caption = str_dup(attr->name);
	}

	if (caption == NULL) {
		free(entry->name);
		free(entry);
		return ENOMEM;
	}

	lattr.caption = caption;
	lattr.arg = (void *)entry;
	lattr.color = NULL;
	lattr.bgcolor = NULL;

	if (res->textmode) {
		if (attr->isdir) {
			lattr.color = flist->dir_color;
			lattr.bgcolor = flist->dir_color;
		} else if (attr->svc != 0) {
			lattr.color = flist->svc_color;
			lattr.bgcolor = flist->svc_color;
		}
	}

	rc = ui_list_entry_append(flist->list, &lattr, &lentry);
	if (rc != EOK) {
		free(caption);
		free(entry->name);
		free(entry);
		return rc;
	}

	free(caption);
	entry->entry = lentry;
	return EOK;
}

/** Delete file list entry.
 *
 * @param entry File list entry
 */
void ui_file_list_entry_destroy(ui_file_list_entry_t *entry)
{
	ui_list_entry_destroy(entry->entry);
	free(entry->name);
	free(entry);
}

/** Clear file list entry list.
 *
 * @param flist File list
 */
void ui_file_list_clear_entries(ui_file_list_t *flist)
{
	ui_file_list_entry_t *entry;

	entry = ui_file_list_first(flist);
	while (entry != NULL) {
		ui_file_list_entry_destroy(entry);
		entry = ui_file_list_first(flist);
	}
}

/** Read directory into file list entry list.
 *
 * @param flist File list
 * @param dirname Directory path
 * @return EOK on success or an error code
 */
errno_t ui_file_list_read_dir(ui_file_list_t *flist, const char *dirname)
{
	DIR *dir;
	struct dirent *dirent;
	vfs_stat_t finfo;
	char newdir[256];
	char *ndir = NULL;
	ui_file_list_entry_attr_t attr;
	ui_file_list_entry_t *cur;
	ui_file_list_entry_t *next;
	char *olddn;
	errno_t rc;

	rc = vfs_cwd_set(dirname);
	if (rc != EOK)
		return rc;

	rc = vfs_cwd_get(newdir, sizeof(newdir));
	if (rc != EOK)
		return rc;

	ndir = str_dup(newdir);
	if (ndir == NULL)
		return ENOMEM;

	dir = opendir(".");
	if (dir == NULL) {
		rc = errno;
		goto error;
	}

	if (str_cmp(ndir, "/") != 0) {
		/* Need to add a synthetic up-dir entry */
		ui_file_list_entry_attr_init(&attr);
		attr.name = "..";
		attr.isdir = true;

		rc = ui_file_list_entry_append(flist, &attr);
		if (rc != EOK)
			goto error;
	}

	dirent = readdir(dir);
	while (dirent != NULL) {
		rc = vfs_stat_path(dirent->d_name, &finfo);
		if (rc != EOK) {
			/* Possibly a stale entry */
			dirent = readdir(dir);
			continue;
		}

		ui_file_list_entry_attr_init(&attr);
		attr.name = dirent->d_name;
		attr.size = finfo.size;
		attr.isdir = finfo.is_directory;
		attr.svc = finfo.service;

		rc = ui_file_list_entry_append(flist, &attr);
		if (rc != EOK)
			goto error;

		dirent = readdir(dir);
	}

	closedir(dir);

	rc = ui_file_list_sort(flist);
	if (rc != EOK)
		goto error;

	/* Moving up? */
	if (str_cmp(dirname, "..") == 0) {
		/* Get the last component of old path */
		olddn = str_rchr(flist->dir, '/');
		if (olddn != NULL && *olddn != '\0') {
			/* Find corresponding entry */
			++olddn;
			cur = ui_file_list_first(flist);
			next = ui_file_list_next(cur);
			while (next != NULL && str_cmp(next->name, olddn) <= 0 &&
			    next->isdir) {
				cur = next;
				next = ui_file_list_next(cur);
			}

			/* Center on the entry */
			ui_list_cursor_center(flist->list, cur->entry);
		}
	}

	free(flist->dir);
	flist->dir = ndir;

	return EOK;
error:
	(void) vfs_cwd_set(flist->dir);
	if (ndir != NULL)
		free(ndir);
	if (dir != NULL)
		closedir(dir);
	return rc;
}

/** Sort file list entries.
 *
 * @param flist File list
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ui_file_list_sort(ui_file_list_t *flist)
{
	return ui_list_sort(flist->list);
}

/** Compare two list entries within file list entries.
 *
 * @param ea First UI list entry
 * @param eb Second UI list entry
 * @return <0, =0, >=0 if a < b, a == b, a > b, resp.
 */
int ui_file_list_list_compare(ui_list_entry_t *ea, ui_list_entry_t *eb)
{
	ui_file_list_entry_t *a;
	ui_file_list_entry_t *b;
	int dcmp;

	a = (ui_file_list_entry_t *)ui_list_entry_get_arg(ea);
	b = (ui_file_list_entry_t *)ui_list_entry_get_arg(eb);

	/* Sort directories first */
	dcmp = b->isdir - a->isdir;
	if (dcmp != 0)
		return dcmp;

	return str_cmp(a->name, b->name);
}

/** Return first file list entry.
 *
 * @param flist File list
 * @return First file list entry or @c NULL if there are no entries
 */
ui_file_list_entry_t *ui_file_list_first(ui_file_list_t *flist)
{
	ui_list_entry_t *lentry;

	lentry = ui_list_first(flist->list);
	if (lentry == NULL)
		return NULL;

	return (ui_file_list_entry_t *)ui_list_entry_get_arg(lentry);
}

/** Return last file list entry.
 *
 * @param flist File list
 * @return Last file list entry or @c NULL if there are no entries
 */
ui_file_list_entry_t *ui_file_list_last(ui_file_list_t *flist)
{
	ui_list_entry_t *lentry;

	lentry = ui_list_last(flist->list);
	if (lentry == NULL)
		return NULL;

	return (ui_file_list_entry_t *)ui_list_entry_get_arg(lentry);
}

/** Return next file list entry.
 *
 * @param cur Current entry
 * @return Next entry or @c NULL if @a cur is the last entry
 */
ui_file_list_entry_t *ui_file_list_next(ui_file_list_entry_t *cur)
{
	ui_list_entry_t *lentry;

	lentry = ui_list_next(cur->entry);
	if (lentry == NULL)
		return NULL;

	return (ui_file_list_entry_t *)ui_list_entry_get_arg(lentry);
}

/** Return previous file list entry.
 *
 * @param cur Current entry
 * @return Previous entry or @c NULL if @a cur is the first entry
 */
ui_file_list_entry_t *ui_file_list_prev(ui_file_list_entry_t *cur)
{
	ui_list_entry_t *lentry;

	lentry = ui_list_prev(cur->entry);
	if (lentry == NULL)
		return NULL;

	return (ui_file_list_entry_t *)ui_list_entry_get_arg(lentry);
}

/** Get entry under cursor.
 *
 * @param flist File list
 * @return Current cursor
 */
ui_file_list_entry_t *ui_file_list_get_cursor(ui_file_list_t *flist)
{
	ui_list_entry_t *entry;

	entry = ui_list_get_cursor(flist->list);
	if (entry == NULL)
		return NULL;

	return (ui_file_list_entry_t *)ui_list_entry_get_arg(entry);
}

/** Open file list entry.
 *
 * Perform Open action on a file list entry (e.g. switch to a subdirectory).
 *
 * @param flist File list
 * @param entry File list entry
 *
 * @return EOK on success or an error code
 */
errno_t ui_file_list_open(ui_file_list_t *flist, ui_file_list_entry_t *entry)
{
	if (entry->isdir)
		return ui_file_list_open_dir(flist, entry);
	else if (entry->svc == 0)
		return ui_file_list_open_file(flist, entry);
	else
		return EOK;
}

/** Open file list directory entry.
 *
 * Perform Open action on a directory entry (i.e. switch to the directory).
 *
 * @param flist File list
 * @param entry File list entry (which is a directory)
 *
 * @return EOK on success or an error code
 */
errno_t ui_file_list_open_dir(ui_file_list_t *flist,
    ui_file_list_entry_t *entry)
{
	char *dirname;
	errno_t rc;

	assert(entry->isdir);

	/*
	 * Need to copy out name before we free the entry below
	 * via ui_file_list_clear_entries().
	 */
	dirname = str_dup(entry->name);
	if (dirname == NULL)
		return ENOMEM;

	ui_file_list_clear_entries(flist);

	rc = ui_file_list_read_dir(flist, dirname);
	if (rc != EOK) {
		free(dirname);
		return rc;
	}

	free(dirname);

	rc = ui_file_list_paint(flist);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Open file list file entry.
 *
 * Perform Open action on a file entry (i.e. try running it).
 *
 * @param flist File list
 * @param entry File list entry (which is a file)
 *
 * @return EOK on success or an error code
 */
errno_t ui_file_list_open_file(ui_file_list_t *flist,
    ui_file_list_entry_t *entry)
{
	ui_file_list_selected(flist, entry->name);
	return EOK;
}

/** Request file list activation.
 *
 * Call back to request file list activation.
 *
 * @param flist File list
 */
void ui_file_list_activate_req(ui_file_list_t *flist)
{
	if (flist->cb != NULL && flist->cb->activate_req != NULL)
		flist->cb->activate_req(flist, flist->cb_arg);
}

/** Call back when a file is selected.
 *
 * @param flist File list
 * @param fname File name
 */
void ui_file_list_selected(ui_file_list_t *flist, const char *fname)
{
	if (flist->cb != NULL && flist->cb->selected != NULL)
		flist->cb->selected(flist, flist->cb_arg, fname);
}

/** Paint file list.
 *
 * @param flist File list
 * @return EOK on success or an error code.
 */
errno_t ui_file_list_paint(ui_file_list_t *flist)
{
	return ui_control_paint(ui_list_ctl(flist->list));
}

/** Destroy file list control.
 *
 * @param arg Argument (ui_list_t *)
 */
void ui_file_list_ctl_destroy(void *arg)
{
	ui_file_list_t *flist = (ui_file_list_t *) arg;

	ui_file_list_destroy(flist);
}

/** Paint file list control.
 *
 * @param arg Argument (ui_file_list_t *)
 * @return EOK on success or an error code
 */
errno_t ui_file_list_ctl_paint(void *arg)
{
	ui_file_list_t *flist = (ui_file_list_t *) arg;

	return ui_file_list_paint(flist);
}

/** Handle file list control keyboard event.
 *
 * @param arg Argument (ui_file_list_t *)
 * @param kbd_event Keyboard event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_file_list_ctl_kbd_event(void *arg, kbd_event_t *event)
{
	ui_file_list_t *flist = (ui_file_list_t *) arg;

	return ui_control_kbd_event(ui_list_ctl(flist->list), event);
}

/** Handle file list control position event.
 *
 * @param arg Argument (ui_file_list_t *)
 * @param pos_event Position event
 * @return @c ui_claimed iff the event is claimed
 */
ui_evclaim_t ui_file_list_ctl_pos_event(void *arg, pos_event_t *event)
{
	ui_file_list_t *flist = (ui_file_list_t *) arg;

	return ui_control_pos_event(ui_list_ctl(flist->list), event);
}

/** Activate request callback handler for UI list within file list.
 *
 * @param list UI list
 * @param arg Argument (File list)
 */
static void ui_file_list_list_activate_req(ui_list_t *list, void *arg)
{
	ui_file_list_t *flist = (ui_file_list_t *)arg;

	ui_file_list_activate_req(flist);
}

/** Entry selected callback handler for UI list within file list.
 *
 * @param entr Activated UI list entry
 * @param arg Argument (File list entry)
 */
static void ui_file_list_list_selected(ui_list_entry_t *entry, void *arg)
{
	ui_file_list_entry_t *fentry = (void *)arg;

	(void) ui_file_list_open(fentry->flist, fentry);
}

/** @}
 */
