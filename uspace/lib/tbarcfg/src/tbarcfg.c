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

/** @addtogroup libtbarcfg
 * @{
 */
/**
 * @file Taskbar configuration
 */

#include <async.h>
#include <errno.h>
#include <sif.h>
#include <ipc/tbarcfg.h>
#include <loc.h>
#include <task.h>
#include <tbarcfg/tbarcfg.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include "../private/tbarcfg.h"

static void tbarcfg_notify_conn(ipc_call_t *, void *);
static errno_t smenu_entry_save(smenu_entry_t *, sif_node_t *);

/** Create taskbar configuration.
 *
 * @param repopath Pathname of the new menu repository
 * @param rtbcfg Place to store pointer to taskbar configuration
 * @return EOK on success or an error code
 */
errno_t tbarcfg_create(const char *repopath, tbarcfg_t **rtbcfg)
{
	tbarcfg_t *tbcfg;
	sif_doc_t *doc = NULL;
	sif_node_t *rnode;
	sif_node_t *nentries;
	errno_t rc;

	tbcfg = calloc(1, sizeof(tbarcfg_t));
	if (tbcfg == NULL) {
		rc = ENOMEM;
		goto error;
	}

	list_initialize(&tbcfg->entries);
	tbcfg->cfgpath = str_dup(repopath);
	if (tbcfg->cfgpath == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = sif_new(&doc);
	if (rc != EOK)
		goto error;

	rnode = sif_get_root(doc);

	rc = sif_node_append_child(rnode, "entries", &nentries);
	if (rc != EOK)
		goto error;

	(void)nentries;

	rc = sif_save(doc, repopath);
	if (rc != EOK)
		goto error;

	sif_delete(doc);

	*rtbcfg = tbcfg;
	return EOK;
error:
	if (doc != NULL)
		sif_delete(doc);
	if (tbcfg != NULL && tbcfg->cfgpath != NULL)
		free(tbcfg->cfgpath);
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
	sif_doc_t *doc = NULL;
	sif_node_t *nentries;
	sif_node_t *rnode;
	sif_node_t *nentry;
	const char *ntype;
	const char *separator;
	const char *caption;
	const char *cmd;
	const char *terminal = NULL;
	errno_t rc;

	tbcfg = calloc(1, sizeof(tbarcfg_t));
	if (tbcfg == NULL) {
		rc = ENOMEM;
		goto error;
	}

	list_initialize(&tbcfg->entries);
	tbcfg->cfgpath = str_dup(repopath);
	if (tbcfg->cfgpath == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = sif_load(repopath, &doc);
	if (rc != EOK)
		goto error;

	rnode = sif_get_root(doc);
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

		separator = sif_node_get_attr(nentry, "separator");
		if (separator != NULL && str_cmp(separator, "y") != 0) {
			rc = EIO;
			goto error;
		}

		if (separator == NULL) {
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

			terminal = sif_node_get_attr(nentry, "terminal");
			if (terminal == NULL)
				terminal = "n";

			rc = smenu_entry_create(tbcfg, caption, cmd,
			    str_cmp(terminal, "y") == 0, NULL);
			if (rc != EOK)
				goto error;
		} else {
			rc = smenu_entry_sep_create(tbcfg, NULL);
			if (rc != EOK)
				goto error;
		}

		nentry = sif_node_next_child(nentry);
	}

	sif_delete(doc);
	*rtbcfg = tbcfg;
	return EOK;
error:
	if (doc != NULL)
		sif_delete(doc);
	if (tbcfg != NULL && tbcfg->cfgpath != NULL)
		free(tbcfg->cfgpath);
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
		smenu_entry_destroy(entry);
		entry = tbarcfg_smenu_first(tbcfg);
	}

	free(tbcfg->cfgpath);
	free(tbcfg);
}

/** Synchronize taskbar configuration to config file.
 *
 * @param repopath Pathname of the menu repository
 * @param rtbcfg Place to store pointer to taskbar configuration
 * @return EOK on success or an error code
 */
