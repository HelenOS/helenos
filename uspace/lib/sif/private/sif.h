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
