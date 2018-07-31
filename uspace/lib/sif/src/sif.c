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
/** @addtogroup libsif
 * @{
 */
/**
 * @file Structured Information Format
 *
 * Structured Information Format (SIF) is an API that allows an application
 * to maintain data in a persistent repository in a format that
 *
 *  - is structured (and hence extensible)
 *  - allows atomic (transactional) updates
 *  - allows efficient updates
 *
 * SIF is meant to be used as the basis for the storage backend used to
 * maintain application or configuration data. SIF is *not* a (relational)
 * database (not even close). The structure of a SIF repository is quite
 * similar to an XML document that contains just tags with attributes
 * (but no text).
 *
 * SIF can thus naturally express ordered lists (unlike a relational database).
 * When contrasted to a (relational) database, SIF is much more primitive.
 *
 * In particular, SIF
 *
 *  - does not run on a separate server
 *  - des not handle concurrency
 *  - does not have a notion of types or data validation
 *  - does not understand relations
 *  - does not implement any kind of search/queries
 *  - does not deal with data sets large enough not to fit in primary memory
 *
 * any kind of structure data validation is left up to the application.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include "../include/sif.h"
#include "../private/sif.h"

static errno_t sif_export_node(sif_node_t *, FILE *);
static errno_t sif_import_node(sif_node_t *, FILE *, sif_node_t **);

/** Create new SIF node.
 *
 * @param parent Parent onde
 * @return Pointer to new node on success or @c NULL if out of memory
 */
static sif_node_t *sif_node_new(sif_node_t *parent)
{
	sif_node_t *node;

	node = calloc(1, sizeof(sif_node_t));
	if (node == NULL)
		return NULL;

	node->parent = parent;
	list_initialize(&node->children);

	return node;
}

/** Delete SIF node.
 *
 * Delete a SIF node that has been already unlinked from the tree.
 *
 * @param node Node
 */
static void sif_node_delete(sif_node_t *node)
{
	if (node == NULL)
		return;

	if (node->ntype != NULL)
		free(node->ntype);

	free(node);
}

/** Create and open a SIF repository.
 *
 * @param fname File name
 * @param rsess Place to store pointer to new session.
 *
 * @return EOK on success or error code
 */
errno_t sif_create(const char *fname, sif_sess_t **rsess)
{
	sif_sess_t *sess;
	sif_node_t *root = NULL;
	errno_t rc;
	FILE *f;

	sess = calloc(1, sizeof(sif_sess_t));
	if (sess == NULL)
		return ENOMEM;

	root = sif_node_new(NULL);
	if (root == NULL) {
		rc = ENOMEM;
		goto error;
	}

	root->ntype = str_dup("sif");
	if (root->ntype == NULL) {
		rc = ENOMEM;
		goto error;
	}

	f = fopen(fname, "w");
	if (f == NULL) {
		rc = EIO;
		goto error;
	}

	sess->f = f;
	sess->root = root;
	*rsess = sess;
	return EOK;
error:
	sif_node_delete(root);
	free(sess);
	return rc;
}

/** Open an existing SIF repository.
 *
 * @param fname File name
 * @param rsess Place to store pointer to new session.
 *
 * @return EOK on success or error code
 */
errno_t sif_open(const char *fname, sif_sess_t **rsess)
{
	sif_sess_t *sess;
	sif_node_t *root = NULL;
	errno_t rc;
	FILE *f;

	sess = calloc(1, sizeof(sif_sess_t));
	if (sess == NULL)
		return ENOMEM;

	f = fopen(fname, "r+");
	if (f == NULL) {
		rc = EIO;
		goto error;
	}

	rc = sif_import_node(NULL, f, &root);
	if (rc != EOK)
		goto error;

	if (str_cmp(root->ntype, "sif") != 0) {
		rc = EIO;
		goto error;
	}

	sess->root = root;

	sess->f = f;
	sess->root = root;
	*rsess = sess;
	return EOK;
error:
	sif_node_delete(root);
	free(sess);
	return rc;
}

/** Close SIF session.
 *
 * @param sess SIF session
 * @return EOK on success or error code
 */
errno_t sif_close(sif_sess_t *sess)
{
	sif_node_delete(sess->root);

	if (fclose(sess->f) < 0) {
		free(sess);
		return EIO;
	}

	return EOK;
}

/** Return root node.
 *
 * @param sess SIF session
 */
sif_node_t *sif_get_root(sif_sess_t *sess)
{
	return sess->root;
}

/** Get first child of a node.
 *
 * @param parent Parent node
 * @return First child node or @c NULL if @a parent has no children
 */
sif_node_t *sif_node_first_child(sif_node_t *parent)
{
	link_t *link;

	link = list_first(&parent->children);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, sif_node_t, lparent);
}

/** Get next child of a node.
 *
 * @param current Current node
 * @return Next child (i.e. next sibling of @a current)
 */