errno_t tbarcfg_sync(tbarcfg_t *tbcfg)
{
	sif_doc_t *doc = NULL;
	sif_node_t *nentries;
	sif_node_t *rnode;
	smenu_entry_t *entry;
	errno_t rc;

	rc = sif_new(&doc);
	if (rc != EOK)
		goto error;

	rnode = sif_get_root(doc);

	rc = sif_node_append_child(rnode, "entries", &nentries);
	if (rc != EOK)
		goto error;

	entry = tbarcfg_smenu_first(tbcfg);
	while (entry != NULL) {
		rc = smenu_entry_save(entry, nentries);
		if (rc != EOK)
			goto error;

		entry = tbarcfg_smenu_next(entry);
	}

	rc = sif_save(doc, tbcfg->cfgpath);
	if (rc != EOK)
		goto error;

	sif_delete(doc);
	return EOK;
error:
	if (doc != NULL)
		sif_delete(doc);
	if (tbcfg != NULL && tbcfg->cfgpath != NULL)
		free(tbcfg->cfgpath);
	if (tbcfg != NULL)
		free(tbcfg);
	return rc;
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

/** Get last start menu entry.
 *
 * @param tbcfg Taskbar configuration
 * @return Previous entry or @c NULL if the menu is empty
 */
smenu_entry_t *tbarcfg_smenu_last(tbarcfg_t *tbcfg)
{
	link_t *link;

	link = list_last(&tbcfg->entries);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, smenu_entry_t, lentries);
}

/** Get previous start menu entry.
 *
 * @param cur Current entry
 * @return Previous entry or @c NULL if @a cur is the last entry
 */
smenu_entry_t *tbarcfg_smenu_prev(smenu_entry_t *cur)
{
	link_t *link;

	link = list_prev(&cur->lentries, &cur->smenu->entries);
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
	assert(!entry->separator);
	return entry->caption;
}

/** Get start menu entry command.
 *
 * @param entry Start menu entry
 * @return Command to run
 */
const char *smenu_entry_get_cmd(smenu_entry_t *entry)
{
	assert(!entry->separator);
	return entry->cmd;
}

/** Get start menu entry start in terminal flag.
 *
 * @param entry Start menu entry
 * @return Start in terminal flag
 */
bool smenu_entry_get_terminal(smenu_entry_t *entry)
{
	assert(!entry->separator);
	return entry->terminal;
}

/** Get start menu entry separator flag.
 *
 * @param entry Start menu entry
 * @return Separator flag
 */
bool smenu_entry_get_separator(smenu_entry_t *entry)
{
	return entry->separator;
}

/** Set start menu entry caption.
 *
 * Note: To make the change visible to others and persistent,
 * you must call @c tbarcfg_sync()
 *
 * @param entry Start menu entry
 * @param caption New caption
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t smenu_entry_set_caption(smenu_entry_t *entry, const char *caption)
{
	char *dcap;

	assert(!entry->separator);

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
 * you must call @c tbarcfg_sync()
 *
 * @param entry Start menu entry
 * @param cmd New command
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t smenu_entry_set_cmd(smenu_entry_t *entry, const char *cmd)
{
	char *dcmd;

	assert(!entry->separator);

	dcmd = str_dup(cmd);
	if (dcmd == NULL)
		return ENOMEM;

	free(entry->cmd);
	entry->cmd = dcmd;
	return EOK;
}

/** Set start menu entry start in terminal flag.
 *
 * Note: To make the change visible to others and persistent,
 * you must call @c tbarcfg_sync()
 *
 * @param entry Start menu entry
 * @param terminal Start in terminal flag
 */
void smenu_entry_set_terminal(smenu_entry_t *entry, bool terminal)
{
	assert(!entry->separator);
	entry->terminal = terminal;
}

/** Save start menu entry using transaction.
 *
 * @param entry Start menu entry
 * @param nentries Entries node
 */
