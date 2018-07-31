/*
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup contacts
 * @{
 */

/**
 * @file Contact list application.
 *
 * Maintain a contact list / address book. The main purpose of this
 * trivial application is to serve as an example of using SIF.
 */

#include <adt/list.h>
#include <errno.h>
#include <nchoice.h>
#include <sif.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

/** Contacts */
typedef struct {
	/** Open SIF repository */
	sif_sess_t *repo;
	/** Entries SIF node */
	sif_node_t *nentries;
	/** Entries list (of contacts_entry_t) */
	list_t entries;
} contacts_t;

/** Contact entry */
typedef struct {
	/** Link to contacts_t.entries */
	link_t lentries;
} contacts_entry_t;

/** Actions in contact menu */
typedef enum {
	/** Create new contact */
	ac_create_contact,
	/** Delete contact */
	ac_delete_contact,
	/** Exit */
	ac_exit
} contact_action_t;

/** Open contacts repo or create it if it does not exist.
 *
 * @param fname File name
 * @param rcontacts Place to store pointer to contacts object.
 *
 * @return EOK on success or error code
 */
static errno_t contacts_open(const char *fname, contacts_t **rcontacts)
{
	contacts_t *contacts = NULL;
	sif_sess_t *repo = NULL;
	sif_trans_t *trans = NULL;
	sif_node_t *node;
	const char *ntype;
	errno_t rc;

	contacts = calloc(1, sizeof(contacts_t));
	if (contacts == NULL)
		return ENOMEM;

	list_initialize(&contacts->entries);

	/* Try to open an existing repository. */
	rc = sif_open(fname, &repo);
	if (rc != EOK) {
		/* Failed to open existing, create new repository */
		rc = sif_create(fname, &repo);
		if (rc != EOK)
			goto error;

		/* Start a transaction */
		rc = sif_trans_begin(repo, &trans);
		if (rc != EOK)
			goto error;

		/* Create 'entries' node container for all entries */
		rc = sif_node_append_child(trans, sif_get_root(repo), "entries",
		    &contacts->nentries);
		if (rc != EOK)
			goto error;

		/* Finish the transaction */
		rc = sif_trans_end(trans);
		if (rc != EOK)
			goto error;

		trans = NULL;
	} else {
		/*
		 * Opened an existing repository. Find the 'entries' node.
		 * It should be the very first child of the root node.
		 * This is okay to do in general, as long as we don't
		 * require forward compatibility (which we don't).
		 */
		node = sif_node_first_child(sif_get_root(repo));
		if (node == NULL) {
			rc = EIO;
			goto error;
		}

		/* Verify it's the correct node type(!) */
		ntype = sif_node_get_type(node);
		if (str_cmp(ntype, "entries") != 0) {
			rc = EIO;
			goto error;
		}

		contacts->nentries = node;
	}

	contacts->repo = repo;
	*rcontacts = contacts;

	return EOK;
error:
	if (trans != NULL)
		sif_trans_abort(trans);
	if (repo != NULL)
		(void) sif_close(repo);
	if (contacts != NULL)
		free(contacts);
	return rc;
}

/** Interaction to create new contact.
 *
 * @param contacts Contacts
 */
static errno_t contacts_create_contact(contacts_t *contacts)
{
	tinput_t *tinput;
	sif_trans_t *trans = NULL;
	sif_node_t *nentry;
	errno_t rc;
	char *cname = NULL;

	tinput = tinput_new();
	if (tinput == NULL)
		return ENOMEM;

	printf("Contact name:\n");

	rc = tinput_set_prompt(tinput, "?> ");
	if (rc != EOK)
		goto error;

	rc = tinput_read(tinput, &cname);
	if (rc != EOK)
		goto error;

	rc = sif_trans_begin(contacts->repo, &trans);
	if (rc != EOK)
		goto error;

	rc = sif_node_append_child(trans, contacts->nentries, "entry", &nentry);
	if (rc != EOK)
		goto error;

	rc = sif_node_set_attr(trans, nentry, "name", cname);
	if (rc != EOK)
		goto error;

	rc = sif_trans_end(trans);
	if (rc != EOK)
		goto error;

	trans = NULL;

	free(cname);
	tinput_destroy(tinput);
	return EOK;
error:
	if (trans != NULL)
		sif_trans_abort(trans);
	if (cname != NULL)
		free(cname);
	return rc;
}

/** Interaction to delete contact.
 *
 * @param contacts Contacts
 */
static errno_t contacts_delete_contact(contacts_t *contacts)
{
	return EOK;
}

/** Close contacts repo.
 *
 * @param contacts Contacts
 */
static void contacts_close(contacts_t *contacts)
{
	printf("Closing repo %p\n", contacts->repo);
	sif_close(contacts->repo);
	free(contacts);
}

/** Run contacts main menu.
 *
 * @param contacts Contacts
 * @return EOK on success or error code
 */
static errno_t contacts_main(contacts_t *contacts)
{
	nchoice_t *choice = NULL;
	errno_t rc;
	bool quit = false;
	void *sel;

	rc = nchoice_create(&choice);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_set_prompt(choice, "Select action");
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_add(choice, "Create contact",
	    (void *)ac_create_contact, 0);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_add(choice, "Delete contact",
	    (void *)ac_delete_contact, 0);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_add(choice, "Exit",
	    (void *)ac_exit, 0);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	while (!quit) {
		rc = nchoice_get(choice, &sel);
		if (rc != EOK) {
			printf("Error getting user selection.\n");
			return rc;
		}

		switch ((contact_action_t)sel) {
		case ac_create_contact:
			(void) contacts_create_contact(contacts);
			break;
		case ac_delete_contact:
			(void) contacts_delete_contact(contacts);
			break;
		case ac_exit:
			quit = true;
			break;
		}
	}

	nchoice_destroy(choice);
	return EOK;
error:
	if (choice != NULL)
		nchoice_destroy(choice);
	return rc;
}

int main(void)
{
	errno_t rc;
	contacts_t *contacts;

	rc = contacts_open("contacts.sif", &contacts);
	if (rc != EOK)
		return 1;

	rc = contacts_main(contacts);
	contacts_close(contacts);

	if (rc != EOK)
		return 1;

	return 0;
}
