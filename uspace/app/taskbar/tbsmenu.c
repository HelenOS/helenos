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

/** @addtogroup taskbar
 * @{
 */
/** @file Taskbar start menu
 */

#include <gfx/coord.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <tbarcfg/tbarcfg.h>
#include <ui/fixed.h>
#include <ui/menu.h>
#include <ui/menuentry.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "tbsmenu.h"

static void tbsmenu_smenu_close_req(ui_menu_t *, void *);

/** Start menu callbacks */
static ui_menu_cb_t tbsmenu_smenu_cb = {
	.close_req = tbsmenu_smenu_close_req,
};

static void tbsmenu_button_down(ui_pbutton_t *, void *);

/** Start button callbacks */
static ui_pbutton_cb_t tbsmenu_button_cb = {
	.down = tbsmenu_button_down
};

static void tbsmenu_smenu_entry_cb(ui_menu_entry_t *, void *);
static errno_t tbsmenu_entry_start(tbsmenu_entry_t *);
static void tbsmenu_cmd_fini(tbsmenu_cmd_t *);

/** Create taskbar start menu.
 *
 * @param window Containing window
 * @param fixed Fixed layout to which start button will be added
 * @param dspec Display specification (for passing to applications)
 * @param rtbsmenu Place to store pointer to new start menu
 * @return @c EOK on success or an error code
 */