static errno_t smenu_entry_save(smenu_entry_t *entry, sif_node_t *nentries)
{
	sif_node_t *nentry = NULL;
	errno_t rc;

	rc = sif_node_append_child(nentries, "entry", &nentry);
	if (rc != EOK)
		goto error;

	if (entry->separator) {
		rc = sif_node_set_attr(nentry, "separator", "y");
		if (rc != EOK)
			goto error;
	} else {
		rc = sif_node_set_attr(nentry, "cmd", entry->cmd);
		if (rc != EOK)
			goto error;

		rc = sif_node_set_attr(nentry, "caption",
		    entry->caption);
		if (rc != EOK)
			goto error;

		rc = sif_node_set_attr(nentry, "terminal",
		    entry->terminal ? "y" : "n");
		if (rc != EOK)
			goto error;
	}

	return EOK;
error:
	if (nentry != NULL)
		sif_node_destroy(nentry);
	return rc;
}

/** Create new start menu entry and append it to the start menu (internal).
 *
 * This only creates the entry in memory, but does not update the repository.
 *
 * @param smenu Start menu
 * @param caption Caption
 * @param cmd Command to run
 * @param terminal Start in terminal
 * @param rentry Place to store pointer to new entry or @c NULL
 */
errno_t smenu_entry_create(tbarcfg_t *smenu, const char *caption,
    const char *cmd, bool terminal, smenu_entry_t **rentry)
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

	entry->terminal = terminal;

	entry->smenu = smenu;
	list_append(&entry->lentries, &smenu->entries);
	if (rentry != NULL)
		*rentry = entry;
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

/** Create new start menu separator entry and append it to the start menu
 * (internal).
 *
 * This only creates the entry in memory, but does not update the repository.
 *
 * @param smenu Start menu
 * @param rentry Place to store pointer to new entry or @c NULL
 */
errno_t smenu_entry_sep_create(tbarcfg_t *smenu, smenu_entry_t **rentry)
{
	smenu_entry_t *entry;
	errno_t rc;

	entry = calloc(1, sizeof(smenu_entry_t));
	if (entry == NULL) {
		rc = ENOMEM;
		goto error;
	}

	entry->separator = true;

	entry->smenu = smenu;
	list_append(&entry->lentries, &smenu->entries);
	if (rentry != NULL)
		*rentry = entry;

	return EOK;
error:
	return rc;
}

/** Destroy start menu entry.
 *
 * This only deletes the entry from, but does not update the
 * repository.
 *
 * @param entry Start menu entry
 */
void smenu_entry_destroy(smenu_entry_t *entry)
{
	list_remove(&entry->lentries);
	if (entry->caption != NULL)
		free(entry->caption);
	if (entry->cmd != NULL)
		free(entry->cmd);
	free(entry);
}

/** Move start menu entry up.
 *
 * @param entry Start menu entry
 */
void smenu_entry_move_up(smenu_entry_t *entry)
{
	smenu_entry_t *prev;

	prev = tbarcfg_smenu_prev(entry);
	if (prev == NULL) {
		/* Entry is already at first position, nothing to do. */
		return;
	}

	list_remove(&entry->lentries);
	list_insert_before(&entry->lentries, &prev->lentries);
}

/** Move start menu entry down.
 *
 * @param entry Start menu entry
 */
void smenu_entry_move_down(smenu_entry_t *entry)
{
	smenu_entry_t *next;

	next = tbarcfg_smenu_next(entry);
	if (next == NULL) {
		/* Entry is already at last position, nothing to do. */
		return;
	}

	list_remove(&entry->lentries);
	list_insert_after(&entry->lentries, &next->lentries);
}

/** Create taskbar configuration listener.
 *
 * Listens for taskbar configuration change notifications.
 *
 * @param nchan Notification channel (TBARCFG_NOTIFY_DEFAULT)
 * @param rlst Place to store pointer to new listener
 * @return EOK on success or an error code
 */
