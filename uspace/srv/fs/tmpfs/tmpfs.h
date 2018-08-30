/*
 * Copyright (c) 2008 Jakub Jermar
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

/** @addtogroup tmpfs
 * @{
 */

#ifndef TMPFS_TMPFS_H_
#define TMPFS_TMPFS_H_

#include <libfs.h>
#include <stddef.h>
#include <stdbool.h>
#include <adt/hash_table.h>

#define TMPFS_NODE(node)	((node) ? (tmpfs_node_t *)(node)->data : NULL)
#define FS_NODE(node)		((node) ? (node)->bp : NULL)

typedef enum {
	TMPFS_NONE,
	TMPFS_FILE,
	TMPFS_DIRECTORY
} tmpfs_dentry_type_t;

/* forward declaration */
struct tmpfs_node;

typedef struct tmpfs_dentry {
	link_t link;		/**< Linkage for the list of siblings. */
	struct tmpfs_node *node;/**< Back pointer to TMPFS node. */
	char *name;		/**< Name of dentry. */
} tmpfs_dentry_t;

typedef struct tmpfs_node {
	fs_node_t *bp;		/**< Back pointer to the FS node. */
	fs_index_t index;	/**< TMPFS node index. */
	service_id_t service_id;/**< Service ID of block device. */
	ht_link_t nh_link;		/**< Nodes hash table link. */
	tmpfs_dentry_type_t type;
	unsigned lnkcnt;	/**< Link count. */
	size_t size;		/**< File size if type is TMPFS_FILE. */
	void *data;		/**< File content's if type is TMPFS_FILE. */
	list_t cs_list;		/**< Child's siblings list. */
} tmpfs_node_t;

extern vfs_out_ops_t tmpfs_ops;
extern libfs_ops_t tmpfs_libfs_ops;

extern bool tmpfs_init(void);

#endif

/**
 * @}
 */
