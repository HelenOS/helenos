/*
 * Copyright (c) 2008 Jakub Jermar
 * Copyright (c) 2012 Julia Medvedeva
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

/** @addtogroup fs
 * @{
 */

#ifndef UDF_UDF_H_
#define UDF_UDF_H_

#include <fibril_synch.h>
#include <libfs.h>
#include <atomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../vfs/vfs.h"
#include "udf_types.h"
#include <adt/hash_table.h>

#define UDF_NODE(node) \
	((node) ? (udf_node_t *) (node)->data : NULL)

#define FS_NODE(node) \
	((node) ? (fs_node_t *) ((node)->fs_node) : NULL)

#define BS_BLOCK     0
#define MIN_SIZE     512
#define MAX_SIZE     8192
#define DEFAULT_VOL  0

#define NODE_DIR   0
#define NODE_FILE  1

#define MAX_FILE_NAME_LEN  512

#define MIN_FID_LEN  38

#define SPACE_TABLE   0
#define SPACE_BITMAP  1

typedef struct udf_partition {
	/* Partition info */
	uint16_t number;
	uint32_t access_type;
	uint32_t start;
	uint32_t lenght;
} udf_partition_t;

typedef struct udf_lvolume {
	udf_partition_t **partitions;
	size_t partition_cnt;
	uint32_t logical_block_size;
	fs_index_t root_dir;
} udf_lvolume_t;

typedef struct {
	service_id_t service_id;
	size_t open_nodes_count;
	udf_charspec_t charset;

	uint32_t sector_size;
	udf_lvolume_t *volumes;
	udf_partition_t *partitions;
	size_t partition_cnt;
	udf_unallocated_space_descriptor_t *uasd;
	uint64_t uaspace_start;
	uint64_t uaspace_lenght;
	uint8_t space_type;
} udf_instance_t;

typedef struct udf_allocator {
	uint32_t length;
	uint32_t position;
} udf_allocator_t;

typedef struct udf_node {
	udf_instance_t *instance;
	fs_node_t *fs_node;
	fibril_mutex_t lock;

	fs_index_t index;  /* FID logical block */
	ht_link_t link;
	size_t ref_cnt;
	size_t link_cnt;

	uint8_t type;  /* 1 - file, 0 - directory */
	uint64_t data_size;
	uint8_t *data;
	udf_allocator_t *allocators;
	size_t alloc_size;
} udf_node_t;

extern vfs_out_ops_t udf_ops;
extern libfs_ops_t udf_libfs_ops;

#endif

/**
 * @}
 */