errno_t tbarcfg_listener_create(const char *nchan, void (*cb)(void *),
    void *arg, tbarcfg_listener_t **rlst)
{
	tbarcfg_listener_t *lst;
	service_id_t svcid = 0;
	loc_srv_t *srv = NULL;
	task_id_t taskid;
	char *svcname = NULL;
	category_id_t catid;
	port_id_t port;
	int rv;
	errno_t rc;

	lst = calloc(1, sizeof(tbarcfg_listener_t));
	if (lst == NULL)
		return ENOMEM;

	lst->cb = cb;
	lst->arg = arg;

	rc = async_create_port(INTERFACE_TBARCFG_NOTIFY,
	    tbarcfg_notify_conn, (void *)lst, &port);
	if (rc != EOK)
		goto error;

	rc = loc_server_register("tbarcfg-listener", &srv);
	if (rc != EOK)
		goto error;

	taskid = task_get_id();

	rv = asprintf(&svcname, "tbarcfg/%u", (unsigned)taskid);
	if (rv < 0) {
		rc = ENOMEM;
		goto error;
	}

	rc = loc_service_register(srv, svcname, &svcid);
	if (rc != EOK)
		goto error;

	rc = loc_category_get_id(nchan, &catid, 0);
	if (rc != EOK)
		goto error;

	rc = loc_service_add_to_cat(srv, svcid, catid);
	if (rc != EOK)
		goto error;

	*rlst = lst;
	return EOK;
error:
	if (svcid != 0)
		loc_service_unregister(srv, svcid);
	if (srv != NULL)
		loc_server_unregister(srv);
	if (svcname != NULL)
		free(svcname);
	return rc;
}

/** Destroy taskbar configuration listener.
 *
 * @param lst Listener
 */
void tbarcfg_listener_destroy(tbarcfg_listener_t *lst)
{
	free(lst);
}

/** Send taskbar configuration notification to a particular service ID.
 *
 * @param svcid Service ID
 * @return EOK on success or an error code
 */
static errno_t tbarcfg_notify_svc(service_id_t svcid)
{
	async_sess_t *sess;
	async_exch_t *exch;
	errno_t rc;

	sess = loc_service_connect(svcid, INTERFACE_TBARCFG_NOTIFY, 0);
	if (sess == NULL)
		return EIO;

	exch = async_exchange_begin(sess);
	rc = async_req_0_0(exch, TBARCFG_NOTIFY_NOTIFY);
	if (rc != EOK) {
		async_exchange_end(exch);
		async_hangup(sess);
		return rc;
	}

	async_exchange_end(exch);
	async_hangup(sess);
	return EOK;
}

/** Send taskbar configuration change notification.
 *
 * @param nchan Notification channel (TBARCFG_NOTIFY_DEFAULT)
 */
errno_t tbarcfg_notify(const char *nchan)
{
	errno_t rc;
	category_id_t catid;
	service_id_t *svcs = NULL;
	size_t count, i;

	rc = loc_category_get_id(nchan, &catid, 0);
	if (rc != EOK)
		return rc;

	rc = loc_category_get_svcs(catid, &svcs, &count);
	if (rc != EOK)
		return rc;

	for (i = 0; i < count; i++) {
		rc = tbarcfg_notify_svc(svcs[i]);
		if (rc != EOK)
			goto error;
	}

	free(svcs);
	return EOK;
error:
	free(svcs);
	return rc;
}

/** Taskbar configuration connection handler.
 *
 * @param icall Initial call
 * @param arg Argument (tbarcfg_listener_t *)
 */
static void tbarcfg_notify_conn(ipc_call_t *icall, void *arg)
{
	tbarcfg_listener_t *lst = (tbarcfg_listener_t *)arg;

	/* Accept the connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			return;
		}

		switch (method) {
		case TBARCFG_NOTIFY_NOTIFY:
			lst->cb(lst->arg);
			async_answer_0(&call, EOK);
			break;
		default:
			async_answer_0(&call, EINVAL);
		}
	}
}

/** @}
 */
