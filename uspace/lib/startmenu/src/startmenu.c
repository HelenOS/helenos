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

/** @addtogroup libstartmenu
 * @{
 */
/**
 * @file Start menu
 */

#include <errno.h>
#include <sif.h>
#include <startmenu/startmenu.h>
#include <stdlib.h>
#include <str.h>
#include "../private/startmenu.h"

/** Open start menu.
 *
 * @param repopath Pathname of the menu repository
 * @param rsmenu Place to store pointer to start menu
 * @return EOK on success or an error code
 */
errno_t startmenu_open(const char *repopath, startmenu_t **rsmenu)
{
	startmenu_t *smenu;
	sif_sess_t *repo = NULL;
	sif_node_t *rnode;
	sif_node_t *nentries;
	sif_node_t *nentry;
	const char *ntype;
	const char *caption;
	const char *cmd;
	errno_t rc;

	smenu = calloc(1, sizeof(startmenu_t));
	if (smenu == NULL) {
		rc = ENOMEM;
		goto error;
	}

	list_initialize(&smenu->entries);

	rc = sif_open(repopath, &repo);
	if (rc != EOK)
		goto error;

	rnode = sif_get_root(repo);
	nentries = sif_node_first_child(rnode);
	ntype = sif_node_get_type(nentries);
	if (str_cmp(ntype, "entries") != 0) {
		rc = EIO;
		goto error;
	}

	nentry = sif_node_first_child(nentries);
	while (nentry != NULL) {
		ntype = sif_node_get_type(nentry);
		if (str_cmp(ntype, "entry") != 0) {
			rc = EIO;
			goto error;
		}

		caption = sif_node_get_attr(nentry, "caption");
		if (caption == NULL) {
			rc = EIO;
			goto error;
		}

		cmd = sif_node_get_attr(nentry, "cmd");
		if (cmd == NULL) {
			rc = EIO;
			goto error;
		}

		rc = startmenu_entry_create(smenu, caption, cmd);
		if (rc != EOK)
			goto error;

		nentry = sif_node_next_child(nentry);
	}

	*rsmenu = smenu;
	return EOK;
error:
	if (repo != NULL)
		sif_close(repo);
	if (smenu != NULL)
		free(smenu);
	return rc;
}

/** Close start menu.
 *
 * @param smenu Start menu
 */
void startmenu_close(startmenu_t *smenu)
{
}

/** Get first start menu entry.
 *
 * @param smenu Start menu
 * @return First entry or @c NULL if the menu is empty
 */
startmenu_entry_t *startmenu_first(startmenu_t *smenu)
{
	link_t *link;

	link = list_first(&smenu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, startmenu_entry_t, lentries);
}

/** Get next start menu entry.
 *
 * @param cur Current entry
 * @return Next entry or @c NULL if @a cur is the last entry
 */
startmenu_entry_t *startmenu_next(startmenu_entry_t *cur)
{
	link_t *link;

	link = list_next(&cur->lentries, &cur->smenu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, startmenu_entry_t, lentries);
}

/** Get start menu entry caption.
 *
 * @param entry Start menu entry
 * @return Caption (with accelerator markup)
 */
const char *startmenu_entry_get_caption(startmenu_entry_t *entry)
{
	return entry->caption;
}

/** Get start menu entry command.
 *
 * @param entr Start menu entry
 * @return Command to run
 */
const char *startmenu_entry_get_cmd(startmenu_entry_t *entry)
{
	return entry->cmd;
}

/** Create a start menu entry and append it to the start menu (internal).
 *
 * This only creates the entry in memory, but does not update the repository.
 *
 * @param smenu Start menu
 * @param caption Caption
 * @param cmd Command to run
 */
errno_t startmenu_entry_create(startmenu_t *smenu, const char *caption,
    const char *cmd)
{
	startmenu_entry_t *entry;
	errno_t rc;

	entry = calloc(1, sizeof(startmenu_entry_t));
	if (entry == NULL) {
		rc = ENOMEM;
		goto error;
	}

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

	entry->smenu = smenu;
	list_append(&entry->lentries, &smenu->entries);
	return EOK;
error:
	if (entry != NULL) {
		if (entry->caption != NULL)
			free(entry->caption);
		if (entry->cmd != NULL)
			free(entry->cmd);
		free(entry);
	}

	return rc;
}

/** @}
 */