sif_node_t *sif_node_next_child(sif_node_t *current)
{
	link_t *link;

	link = list_next(&current->lparent, &current->parent->children);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, sif_node_t, lparent);
}

/** Get node type.
 *
 * @param node SIF node
 * @return Pointer to string, valid until next modification.
 */
const char *sif_node_get_type(sif_node_t *node)
{
	return node->ntype;
}

/** Get node attribute.
 *
 * @param node SIF node
 * @param aname Attribute name
 *
 * @return Attribute value or @c NULL if attribute is not set
 */
const char *sif_node_get_attr(sif_node_t *node, const char *aname)
{
	return NULL;
}

/** Begin SIF transaction.
 *
 * @param trans Transaction
 * @return EOK on success or error code
 */
errno_t sif_trans_begin(sif_sess_t *sess, sif_trans_t **rtrans)
{
	sif_trans_t *trans;

	trans = calloc(1, sizeof(sif_trans_t));
	if (trans == NULL)
		return ENOMEM;

	trans->sess = sess;
	*rtrans = trans;
	return EOK;
}

/** Commit SIF transaction.
 *
 * @param trans Transaction
 * @return EOK on success or error code
 */
errno_t sif_trans_end(sif_trans_t *trans)
{
	errno_t rc;

	rewind(trans->sess->f);

	rc = sif_export_node(trans->sess->root, trans->sess->f);
	if (rc != EOK)
		return rc;

	free(trans);
	return EOK;
}

/** Abort SIF transaction.
 *
 * @param trans Transaction
 */
void sif_trans_abort(sif_trans_t *trans)
{
}

/** Prepend new child.
 *
 * Create a new child and prepend it at the beginning of children list of
 * @a parent.
 *
 * @param trans Transaction
 * @param parent Parent node
 * @param ctype Child type
 * @param rchild Place to store pointer to new child
 *
 * @return EOK on success or ENOMEM if out of memory
 */
errno_t sif_node_prepend_child(sif_trans_t *trans, sif_node_t *parent,
    const char *ctype, sif_node_t **rchild)
{
	sif_node_t *child;

	child = sif_node_new(parent);
	if (child == NULL)
		return ENOMEM;

	child->ntype = str_dup(ctype);
	if (child->ntype == NULL) {
		sif_node_delete(child);
		return ENOMEM;
	}

	list_prepend(&child->lparent, &parent->children);

	*rchild = child;
	return EOK;
}

/** Append new child.
 *
 * Create a new child and append it at the end of children list of @a parent.
 *
 * @param trans Transaction
 * @param parent Parent node
 * @param ctype Child type
 * @param rchild Place to store pointer to new child
 *
 * @return EOK on success or ENOMEM if out of memory
 */
errno_t sif_node_append_child(sif_trans_t *trans, sif_node_t *parent,
    const char *ctype, sif_node_t **rchild)
{
	sif_node_t *child;

	child = sif_node_new(parent);
	if (child == NULL)
		return ENOMEM;

	child->ntype = str_dup(ctype);
	if (child->ntype == NULL) {
		sif_node_delete(child);
		return ENOMEM;
	}

	list_append(&child->lparent, &parent->children);

	*rchild = child;
	return EOK;
}

/** Insert new child before existing child.
 *
 * Create a new child and insert it before an existing child.
 *
 * @param trans Transaction
 * @param sibling Sibling before which to insert
 * @param ctype Child type
 * @param rchild Place to store pointer to new child
 *
 * @return EOK on success or ENOMEM if out of memory
 */
errno_t sif_node_insert_before(sif_trans_t *trans, sif_node_t *sibling,
    const char *ctype, sif_node_t **rchild)
{
	sif_node_t *child;

	child = sif_node_new(sibling->parent);
	if (child == NULL)
		return ENOMEM;

	child->ntype = str_dup(ctype);
	if (child->ntype == NULL) {
		sif_node_delete(child);
		return ENOMEM;
	}

	list_insert_before(&child->lparent, &sibling->lparent);

	*rchild = child;
	return EOK;
}

/** Insert new child after existing child.
 *
 * Create a new child and insert it after an existing child.
 *
 * @param trans Transaction
 * @param sibling Sibling after which to insert
 * @param ctype Child type
 * @param rchild Place to store pointer to new child
 *
 * @return EOK on success or ENOMEM if out of memory
 */
errno_t sif_node_insert_after(sif_trans_t *trans, sif_node_t *sibling,
    const char *ctype, sif_node_t **rchild)
{
	sif_node_t *child;

	child = sif_node_new(sibling->parent);
	if (child == NULL)
		return ENOMEM;

	child->ntype = str_dup(ctype);
	if (child->ntype == NULL) {
		sif_node_delete(child);
		return ENOMEM;
	}

	list_insert_after(&child->lparent, &sibling->lparent);

	*rchild = child;
	return EOK;
}

