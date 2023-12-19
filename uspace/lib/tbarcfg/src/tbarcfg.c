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
 * @file Taskbar configuration
 */

#include <errno.h>
#include <sif.h>
#include <tbarcfg/tbarcfg.h>
#include <stdlib.h>
#include <str.h>
#include "../private/tbarcfg.h"

/** Create taskbar configuration.
 *
 * @param repopath Pathname of the new menu repository
 * @param rtbcfg Place to store pointer to taskbar configuration
 * @return EOK on success or an error code
 */
errno_t tbarcfg_create(const char *repopath, tbarcfg_t **rtbcfg)
{
	tbarcfg_t *tbcfg;
	sif_sess_t *repo = NULL;
	sif_node_t *rnode;
	errno_t rc;
	sif_trans_t *trans = NULL;

	tbcfg = calloc(1, sizeof(tbarcfg_t));
	if (tbcfg == NULL) {
		rc = ENOMEM;
		goto error;
	}

	list_initialize(&tbcfg->entries);

	rc = sif_create(repopath, &repo);
	if (rc != EOK)
		goto error;

	tbcfg->repo = repo;

	rnode = sif_get_root(repo);

	rc = sif_trans_begin(repo, &trans);
	if (rc != EOK)
		goto error;

	rc = sif_node_append_child(trans, rnode, "entries", &tbcfg->nentries);
	if (rc != EOK)
		goto error;

	rc = sif_trans_end(trans);
	if (rc != EOK)
		goto error;

	*rtbcfg = tbcfg;
	return EOK;
error:
	if (trans != NULL)
		sif_trans_abort(trans);
	if (repo != NULL)
		sif_close(repo);
	if (tbcfg != NULL)
		free(tbcfg);
	return rc;
}

/** Open taskbar configuration.
 *
 * @param repopath Pathname of the menu repository
 * @param rtbcfg Place to store pointer to taskbar configuration
 * @return EOK on success or an error code
 */
errno_t tbarcfg_open(const char *repopath, tbarcfg_t **rtbcfg)
{
	tbarcfg_t *tbcfg;
	sif_sess_t *repo = NULL;
	sif_node_t *rnode;
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

	tbcfg->repo = repo;

	rnode = sif_get_root(repo);
	tbcfg->nentries = sif_node_first_child(rnode);
	ntype = sif_node_get_type(tbcfg->nentries);
	if (str_cmp(ntype, "entries") != 0) {
		rc = EIO;
		goto error;
	}

	nentry = sif_node_first_child(tbcfg->nentries);
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

		rc = smenu_entry_new(tbcfg, nentry, caption, cmd);
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

/** Close taskbar configuration.
 *
 * @param tbcfg Start menu
 */
void tbarcfg_close(tbarcfg_t *tbcfg)
{
	smenu_entry_t *entry;

	entry = tbarcfg_smenu_first(tbcfg);
	while (entry != NULL) {
		smenu_entry_delete(entry);
		entry = tbarcfg_smenu_first(tbcfg);
	}

	(void)sif_close(tbcfg->repo);
	free(tbcfg);
}

/** Get first start menu entry.
 *
 * @param tbcfg Taskbar configuration
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

/** Set start menu entry caption.
 *
 * Note: To make the change visible to others and persistent,
 * you must call @c smenu_entry_save()
 *
 * @param entry Start menu entry
 * @param caption New caption
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t smenu_entry_set_caption(smenu_entry_t *entry, const char *caption)
{
	char *dcap;

	dcap = str_dup(caption);
	if (dcap == NULL)
		return ENOMEM;

	free(entry->caption);
	entry->caption = dcap;
	return EOK;
}

/** Set start menu entry command.
 *
 * Note: To make the change visible to others and persistent,
 * you must call @c smenu_entry_save()
 *
 * @param entry Start menu entry
 * @param cmd New command
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t smenu_entry_set_cmd(smenu_entry_t *entry, const char *cmd)
{
	char *dcmd;

	dcmd = str_dup(cmd);
	if (dcmd == NULL)
		return ENOMEM;

	free(entry->cmd);
	entry->cmd = dcmd;
	return EOK;
}

/** Save any changes to start menu entry.
 *
 * @param entry Start menu entry
 */
errno_t smenu_entry_save(smenu_entry_t *entry)
{
	sif_trans_t *trans = NULL;
	errno_t rc;

	rc = sif_trans_begin(entry->smenu->repo, &trans);
	if (rc != EOK)
		goto error;

	rc = sif_node_set_attr(trans, entry->nentry, "cmd", entry->cmd);
	if (rc != EOK)
		goto error;

	rc = sif_node_set_attr(trans, entry->nentry, "caption", entry->caption);
	if (rc != EOK)
		goto error;

	rc = sif_trans_end(trans);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	if (trans != NULL)
		sif_trans_abort(trans);
	return rc;
}

/** Allocate a start menu entry and append it to the start menu (internal).
 *
 * This only creates the entry in memory, but does not update the repository.
 *
 * @param smenu Start menu
 * @param nentry Backing SIF node
 * @param caption Caption
 * @param cmd Command to run
 */
errno_t smenu_entry_new(tbarcfg_t *smenu, sif_node_t *nentry,
    const char *caption, const char *cmd)
{
	smenu_entry_t *entry;
	errno_t rc;

	entry = calloc(1, sizeof(smenu_entry_t));
	if (entry == NULL) {
		rc = ENOMEM;
		goto error;
	}

	entry->nentry = nentry;

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

/** Delete start menu entry.
 *
 * This only deletes the entry from, but does not update the
 * repository.
 *
 * @param entry Start menu entry
 */
void smenu_entry_delete(smenu_entry_t *entry)
{
	list_remove(&entry->lentries);
	free(entry->caption);
	free(entry->cmd);
	free(entry);
}

/** Create new start menu entry.
 *
 * @param smenu Start menu
 * @param nentry Backing SIF node
 * @param caption Caption
 * @param cmd Command to run
 */
errno_t smenu_entry_create(tbarcfg_t *smenu, const char *caption,
    const char *cmd)
{
	sif_node_t *nentry;
	errno_t rc;
	sif_trans_t *trans = NULL;

	rc = sif_trans_begin(smenu->repo, &trans);
	if (rc != EOK)
		goto error;

	rc = sif_node_append_child(trans, smenu->nentries, "entry",
	    &nentry);
	if (rc != EOK)
		goto error;

	rc = sif_node_set_attr(trans, nentry, "cmd", cmd);
	if (rc != EOK)
		goto error;

	rc = sif_node_set_attr(trans, nentry, "caption", caption);
	if (rc != EOK)
		goto error;

	rc = smenu_entry_new(smenu, nentry, caption, cmd);
	if (rc != EOK)
		goto error;

	rc = sif_trans_end(trans);
	if (rc != EOK)
		goto error;

	return EOK;
error:
	if (trans != NULL)
		sif_trans_abort(trans);
	return rc;
}

/** Destroy start menu entry..
 *
 * @param entry Start menu entry
 * @return EOK on success or an error code
 */
errno_t smenu_entry_destroy(smenu_entry_t *entry)
{
	errno_t rc;
	sif_trans_t *trans = NULL;

	rc = sif_trans_begin(entry->smenu->repo, &trans);
	if (rc != EOK)
		goto error;

	sif_node_destroy(trans, entry->nentry);

	rc = sif_trans_end(trans);
	if (rc != EOK)
		goto error;

	smenu_entry_delete(entry);
	return EOK;
error:
	if (trans != NULL)
		sif_trans_abort(trans);
	return rc;
}

/** @}
 */
