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

#include <adt/list.h>
#include <adt/odict.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include "../include/sif.h"
#include "../private/sif.h"

static errno_t sif_export_node(sif_node_t *, FILE *);
static errno_t sif_import_node(sif_node_t *, FILE *, sif_node_t **);
static sif_attr_t *sif_node_first_attr(sif_node_t *);
static sif_attr_t *sif_node_next_attr(sif_attr_t *);
static void sif_attr_delete(sif_attr_t *);
static void *sif_attr_getkey(odlink_t *);
static int sif_attr_cmp(void *, void *);

/** Create new SIF node.
 *
 * @param parent Parent node
 * @return Pointer to new node on success or @c NULL if out of memory
 */
static sif_node_t *sif_node_new(sif_node_t *parent)
{
	sif_node_t *node;

	node = calloc(1, sizeof(sif_node_t));
	if (node == NULL)
		return NULL;

	node->parent = parent;
	odict_initialize(&node->attrs, sif_attr_getkey, sif_attr_cmp);
	list_initialize(&node->children);

	return node;
}

/** Delete SIF node.
 *
 * Delete a SIF node that has been already unlinked from the tree.
 * This will also delete any attributes or child nodes.
 *
 * @param node Node
 */
static void sif_node_delete(sif_node_t *node)
{
	sif_attr_t *attr;
	sif_node_t *child;

	if (node == NULL)
		return;

	assert(!link_used(&node->lparent));

	if (node->ntype != NULL)
		free(node->ntype);

	attr = sif_node_first_attr(node);
	while (attr != NULL) {
		odict_remove(&attr->lattrs);
		sif_attr_delete(attr);
		attr = sif_node_first_attr(node);
	}

	child = sif_node_first_child(node);
	while (child != NULL) {
		list_remove(&child->lparent);
		sif_node_delete(child);
		child = sif_node_first_child(node);
	}

	free(node);
}

/** Create new SIF attribute.
 *
 * @param node Containing node
 * @return Pointer to new node on success or @c NULL if out of memory
 */
static sif_attr_t *sif_attr_new(sif_node_t *node)
{
	sif_attr_t *attr;

	attr = calloc(1, sizeof(sif_attr_t));
	if (attr == NULL)
		return NULL;

	attr->node = node;
	return attr;
}

/** Delete SIF attribute.
 *
 * Delete a SIF attribute that has been already unlinked from is node.
 *
 * @param attr Attribute
 */
