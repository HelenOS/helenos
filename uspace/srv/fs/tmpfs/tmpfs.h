/*
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