/** Destroy SIF node.
 *
 * This function does not return an error, but the transaction may still
 * fail to complete.
 *
 * @param trans Transaction
 * @param node Node to destroy
 */
void sif_node_destroy(sif_trans_t *trans, sif_node_t *node)
{
	sif_node_t *child;

	child = sif_node_first_child(node);
	while (child != NULL) {
		sif_node_destroy(trans, child);
		child = sif_node_first_child(node);
	}

	list_remove(&node->lparent);
	sif_node_delete(node);
}

/** Set node attribute.
 *
 * @param trans Transaction
 * @param node SIF node
 * @param aname Attribute name
 * @param value Attribute value
 *
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t sif_node_set_attr(sif_trans_t *trans, sif_node_t *node,
    const char *aname, const char *value)
{
	return EOK;
}

/** Unset node attribute.
 *
 * This function does not return an error, but the transaction may still
 * fail to complete.
 *
 * @param trans Transaction
 * @param node Node
 * @param aname Attribute name
 */
void sif_node_unset_attr(sif_trans_t *trans, sif_node_t *node,
    const char *aname)
{
}

/** Export string to file.
 *
 * Export string to file (the string is bracketed and escaped).
 *
 * @param str String
 * @param f File
 * @return EOK on success, EIO on I/O error
 */
static errno_t sif_export_string(const char *str, FILE *f)
{
	const char *cp;

	if (fputc('[', f) == EOF)
		return EIO;

	cp = str;
	while (*cp != '\0') {
		if (*cp == ']' || *cp == '\\') {
			if (fputc('\\', f) == EOF)
				return EIO;
		}
		if (fputc(*cp, f) == EOF)
			return EIO;
		++cp;
	}

	if (fputc(']', f) == EOF)
		return EIO;

	return EOK;
}

/** Import string from file.
 *
 * Import string from file (the string in the file must be
 * properly bracketed and escaped).
 *
 * @param f File
 * @param rstr Place to store pointer to newly allocated string
 * @return EOK on success, EIO on I/O error
 */
static errno_t sif_import_string(FILE *f, char **rstr)
{
	char *str;
	size_t str_size;
	size_t sidx;
	char c;
	errno_t rc;

	str_size = 1;
	sidx = 0;
	str = malloc(str_size + 1);
	if (str == NULL)
		return ENOMEM;

	c = fgetc(f);
	if (c != '[') {
		rc = EIO;
		goto error;
	}

	while (true) {
		c = fgetc(f);
		if (c == EOF) {
			rc = EIO;
			goto error;
		}

		if (c == ']')
			break;

		if (c == '\\') {
			c = fgetc(f);
			if (c == EOF) {
				rc = EIO;
				goto error;
			}
		}

		if (sidx >= str_size) {
			str_size *= 2;
			str = realloc(str, str_size + 1);
			if (str == NULL) {
				rc = ENOMEM;
				goto error;
			}
		}

		str[sidx++] = c;
	}

	str[sidx] = '\0';
	*rstr = str;
	return EOK;
error:
	free(str);
	return rc;
}

/** Export SIF node to file.
 *
 * @param node SIF node
 * @param f File
 * @return EOK on success, EIO on I/O error
 */
static errno_t sif_export_node(sif_node_t *node, FILE *f)
{
	errno_t rc;
	sif_node_t *child;

	rc = sif_export_string(node->ntype, f);
	if (rc != EOK)
		return rc;

	if (fputc('{', f) == EOF)
		return EIO;

	child = sif_node_first_child(node);
	while (child != NULL) {
		rc = sif_export_node(child, f);
		if (rc != EOK)
			return rc;

		child = sif_node_next_child(child);
	}

	if (fputc('}', f) == EOF)
		return EIO;

	return EOK;
}

/** Import SIF node from file.
 *
 * @param parent Parent node
 * @param f File
 * @param rnode Place to store pointer to imported node
 * @return EOK on success, EIO on I/O error
 */
static errno_t sif_import_node(sif_node_t *parent, FILE *f, sif_node_t **rnode)
{
	errno_t rc;
	sif_node_t *node = NULL;
	sif_node_t *child;
	char *ntype;
	char c;

	node = sif_node_new(parent);
	if (node == NULL)
		return ENOMEM;

	rc = sif_import_string(f, &ntype);
	if (rc != EOK)
		goto error;

	node->ntype = ntype;

	c = fgetc(f);
	if (c != '{') {
		rc = EIO;
		goto error;
	}

	c = fgetc(f);
	if (c == EOF) {
		rc = EIO;
		goto error;
	}

	while (c != '}') {
		ungetc(c, f);

		rc = sif_import_node(node, f, &child);
		if (rc != EOK)
			goto error;

		list_append(&child->lparent, &node->children);

		c = fgetc(f);
		if (c == EOF) {
			rc = EIO;
			goto error;
		}
	}

	*rnode = node;
	return EOK;
error:
	sif_node_delete(node);
	return rc;
}

/** @}
 */