static void sif_attr_delete(sif_attr_t *attr)
{
	if (attr == NULL)
		return;

	assert(!odlink_used(&attr->lattrs));

	if (attr->aname != NULL)
		free(attr->aname);
	if (attr->avalue != NULL)
		free(attr->avalue);

	free(attr);
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
	sif_trans_t *trans = NULL;
	errno_t rc;
	FILE *f;

	sess = calloc(1, sizeof(sif_sess_t));
	if (sess == NULL)
		return ENOMEM;

	sess->fname = str_dup(fname);
	if (sess->fname == NULL) {
		rc = ENOMEM;
		goto error;
	}

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

	f = fopen(fname, "wx");
	if (f == NULL) {
		rc = EIO;
		goto error;
	}

	sess->f = f;
	sess->root = root;

	/* Run a dummy trasaction to marshall initial repo state to file */
	rc = sif_trans_begin(sess, &trans);
	if (rc != EOK)
		goto error;

	rc = sif_trans_end(trans);
	if (rc != EOK)
		goto error;

	*rsess = sess;
	return EOK;
error:
	if (trans != NULL)
		sif_trans_abort(trans);
	sif_node_delete(root);
	if (sess->fname != NULL)
		free(sess->fname);
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

	sess->fname = str_dup(fname);
	if (sess->fname == NULL) {
		rc = ENOMEM;
		goto error;
	}

	f = fopen(fname, "r");
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
	if (sess->fname != NULL)
		free(sess->fname);
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

	if (sess->fname != NULL)
		free(sess->fname);
	free(sess);
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
	odlink_t *link;
	sif_attr_t *attr;

	link = odict_find_eq(&node->attrs, (void *)aname, NULL);
	if (link == NULL)
		return NULL;

	attr = odict_get_instance(link, sif_attr_t, lattrs);
	return attr->avalue;
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
 * Commit and free the transaction. If an error is returned, that means
 * the transaction has not been freed (and sif_trans_abort() must be used).
 *
 * @param trans Transaction
 * @return EOK on success or error code
 */
errno_t sif_trans_end(sif_trans_t *trans)
{
	errno_t rc;

	(void) fclose(trans->sess->f);

	trans->sess->f = fopen(trans->sess->fname, "w");
	if (trans->sess->f == NULL)
		return EIO;

	rc = sif_export_node(trans->sess->root, trans->sess->f);
	if (rc != EOK)
		return rc;

	if (fputc('\n', trans->sess->f) == EOF)
		return EIO;

	if (fflush(trans->sess->f) == EOF)
		return EIO;

	free(trans);
	return EOK;
}

/** Abort SIF transaction.
 *
 * @param trans Transaction
 */
void sif_trans_abort(sif_trans_t *trans)
{
	free(trans);
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
    const char *aname, const char *avalue)
{
	odlink_t *link;
	sif_attr_t *attr;
	char *cvalue;

	link = odict_find_eq(&node->attrs, (void *)aname, NULL);

	if (link != NULL) {
		attr = odict_get_instance(link, sif_attr_t, lattrs);
		cvalue = str_dup(avalue);
		if (cvalue == NULL)
			return ENOMEM;

		free(attr->avalue);
		attr->avalue = cvalue;
	} else {
		attr = sif_attr_new(node);
		if (attr == NULL)
			return ENOMEM;

		attr->aname = str_dup(aname);
		if (attr->aname == NULL) {
			sif_attr_delete(attr);
			return ENOMEM;
		}

		attr->avalue = str_dup(avalue);
		if (attr->avalue == NULL) {
			sif_attr_delete(attr);
			return ENOMEM;
		}

		odict_insert(&attr->lattrs, &node->attrs, NULL);
	}

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
	odlink_t *link;
	sif_attr_t *attr;

	link = odict_find_eq(&node->attrs, (void *)aname, NULL);
	if (link == NULL)
		return;

	attr = odict_get_instance(link, sif_attr_t, lattrs);
	odict_remove(link);
	sif_attr_delete(attr);
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
	int c;
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

/** Import SIF attribute from file.
 *
 * @param node Node under which attribute shou
 * @param f File
 * @param rattr Place to store pointer to imported SIF attribute
 * @return EOK on success, EIO on I/O error
 */
static errno_t sif_import_attr(sif_node_t *node, FILE *f, sif_attr_t **rattr)
{
	errno_t rc;
	char *aname = NULL;
	char *avalue = NULL;
	sif_attr_t *attr;
	int c;

	rc = sif_import_string(f, &aname);
	if (rc != EOK)
		goto error;

	c = fgetc(f);
	if (c != '=') {
		rc = EIO;
		goto error;
	}

	rc = sif_import_string(f, &avalue);
	if (rc != EOK)
		goto error;

	attr = sif_attr_new(node);
	if (attr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	attr->aname = aname;
	attr->avalue = avalue;

	*rattr = attr;
	return EOK;
error:
	if (aname != NULL)
		free(aname);
	if (avalue != NULL)
		free(avalue);
	return rc;
}

/** Export SIF attribute to file.
 *
 * @param attr SIF attribute
 * @param f File
 * @return EOK on success, EIO on I/O error
 */
static errno_t sif_export_attr(sif_attr_t *attr, FILE *f)
{
	errno_t rc;

	rc = sif_export_string(attr->aname, f);
	if (rc != EOK)
		return rc;

	if (fputc('=', f) == EOF)
		return EIO;

	rc = sif_export_string(attr->avalue, f);
	if (rc != EOK)
		return rc;

	return EOK;
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
	sif_attr_t *attr;
	sif_node_t *child;

	rc = sif_export_string(node->ntype, f);
	if (rc != EOK)
		return rc;

	/* Attributes */

	if (fputc('(', f) == EOF)
		return EIO;

	attr = sif_node_first_attr(node);
	while (attr != NULL) {
		rc = sif_export_attr(attr, f);
		if (rc != EOK)
			return rc;

		attr = sif_node_next_attr(attr);
	}

	if (fputc(')', f) == EOF)
		return EIO;

	/* Child nodes */

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
	sif_attr_t *attr = NULL;
	char *ntype;
	int c;

	node = sif_node_new(parent);
	if (node == NULL)
		return ENOMEM;

	rc = sif_import_string(f, &ntype);
	if (rc != EOK)
		goto error;

	node->ntype = ntype;

	/* Attributes */

	c = fgetc(f);
	if (c != '(') {
		rc = EIO;
		goto error;
	}

	c = fgetc(f);
	if (c == EOF) {
		rc = EIO;
		goto error;
	}

	while (c != ')') {
		ungetc(c, f);

		rc = sif_import_attr(node, f, &attr);
		if (rc != EOK)
			goto error;

		odict_insert(&attr->lattrs, &node->attrs, NULL);

		c = fgetc(f);
		if (c == EOF) {
			rc = EIO;
			goto error;
		}
	}

	/* Child nodes */

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

/** Get first attribute or a node.
 *
 * @param node SIF node
 * @return First attribute or @c NULL if there is none
 */
static sif_attr_t *sif_node_first_attr(sif_node_t *node)
{
	odlink_t *link;

	link = odict_first(&node->attrs);
	if (link == NULL)
		return NULL;

	return odict_get_instance(link, sif_attr_t, lattrs);
}

/** Get next attribute or a node.
 *
 * @param cur Current attribute
 * @return Next attribute or @c NULL if there is none
 */
static sif_attr_t *sif_node_next_attr(sif_attr_t *cur)
{
	odlink_t *link;

	link = odict_next(&cur->lattrs, &cur->node->attrs);
	if (link == NULL)
		return NULL;

	return odict_get_instance(link, sif_attr_t, lattrs);
}

/** Get key callback for ordered dictionary of node attributes.
 *
 * @param link Ordered dictionary link of attribute
 * @return Pointer to attribute name
 */
static void *sif_attr_getkey(odlink_t *link)
{
	return (void *)odict_get_instance(link, sif_attr_t, lattrs)->aname;
}

/** Comparison callback for  ordered dictionary of node attributes.
 *
 * @param a Name of first attribute
 * @param b Name of second attribute
 * @return Less than zero, zero or greater than zero, if a < b, a == b, a > b,
 *         respectively.
 */
static int sif_attr_cmp(void *a, void *b)
{
	char *ca, *cb;

	ca = (char *)a;
	cb = (char *)b;

	return str_cmp(ca, cb);
}

/** @}
 */