errno_t tbsmenu_create(ui_window_t *window, ui_fixed_t *fixed,
    const char *dspec, tbsmenu_t **rtbsmenu)
{
	ui_resource_t *res = ui_window_get_res(window);
	tbsmenu_t *tbsmenu = NULL;
	errno_t rc;

	tbsmenu = calloc(1, sizeof(tbsmenu_t));
	if (tbsmenu == NULL) {
		rc = ENOMEM;
		goto error;
	}

	tbsmenu->display_spec = str_dup(dspec);
	if (tbsmenu->display_spec == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = ui_pbutton_create(res, "Start", &tbsmenu->sbutton);
	if (rc != EOK)
		goto error;

	ui_pbutton_set_cb(tbsmenu->sbutton, &tbsmenu_button_cb,
	    (void *)tbsmenu);

	ui_pbutton_set_default(tbsmenu->sbutton, true);

	rc = ui_fixed_add(fixed, ui_pbutton_ctl(tbsmenu->sbutton));
	if (rc != EOK)
		goto error;

	rc = ui_menu_create(window, &tbsmenu->smenu);
	if (rc != EOK)
		goto error;

	ui_menu_set_cb(tbsmenu->smenu, &tbsmenu_smenu_cb, (void *)tbsmenu);

	tbsmenu->window = window;
	tbsmenu->fixed = fixed;
	list_initialize(&tbsmenu->entries);

	*rtbsmenu = tbsmenu;
	return EOK;
error:
	if (tbsmenu != NULL && tbsmenu->display_spec != NULL)
		free(tbsmenu->display_spec);
	if (tbsmenu != NULL)
		ui_pbutton_destroy(tbsmenu->sbutton);
	if (tbsmenu != NULL)
		free(tbsmenu);
	return rc;
}

/** Load start menu from repository.
 *
 * @param tbsmenu Start menu
 * @param Repository path
 * @return EOK on success or an error code
 */
errno_t tbsmenu_load(tbsmenu_t *tbsmenu, const char *repopath)
{
	tbsmenu_entry_t *tentry;
	tbarcfg_t *tbcfg = NULL;
	smenu_entry_t *sme;
	bool separator;
	const char *caption;
	const char *cmd;
	bool terminal;
	errno_t rc;

	if (tbsmenu->repopath != NULL)
		free(tbsmenu->repopath);

	tbsmenu->repopath = str_dup(repopath);
	if (tbsmenu->repopath == NULL)
		return ENOMEM;

	/* Remove existing entries */
	tentry = tbsmenu_first(tbsmenu);
	while (tentry != NULL) {
		tbsmenu_remove(tbsmenu, tentry, false);
		tentry = tbsmenu_first(tbsmenu);
	}

	rc = tbarcfg_open(repopath, &tbcfg);
	if (rc != EOK)
		goto error;

	sme = tbarcfg_smenu_first(tbcfg);
	while (sme != NULL) {
		separator = smenu_entry_get_separator(sme);
		if (separator == false) {
			caption = smenu_entry_get_caption(sme);
			cmd = smenu_entry_get_cmd(sme);
			terminal = smenu_entry_get_terminal(sme);

			rc = tbsmenu_add(tbsmenu, caption, cmd, terminal,
			    &tentry);
			if (rc != EOK)
				goto error;
		} else {
			rc = tbsmenu_add_sep(tbsmenu, &tentry);
			if (rc != EOK)
				goto error;
		}

		(void)tentry;

		sme = tbarcfg_smenu_next(sme);
	}

	tbarcfg_close(tbcfg);
	return EOK;
error:
	if (tbcfg != NULL)
		tbarcfg_close(tbcfg);
	return rc;
}

/** Reload start menu from repository (or schedule reload).
 *
 * @param tbsmenu Start menu
 */
void tbsmenu_reload(tbsmenu_t *tbsmenu)
{
	if (!tbsmenu_is_open(tbsmenu))
		(void) tbsmenu_load(tbsmenu, tbsmenu->repopath);
	else
		tbsmenu->needs_reload = true;
}

/** Set start menu rectangle.
 *
 * @param tbsmenu Start menu
 * @param rect Rectangle
 */
void tbsmenu_set_rect(tbsmenu_t *tbsmenu, gfx_rect_t *rect)
{
	tbsmenu->rect = *rect;
	ui_pbutton_set_rect(tbsmenu->sbutton, rect);
}

/** Open taskbar start menu.
 *
 * @param tbsmenu Start menu
 */
void tbsmenu_open(tbsmenu_t *tbsmenu)
{
	(void) ui_menu_open(tbsmenu->smenu, &tbsmenu->rect,
	    tbsmenu->ev_idev_id);
}

/** Close taskbar start menu.
 *
 * @param tbsmenu Start menu
 */
void tbsmenu_close(tbsmenu_t *tbsmenu)
{
	ui_menu_close(tbsmenu->smenu);

	if (tbsmenu->needs_reload)
		(void) tbsmenu_load(tbsmenu, tbsmenu->repopath);
}

/** Determine if taskbar start menu is open.
 *
 * @param tbsmenu Start menu
 * @return @c true iff start menu is open
 */
bool tbsmenu_is_open(tbsmenu_t *tbsmenu)
{
	return ui_menu_is_open(tbsmenu->smenu);
}

/** Destroy taskbar start menu.
 *
 * @param tbsmenu Start menu
 */
void tbsmenu_destroy(tbsmenu_t *tbsmenu)
{
	tbsmenu_entry_t *entry;

	/* Destroy entries */
	entry = tbsmenu_first(tbsmenu);
	while (entry != NULL) {
		tbsmenu_remove(tbsmenu, entry, false);
		entry = tbsmenu_first(tbsmenu);
	}

	ui_fixed_remove(tbsmenu->fixed, ui_pbutton_ctl(tbsmenu->sbutton));
	ui_pbutton_destroy(tbsmenu->sbutton);
	ui_menu_destroy(tbsmenu->smenu);

	free(tbsmenu);
}

/** Add entry to start menu.
 *
 * @param tbsmenu Start menu
 * @param caption Caption
 * @param cmd Command to run
 * @param terminal Start in terminal
 * @param entry Start menu entry
 * @return @c EOK on success or an error code
 */
errno_t tbsmenu_add(tbsmenu_t *tbsmenu, const char *caption,
    const char *cmd, bool terminal, tbsmenu_entry_t **rentry)
{
	errno_t rc;
	tbsmenu_entry_t *entry;

	entry = calloc(1, sizeof(tbsmenu_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->caption = str_dup(caption);
	if (entry->caption == NULL) {
		rc = ENOMEM;
		goto error;
	}

	entry->cmd = str_dup(cmd);
	if (entry->cmd == NULL) {
		rc = ENOMEM;
		goto error;
	}

	entry->terminal = terminal;

	rc = ui_menu_entry_create(tbsmenu->smenu, caption, "", &entry->mentry);
	if (rc != EOK)
		goto error;

	ui_menu_entry_set_cb(entry->mentry, tbsmenu_smenu_entry_cb,
	    (void *)entry);

	entry->tbsmenu = tbsmenu;
	list_append(&entry->lentries, &tbsmenu->entries);
	*rentry = entry;
	return EOK;
error:
	if (entry->caption != NULL)
		free(entry->caption);
	if (entry->cmd != NULL)
		free(entry->cmd);
	free(entry);
	return rc;
}

/** Add separator entry to start menu.
 *
 * @param tbsmenu Start menu
 * @param entry Start menu entry
 * @return @c EOK on success or an error code
 */
errno_t tbsmenu_add_sep(tbsmenu_t *tbsmenu, tbsmenu_entry_t **rentry)
{
	errno_t rc;
	tbsmenu_entry_t *entry;

	entry = calloc(1, sizeof(tbsmenu_entry_t));
	if (entry == NULL)
		return ENOMEM;

	rc = ui_menu_entry_sep_create(tbsmenu->smenu, &entry->mentry);
	if (rc != EOK)
		goto error;

	ui_menu_entry_set_cb(entry->mentry, tbsmenu_smenu_entry_cb,
	    (void *)entry);

	entry->tbsmenu = tbsmenu;
	list_append(&entry->lentries, &tbsmenu->entries);
	*rentry = entry;
	return EOK;
error:
	free(entry);
	return rc;
}

/** Remove entry from start menu.
 *
 * @param tbsmenu Start menu
 * @param entry Start menu entry
 * @param paint @c true to repaint start menu
 */
void tbsmenu_remove(tbsmenu_t *tbsmenu, tbsmenu_entry_t *entry,
    bool paint)
{
	assert(entry->tbsmenu == tbsmenu);

	list_remove(&entry->lentries);

	ui_menu_entry_destroy(entry->mentry);
	free(entry->caption);
	free(entry->cmd);
	free(entry);
}

/** Handle start menu close request.
 *
 * @param menu Menu
 * @param arg Argument (tbsmenu_t *)
 * @param wnd_id Window ID
 */
static void tbsmenu_smenu_close_req(ui_menu_t *menu, void *arg)
{
	tbsmenu_t *tbsmenu = (tbsmenu_t *)arg;

	(void)tbsmenu;
	ui_menu_close(menu);
}

/** Start menu entry was activated.
 *
 * @param smentry Start menu entry
 * @param arg Argument (tbsmenu_entry_t *)
 */
static void tbsmenu_smenu_entry_cb(ui_menu_entry_t *smentry, void *arg)
{
	tbsmenu_entry_t *entry = (tbsmenu_entry_t *)arg;

	(void)tbsmenu_entry_start(entry);
}

/** Get first start menu entry.
 *
 * @param tbsmenu Start menu
 * @return First entry or @c NULL if the menu is empty
 */
tbsmenu_entry_t *tbsmenu_first(tbsmenu_t *tbsmenu)
{
	link_t *link;

	link = list_first(&tbsmenu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tbsmenu_entry_t, lentries);
}

/** Get last start menu entry.
 *
 * @param tbsmenu Start menu
 * @return Last entry or @c NULL if the menu is empty
 */
tbsmenu_entry_t *tbsmenu_last(tbsmenu_t *tbsmenu)
{
	link_t *link;

	link = list_last(&tbsmenu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tbsmenu_entry_t, lentries);
}

/** Get next start menu entry.
 *
 * @param cur Current entry
 * @return Next entry or @c NULL if @a cur is the last entry
 */
tbsmenu_entry_t *tbsmenu_next(tbsmenu_entry_t *cur)
{
	link_t *link;

	link = list_next(&cur->lentries, &cur->tbsmenu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, tbsmenu_entry_t, lentries);
}

/** Get number of start menu entries.
 *
 * @param tbsmenu Start menu
 * @return Number of entries
 */
size_t tbsmenu_count(tbsmenu_t *tbsmenu)
{
	return list_count(&tbsmenu->entries);
}

/** Start button was depressed.
 *
 * @param pbutton Push button
 * @param arg Argument (tbsmenu_entry_t *)
 */
static void tbsmenu_button_down(ui_pbutton_t *pbutton, void *arg)
{
	tbsmenu_t *tbsmenu = (tbsmenu_t *)arg;

	if (!tbsmenu_is_open(tbsmenu)) {
		tbsmenu_open(tbsmenu);
	} else {
		/* menu is open */
		tbsmenu_close(tbsmenu);
	}
}

/** Split command string into individual parts.
 *
 * Command arguments are separated by spaces. There is no way
 * to provide an argument containing spaces.
 *
 * @param str Command with arguments separated by spaces
 * @param cmd Command structure to fill in
 * @return EOK on success or an error code
 */
static errno_t tbsmenu_cmd_split(const char *str, tbsmenu_cmd_t *cmd)
{
	char *buf;
	char *arg;
	char *next;
	size_t cnt;

	buf = str_dup(str);
	if (buf == NULL)
		return ENOMEM;

	/* Count the entries */
	cnt = 0;
	arg = str_tok(buf, " ", &next);
	while (arg != NULL) {
		++cnt;
		arg = str_tok(next, " ", &next);
	}

	/* Need to copy again as buf was mangled */
	free(buf);
	buf = str_dup(str);
	if (buf == NULL)
		return ENOMEM;

	cmd->argv = calloc(cnt + 1, sizeof(char *));
	if (cmd->argv == NULL) {
		free(buf);
		return ENOMEM;
	}

	/* Copy individual arguments */
	cnt = 0;
	arg = str_tok(buf, " ", &next);
	while (arg != NULL) {
		cmd->argv[cnt] = str_dup(arg);
		if (cmd->argv[cnt] == NULL) {
			tbsmenu_cmd_fini(cmd);
			return ENOMEM;
		}
		++cnt;

		arg = str_tok(next, " ", &next);
	}

	cmd->argv[cnt] = NULL;

	return EOK;
}

/** Free command structure.
 *
 * @param cmd Command
 */
static void tbsmenu_cmd_fini(tbsmenu_cmd_t *cmd)
{
	char **cp;

	/* Free all pointers in NULL-terminated list */
	cp = cmd->argv;
	while (*cp != NULL) {
		free(*cp);
		++cp;
	}

	/* Free the array of pointers */
	free(cmd->argv);
}

/** Free command structure.
 *
 * @param cmd Command
 * @param entry Start menu entry
 * @param dspec Display specification
 * @return EOK on success or an error code
 */
static errno_t tbsmenu_cmd_subst(tbsmenu_cmd_t *cmd, tbsmenu_entry_t *entry,
    const char *dspec)
{
	char **cp;

	(void)entry;

	/* Walk NULL-terminated list of arguments */
	cp = cmd->argv;
	while (*cp != NULL) {
		if (str_cmp(*cp, "%d") == 0) {
			/* Display specification */
			free(*cp);
			*cp = str_dup(dspec);
			if (*cp == NULL)
				return ENOMEM;
		}
		++cp;
	}

	return EOK;
}

/** Execute start menu entry.
 *
 * @param entry Start menu entry
 *
 * @return EOK on success or an error code
 */
static errno_t tbsmenu_entry_start(tbsmenu_entry_t *entry)
{
	task_id_t id;
	task_wait_t wait;
	task_exit_t texit;
	tbsmenu_cmd_t cmd;
	int retval;
	bool suspended;
	int i;
	int cnt;
	char **cp;
	const char **targv = NULL;
	char *dspec = NULL;
	sysarg_t idev_id;
	int rv;
	errno_t rc;
	ui_t *ui;

	ui = ui_window_get_ui(entry->tbsmenu->window);
	suspended = false;

	idev_id = ui_menu_get_idev_id(entry->tbsmenu->smenu);

	rv = asprintf(&dspec, "%s?idev=%zu",
	    entry->tbsmenu->display_spec, (size_t)idev_id);
	if (rv < 0)
		return ENOMEM;

	/* Split command string into individual arguments */
	rc = tbsmenu_cmd_split(entry->cmd, &cmd);
	if (rc != EOK) {
		free(dspec);
		return rc;
	}

	/* Substitute metacharacters in command */
	rc = tbsmenu_cmd_subst(&cmd, entry, dspec);
	if (rc != EOK)
		goto error;

	/* Free up and clean console for the child task. */
	rc = ui_suspend(ui);
	if (rc != EOK)
		goto error;

	suspended = true;

	/* Don't start in terminal if not running in a window */
	if (entry->terminal && !ui_is_fullscreen(ui)) {
		cnt = 0;
		cp = cmd.argv;
		while (*cp != NULL) {
			++cnt;
			++cp;
		}

		targv = calloc(cnt + 5, sizeof(char **));
		if (targv == NULL)
			goto error;

		targv[0] = "/app/terminal";
		targv[1] = "-d";
		targv[2] = dspec;
		targv[3] = "-c";

		for (i = 0; i <= cnt; i++) {
			targv[4 + i] = cmd.argv[i];
		}

		rc = task_spawnv(&id, &wait, targv[0], targv);
		if (rc != EOK)
			goto error;

		free(targv);
		targv = NULL;
	} else {
		rc = task_spawnv(&id, &wait, cmd.argv[0], (const char *const *)
		    cmd.argv);
		if (rc != EOK)
			goto error;
	}

	rc = task_wait(&wait, &texit, &retval);
	if ((rc != EOK) || (texit != TASK_EXIT_NORMAL))
		goto error;

	/* Resume UI operation */
	rc = ui_resume(ui);
	if (rc != EOK)
		goto error;

	(void) ui_paint(ui);
	return EOK;
error:
	free(dspec);
	if (targv != NULL)
		free(targv);
	tbsmenu_cmd_fini(&cmd);
	if (suspended) {
		rc = ui_resume(ui);
		if (rc != EOK) {
			printf("Failed to resume UI.\n");
			exit(1);
		}
	}
	(void) ui_paint(ui);
	return rc;
}

/** @}
 */
