/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libsif
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef PRIVATE_SIF_H_
#define PRIVATE_SIF_H_

#include <adt/list.h>
#include <adt/odict.h>
#include <stdio.h>

/** SIF session */
struct sif_sess {
	/** Repository file */
	FILE *f;
	/** Repository file name */
	char *fname;
	/** Root node */
	struct sif_node *root;
};

/** SIF transaction */
struct sif_trans {
	struct sif_sess *sess;
};

/** SIF attribute */
typedef struct {
	/** Node to which this attribute belongs */
	struct sif_node *node;
	/** Link to @c node->attrs */
	odlink_t lattrs;
	/** Attribute name */
	char *aname;
	/** Attribute value */
	char *avalue;
} sif_attr_t;

/** SIF node */
struct sif_node {
	/** Parent node or @c NULL in case of root node */
	struct sif_node *parent;
	/** Link to parent->children */
	link_t lparent;
	/** Node type */
	char *ntype;
	/** Attributes (of sif_attr_t) */
	odict_t attrs;
	/** Child nodes */
	list_t children;
};

#endif

/** @}
 */
