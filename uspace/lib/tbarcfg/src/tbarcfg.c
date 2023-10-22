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

/** @addtogroup libtbarcfg
 * @{
 */
/**
 * @file Task bar configuration
 */

#include <errno.h>
#include <sif.h>
#include <tbarcfg/tbarcfg.h>
#include <stdlib.h>
#include <str.h>
#include "../private/tbarcfg.h"

/** Open task bar configuration.
 *
 * @param repopath Pathname of the menu repository
 * @param rtbcfg Place to store pointer to task bar configuration
 * @return EOK on success or an error code
 */
errno_t tbarcfg_open(const char *repopath, tbarcfg_t **rtbcfg)
{
	tbarcfg_t *tbcfg;
	sif_sess_t *repo = NULL;
	sif_node_t *rnode;
	sif_node_t *nentries;
	sif_node_t *nentry;
	const char *ntype;
	const char *caption;
	const char *cmd;
	errno_t rc;

	tbcfg = calloc(1, sizeof(tbarcfg_t));
	if (tbcfg == NULL) {
		rc = ENOMEM;
		goto error;
	}

	list_initialize(&tbcfg->entries);

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

		rc = smenu_entry_create(tbcfg, caption, cmd);
		if (rc != EOK)
			goto error;

		nentry = sif_node_next_child(nentry);
	}

	*rtbcfg = tbcfg;
	return EOK;
error:
	if (repo != NULL)
		sif_close(repo);
	if (tbcfg != NULL)
		free(tbcfg);
	return rc;
}

/** Close task bar configuration.
 *
 * @param tbcfg Start menu
 */
void tbarcfg_close(tbarcfg_t *tbcfg)
{
}

/** Get first start menu entry.
 *
 * @param tbcfg Task bar configuration
 * @return First entry or @c NULL if the menu is empty
 */
smenu_entry_t *tbarcfg_smenu_first(tbarcfg_t *tbcfg)
{
	link_t *link;

	link = list_first(&tbcfg->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, smenu_entry_t, lentries);
}

/** Get next start menu entry.
 *
 * @param cur Current entry
 * @return Next entry or @c NULL if @a cur is the last entry
 */
smenu_entry_t *tbarcfg_smenu_next(smenu_entry_t *cur)
{
	link_t *link;

	link = list_next(&cur->lentries, &cur->smenu->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, smenu_entry_t, lentries);
}

/** Get start menu entry caption.
 *
 * @param entry Start menu entry
 * @return Caption (with accelerator markup)
 */
const char *smenu_entry_get_caption(smenu_entry_t *entry)
{
	return entry->caption;
}

/** Get start menu entry command.
 *
 * @param entr Start menu entry
 * @return Command to run
 */
const char *smenu_entry_get_cmd(smenu_entry_t *entry)
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
errno_t smenu_entry_create(tbarcfg_t *smenu, const char *caption,
    const char *cmd)
{
	smenu_entry_t *entry;
	errno_t rc;

	entry = calloc(1, sizeof(smenu_entry_t));
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
