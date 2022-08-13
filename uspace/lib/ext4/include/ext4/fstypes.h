/*
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */

#ifndef LIBEXT4_FSTYPES_H_
#define LIBEXT4_FSTYPES_H_

#include <adt/list.h>
#include <libfs.h>
#include <loc.h>
#include "ext4/types.h"

/**
 * Type for holding an instance of mounted partition.
 */
typedef struct ext4_instance {
	link_t link;
	service_id_t service_id;
	ext4_filesystem_t *filesystem;
	unsigned int open_nodes_count;
} ext4_instance_t;

/**
 * Type for wrapping common fs_node and add some useful pointers.
 */
typedef struct ext4_node {
	ext4_instance_t *instance;
	ext4_inode_ref_t *inode_ref;
	fs_node_t *fs_node;
	ht_link_t link;
	unsigned int references;
} ext4_node_t;

#define EXT4_NODE(node) \
	((node) ? (ext4_node_t *) (node)->data : NULL)

#endif

/**
 * @}
 */
