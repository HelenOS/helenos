/*
 * Copyright (c) 2011 Martin Decky
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

/**
 * @file cdfs_ops.c
 * @brief Implementation of VFS operations for the cdfs file system
 *        server.
 */

#include <bool.h>
#include <adt/hash_table.h>
#include <malloc.h>
#include <mem.h>
#include <loc.h>
#include <libfs.h>
#include <errno.h>
#include <libblock.h>
#include <str.h>
#include <byteorder.h>
#include <macros.h>
#include "cdfs.h"
#include "cdfs_endian.h"
#include "cdfs_ops.h"

/** Standard CD-ROM block size */
#define BLOCK_SIZE  2048

/** Implicit node cache size
 *
 * More nodes can be actually cached if the files remain
 * opended.
 *
 */
#define NODE_CACHE_SIZE  200

#define NODES_BUCKETS  256

#define NODES_KEY_SRVC   0
#define NODES_KEY_INDEX  1

/** All root nodes have index 0 */
#define CDFS_SOME_ROOT  0

#define CDFS_NODE(node)  ((node) ? (cdfs_node_t *)(node)->data : NULL)
#define FS_NODE(node)    ((node) ? (node)->fs_node : NULL)

#define CDFS_STANDARD_IDENT  "CD001"

typedef enum {
	VOL_DESC_BOOT = 0,
	VOL_DESC_PRIMARY = 1,
	VOL_DESC_SUPPLEMENTARY = 2,
	VOL_DESC_VOL_PARTITION = 3,
	VOL_DESC_SET_TERMINATOR = 255
} vol_desc_type_t;

typedef struct {
	uint8_t system_ident[32];
	uint8_t ident[32];
} __attribute__((packed)) cdfs_vol_desc_boot_t;

typedef struct {
	uint8_t year[4];
	uint8_t mon[2];
	uint8_t day[2];
	
	uint8_t hour[2];
	uint8_t min[2];
	uint8_t sec[2];
	uint8_t msec[2];
	
	uint8_t offset;
} __attribute__((packed)) cdfs_datetime_t;

typedef struct {
	uint8_t year;  /**< Since 1900 */
	uint8_t mon;
	uint8_t day;
	
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	
	uint8_t offset;
} __attribute__((packed)) cdfs_timestamp_t;

typedef enum {
	DIR_FLAG_DIRECTORY = 2
} cdfs_dir_flag_t;

typedef struct {
	uint8_t length;
	uint8_t ea_length;
	
	uint32_t_lb lba;
	uint32_t_lb size;
	
	cdfs_timestamp_t timestamp;
	uint8_t flags;
	uint8_t unit_size;
	uint8_t gap_size;
	uint16_t_lb sequence_nr;
	
	uint8_t name_length;
	uint8_t name[];
} __attribute__((packed)) cdfs_dir_t;

typedef struct {
	uint8_t res0;
	
	uint8_t system_ident[32];
	uint8_t ident[32];
	
	uint64_t res1;
	uint32_t_lb lba_size;
	
	uint8_t res2[32];
	uint16_t_lb set_size;
	uint16_t_lb sequence_nr;
	
	uint16_t_lb block_size;
	uint32_t_lb path_table_size;
	
	uint32_t path_table_lsb;
	uint32_t opt_path_table_lsb;
	uint32_t path_table_msb;
	uint32_t opt_path_table_msb;
	
	cdfs_dir_t root_dir;
	uint8_t pad0;
	
	uint8_t set_ident[128];
	uint8_t publisher_ident[128];
	uint8_t preparer_ident[128];
	uint8_t app_ident[128];
	
	uint8_t copyright_file_ident[37];
	uint8_t abstract_file_ident[37];
	uint8_t biblio_file_ident[37];
	
	cdfs_datetime_t creation;
	cdfs_datetime_t modification;
	cdfs_datetime_t expiration;
	cdfs_datetime_t effective;
	
	uint8_t fs_version;
} __attribute__((packed)) cdfs_vol_desc_primary_t;

typedef struct {
	uint8_t type;
	uint8_t standard_ident[5];
	uint8_t version;
	union {
		cdfs_vol_desc_boot_t boot;
		cdfs_vol_desc_primary_t primary;
	} data;
} __attribute__((packed)) cdfs_vol_desc_t;

typedef enum {
	CDFS_NONE,
	CDFS_FILE,
	CDFS_DIRECTORY
} cdfs_dentry_type_t;

typedef struct {
	link_t link;       /**< Siblings list link */
	fs_index_t index;  /**< Node index */
	char *name;        /**< Dentry name */
} cdfs_dentry_t;

typedef uint32_t cdfs_lba_t;

typedef struct {
	fs_node_t *fs_node;       /**< FS node */
	fs_index_t index;         /**< Node index */
	service_id_t service_id;  /**< Service ID of block device */
	
	link_t nh_link;           /**< Nodes hash table link */
	cdfs_dentry_type_t type;  /**< Dentry type */
	
	unsigned int lnkcnt;      /**< Link count */
	uint32_t size;            /**< File size if type is CDFS_FILE */
	
	list_t cs_list;           /**< Child's siblings list */
	cdfs_lba_t lba;           /**< LBA of data on disk */
	bool processed;           /**< If all children have been read */
	unsigned int opened;      /**< Opened count */
} cdfs_node_t;

/** Shared index of nodes */
static fs_index_t cdfs_index = 1;

/** Number of currently cached nodes */
static size_t nodes_cached = 0;

/** Hash table of all cdfs nodes */
static hash_table_t nodes;

static hash_index_t nodes_hash(unsigned long key[])
{
	return key[NODES_KEY_INDEX] % NODES_BUCKETS;
}

static int nodes_compare(unsigned long key[], hash_count_t keys, link_t *item)
{
	cdfs_node_t *node =
	    hash_table_get_instance(item, cdfs_node_t, nh_link);
	
	switch (keys) {
	case 1:
		return (node->service_id == key[NODES_KEY_SRVC]);
	case 2:
		return ((node->service_id == key[NODES_KEY_SRVC]) &&
		    (node->index == key[NODES_KEY_INDEX]));
	default:
		assert((keys == 1) || (keys == 2));
	}
	
	return 0;
}

static void nodes_remove_callback(link_t *item)
{
	cdfs_node_t *node =
	    hash_table_get_instance(item, cdfs_node_t, nh_link);
	
	assert(node->type == CDFS_DIRECTORY);
	
	link_t *link;
	while ((link = list_first(&node->cs_list)) != NULL) {
		cdfs_dentry_t *dentry = list_get_instance(link, cdfs_dentry_t, link);
		list_remove(&dentry->link);
		free(dentry);
	}
	
	free(node->fs_node);
	free(node);
}

/** Nodes hash table operations */
static hash_table_operations_t nodes_ops = {
	.hash = nodes_hash,
	.compare = nodes_compare,
	.remove_callback = nodes_remove_callback
};

static int cdfs_node_get(fs_node_t **rfn, service_id_t service_id,
    fs_index_t index)
{
	unsigned long key[] = {
		[NODES_KEY_SRVC] = service_id,
		[NODES_KEY_INDEX] = index
	};
	
	link_t *link = hash_table_find(&nodes, key);
	if (link) {
		cdfs_node_t *node =
		    hash_table_get_instance(link, cdfs_node_t, nh_link);
		
		*rfn = FS_NODE(node);
	} else
		*rfn = NULL;
	
	return EOK;
}

static int cdfs_root_get(fs_node_t **rfn, service_id_t service_id)
{
	return cdfs_node_get(rfn, service_id, CDFS_SOME_ROOT);
}

static void cdfs_node_initialize(cdfs_node_t *node)
{
	node->fs_node = NULL;
	node->index = 0;
	node->service_id = 0;
	node->type = CDFS_NONE;
	node->lnkcnt = 0;
	node->size = 0;
	node->lba = 0;
	node->processed = false;
	node->opened = 0;
	
	link_initialize(&node->nh_link);
	list_initialize(&node->cs_list);
}

static int create_node(fs_node_t **rfn, service_id_t service_id, int lflag,
    fs_index_t index)
{
	assert((lflag & L_FILE) ^ (lflag & L_DIRECTORY));
	
	cdfs_node_t *node = malloc(sizeof(cdfs_node_t));
	if (!node)
		return ENOMEM;
	
	cdfs_node_initialize(node);
	
	node->fs_node = malloc(sizeof(fs_node_t));
	if (!node->fs_node) {
		free(node);
		return ENOMEM;
	}
	
	fs_node_initialize(node->fs_node);
	node->fs_node->data = node;
	
	fs_node_t *rootfn;
	int rc = cdfs_root_get(&rootfn, service_id);
	
	assert(rc == EOK);
	
	if (!rootfn)
		node->index = CDFS_SOME_ROOT;
	else
		node->index = index;
	
	node->service_id = service_id;
	
	if (lflag & L_DIRECTORY)
		node->type = CDFS_DIRECTORY;
	else
		node->type = CDFS_FILE;
	
	/* Insert the new node into the nodes hash table. */
	unsigned long key[] = {
		[NODES_KEY_SRVC] = node->service_id,
		[NODES_KEY_INDEX] = node->index
	};
	
	hash_table_insert(&nodes, key, &node->nh_link);
	
	*rfn = FS_NODE(node);
	nodes_cached++;
	
	return EOK;
}

static int link_node(fs_node_t *pfn, fs_node_t *fn, const char *name)
{
	cdfs_node_t *parent = CDFS_NODE(pfn);
	cdfs_node_t *node = CDFS_NODE(fn);
	
	assert(parent->type == CDFS_DIRECTORY);
	
	/* Check for duplicate entries */
	list_foreach(parent->cs_list, link) {
		cdfs_dentry_t *dentry =
		    list_get_instance(link, cdfs_dentry_t, link);
		
		if (str_cmp(dentry->name, name) == 0)
			return EEXIST;
	}
	
	/* Allocate and initialize the dentry */
	cdfs_dentry_t *dentry = malloc(sizeof(cdfs_dentry_t));
	if (!dentry)
		return ENOMEM;
	
	/* Populate and link the new dentry */
	dentry->name = str_dup(name);
	if (dentry->name == NULL) {
		free(dentry);
		return ENOMEM;
	}
	
	link_initialize(&dentry->link);
	dentry->index = node->index;
	
	node->lnkcnt++;
	list_append(&dentry->link, &parent->cs_list);
	
	return EOK;
}

static bool cdfs_readdir(service_id_t service_id, fs_node_t *fs_node)
{
	cdfs_node_t *node = CDFS_NODE(fs_node);
	assert(node);
	
	if (node->processed)
		return true;
	
	uint32_t blocks = node->size / BLOCK_SIZE;
	if ((node->size % BLOCK_SIZE) != 0)
		blocks++;
	
	for (uint32_t i = 0; i < blocks; i++) {
		block_t *block;
		int rc = block_get(&block, service_id, node->lba + i, BLOCK_FLAGS_NONE);
		if (rc != EOK)
			return false;
		
		cdfs_dir_t *dir = (cdfs_dir_t *) block->data;
		
		// FIXME: skip '.' and '..'
		
		for (size_t offset = 0;
		    (dir->length != 0) && (offset < BLOCK_SIZE);
		    offset += dir->length) {
			dir = (cdfs_dir_t *) (block->data + offset);
			
			cdfs_dentry_type_t dentry_type;
			if (dir->flags & DIR_FLAG_DIRECTORY)
				dentry_type = CDFS_DIRECTORY;
			else
				dentry_type = CDFS_FILE;
			
			// FIXME: hack - indexing by dentry byte offset on disc
			
			fs_node_t *fn;
			int rc = create_node(&fn, service_id, dentry_type,
			    (node->lba + i) * BLOCK_SIZE + offset);
			if ((rc != EOK) || (fn == NULL))
				return false;
			
			cdfs_node_t *cur = CDFS_NODE(fn);
			cur->lba = uint32_lb(dir->lba);
			cur->size = uint32_lb(dir->size);
			
			char *name = (char *) malloc(dir->name_length + 1);
			if (name == NULL)
				return false;
			
			memcpy(name, dir->name, dir->name_length);
			name[dir->name_length] = 0;
			
			// FIXME: check return value
			
			link_node(fs_node, fn, name);
			free(name);
			
			if (dentry_type == CDFS_FILE)
				cur->processed = true;
		}
		
		block_put(block);
	}
	
	node->processed = true;
	return true;
}

static fs_node_t *get_uncached_node(service_id_t service_id, fs_index_t index)
{
	cdfs_lba_t lba = index / BLOCK_SIZE;
	size_t offset = index % BLOCK_SIZE;
	
	block_t *block;
	int rc = block_get(&block, service_id, lba, BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return NULL;
	
	cdfs_dir_t *dir = (cdfs_dir_t *) (block->data + offset);
	
	cdfs_dentry_type_t dentry_type;
	if (dir->flags & DIR_FLAG_DIRECTORY)
		dentry_type = CDFS_DIRECTORY;
	else
		dentry_type = CDFS_FILE;
	
	fs_node_t *fn;
	rc = create_node(&fn, service_id, dentry_type, index);
	if ((rc != EOK) || (fn == NULL))
		return NULL;
	
	cdfs_node_t *node = CDFS_NODE(fn);
	node->lba = uint32_lb(dir->lba);
	node->size = uint32_lb(dir->size);
	node->lnkcnt = 1;
	
	if (dentry_type == CDFS_FILE)
		node->processed = true;
	
	block_put(block);
	return fn;
}

static fs_node_t *get_cached_node(service_id_t service_id, fs_index_t index)
{
	unsigned long key[] = {
		[NODES_KEY_SRVC] = service_id,
		[NODES_KEY_INDEX] = index
	};
	
	link_t *link = hash_table_find(&nodes, key);
	if (link) {
		cdfs_node_t *node =
		    hash_table_get_instance(link, cdfs_node_t, nh_link);
		return FS_NODE(node);
	}
	
	return get_uncached_node(service_id, index);
}

static int cdfs_match(fs_node_t **fn, fs_node_t *pfn, const char *component)
{
	cdfs_node_t *parent = CDFS_NODE(pfn);
	
	if (!parent->processed) {
		int rc = cdfs_readdir(parent->service_id, pfn);
		if (rc != EOK)
			return rc;
	}
	
	list_foreach(parent->cs_list, link) {
		cdfs_dentry_t *dentry =
		    list_get_instance(link, cdfs_dentry_t, link);
		
		if (str_cmp(dentry->name, component) == 0) {
			*fn = get_cached_node(parent->service_id, dentry->index);
			return EOK;
		}
	}
	
	*fn = NULL;
	return EOK;
}

static int cdfs_node_open(fs_node_t *fn)
{
	cdfs_node_t *node = CDFS_NODE(fn);
	
	if (!node->processed)
		cdfs_readdir(node->service_id, fn);
	
	node->opened++;
	return EOK;
}

static int cdfs_node_put(fs_node_t *fn)
{
	/* Nothing to do */
	return EOK;
}

static int cdfs_create_node(fs_node_t **fn, service_id_t service_id, int lflag)
{
	/* Read-only */
	return ENOTSUP;
}

static int cdfs_destroy_node(fs_node_t *fn)
{
	/* Read-only */
	return ENOTSUP;
}

static int cdfs_link_node(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	/* Read-only */
	return ENOTSUP;
}

static int cdfs_unlink_node(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	/* Read-only */
	return ENOTSUP;
}

static int cdfs_has_children(bool *has_children, fs_node_t *fn)
{
	cdfs_node_t *node = CDFS_NODE(fn);
	
	if ((node->type == CDFS_DIRECTORY) && (!node->processed))
		cdfs_readdir(node->service_id, fn);
	
	*has_children = !list_empty(&node->cs_list);
	return EOK;
}

static fs_index_t cdfs_index_get(fs_node_t *fn)
{
	cdfs_node_t *node = CDFS_NODE(fn);
	return node->index;
}

static aoff64_t cdfs_size_get(fs_node_t *fn)
{
	cdfs_node_t *node = CDFS_NODE(fn);
	return node->size;
}

static unsigned int cdfs_lnkcnt_get(fs_node_t *fn)
{
	cdfs_node_t *node = CDFS_NODE(fn);
	return node->lnkcnt;
}

static bool cdfs_is_directory(fs_node_t *fn)
{
	cdfs_node_t *node = CDFS_NODE(fn);
	return (node->type == CDFS_DIRECTORY);
}

static bool cdfs_is_file(fs_node_t *fn)
{
	cdfs_node_t *node = CDFS_NODE(fn);
	return (node->type == CDFS_FILE);
}

static service_id_t cdfs_service_get(fs_node_t *fn)
{
	return 0;
}

libfs_ops_t cdfs_libfs_ops = {
	.root_get = cdfs_root_get,
	.match = cdfs_match,
	.node_get = cdfs_node_get,
	.node_open = cdfs_node_open,
	.node_put = cdfs_node_put,
	.create = cdfs_create_node,
	.destroy = cdfs_destroy_node,
	.link = cdfs_link_node,
	.unlink = cdfs_unlink_node,
	.has_children = cdfs_has_children,
	.index_get = cdfs_index_get,
	.size_get = cdfs_size_get,
	.lnkcnt_get = cdfs_lnkcnt_get,
	.is_directory = cdfs_is_directory,
	.is_file = cdfs_is_file,
	.service_get = cdfs_service_get
};

static bool iso_readfs(service_id_t service_id, fs_node_t *rfn,
    cdfs_lba_t altroot)
{
	/* First 16 blocks of isofs are empty */
	block_t *block;
	int rc = block_get(&block, service_id, altroot + 16, BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return false;
	
	cdfs_vol_desc_t *vol_desc = (cdfs_vol_desc_t *) block->data;
	
	/*
	 * Test for primary volume descriptor
	 * and standard compliance.
	 */
	if ((vol_desc->type != VOL_DESC_PRIMARY) ||
	    (bcmp(vol_desc->standard_ident, CDFS_STANDARD_IDENT, 5) != 0) ||
	    (vol_desc->version != 1)) {
		block_put(block);
		return false;
	}
	
	uint16_t set_size = uint16_lb(vol_desc->data.primary.set_size);
	if (set_size > 1) {
		/*
		 * Technically, we don't support multi-disc sets.
		 * But one can encounter erroneously mastered
		 * images in the wild and it might actually work
		 * for the first disc in the set.
		 */
	}
	
	uint16_t sequence_nr = uint16_lb(vol_desc->data.primary.sequence_nr);
	if (sequence_nr != 1) {
		/*
		 * We only support the first disc
		 * in multi-disc sets.
		 */
		block_put(block);
		return false;
	}
	
	uint16_t block_size = uint16_lb(vol_desc->data.primary.block_size);
	if (block_size != BLOCK_SIZE) {
		block_put(block);
		return false;
	}
	
	// TODO: implement path table support
	
	cdfs_node_t *node = CDFS_NODE(rfn);
	node->lba = uint32_lb(vol_desc->data.primary.root_dir.lba);
	node->size = uint32_lb(vol_desc->data.primary.root_dir.size);
	
	if (!cdfs_readdir(service_id, rfn)) {
		block_put(block);
		return false;
	}
	
	block_put(block);
	return true;
}

/* Mount a session with session start offset
 *
 */
static bool cdfs_instance_init(service_id_t service_id, cdfs_lba_t altroot)
{
	/* Create root node */
	fs_node_t *rfn;
	int rc = create_node(&rfn, service_id, L_DIRECTORY, cdfs_index++);
	
	if ((rc != EOK) || (!rfn))
		return false;
	
	/* FS root is not linked */
	CDFS_NODE(rfn)->lnkcnt = 0;
	CDFS_NODE(rfn)->lba = 0;
	CDFS_NODE(rfn)->processed = false;
	
	/* Check if there is cdfs in given session */
	if (!iso_readfs(service_id, rfn, altroot)) {
		// XXX destroy node
		return false;
	}
	
	return true;
}

static int cdfs_mounted(service_id_t service_id, const char *opts,
    fs_index_t *index, aoff64_t *size, unsigned int *lnkcnt)
{
	/* Initialize the block layer */
	int rc = block_init(EXCHANGE_SERIALIZE, service_id, BLOCK_SIZE);
	if (rc != EOK)
		return rc;
	
	cdfs_lba_t altroot = 0;
	
	if (str_lcmp(opts, "altroot=", 8) == 0) {
		/* User-defined alternative root on a multi-session disk */
		if (str_uint32_t(opts + 8, NULL, 0, false, &altroot) != EOK)
			altroot = 0;
	} else {
		/* Read TOC and find the last session */
		toc_block_t *toc = block_get_toc(service_id, 1);
		if ((toc != NULL) && (uint16_t_be2host(toc->size) == 10)) {
			altroot = uint32_t_be2host(toc->first_lba);
			free(toc);
		}
	}
	
	/* Initialize the block cache */
	rc = block_cache_init(service_id, BLOCK_SIZE, 0, CACHE_MODE_WT);
	if (rc != EOK) {
		block_fini(service_id);
		return rc;
	}
	
	/* Check if this device is not already mounted */
	fs_node_t *rootfn;
	rc = cdfs_root_get(&rootfn, service_id);
	if ((rc == EOK) && (rootfn)) {
		cdfs_node_put(rootfn);
		block_cache_fini(service_id);
		block_fini(service_id);
		
		return EEXIST;
	}
	
	/* Initialize cdfs instance */
	if (!cdfs_instance_init(service_id, altroot)) {
		block_cache_fini(service_id);
		block_fini(service_id);
		
		return ENOMEM;
	}
	
	rc = cdfs_root_get(&rootfn, service_id);
	assert(rc == EOK);
	
	cdfs_node_t *root = CDFS_NODE(rootfn);
	*index = root->index;
	*size = root->size;
	*lnkcnt = root->lnkcnt;
	
	return EOK;
}

static void cdfs_instance_done(service_id_t service_id)
{
	unsigned long key[] = {
		[NODES_KEY_SRVC] = service_id
	};
	
	hash_table_remove(&nodes, key, 1);
	block_cache_fini(service_id);
	block_fini(service_id);
}

static int cdfs_unmounted(service_id_t service_id)
{
	cdfs_instance_done(service_id);
	return EOK;
}

static int cdfs_read(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *rbytes)
{
	unsigned long key[] = {
		[NODES_KEY_SRVC] = service_id,
		[NODES_KEY_INDEX] = index
	};
	
	link_t *link = hash_table_find(&nodes, key);
	if (link == NULL)
		return ENOENT;
	
	cdfs_node_t *node =
	    hash_table_get_instance(link, cdfs_node_t, nh_link);
	
	if (!node->processed) {
		int rc = cdfs_readdir(service_id, FS_NODE(node));
		if (rc != EOK)
			return rc;
	}
	
	ipc_callid_t callid;
	size_t len;
	if (!async_data_read_receive(&callid, &len)) {
		async_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	if (node->type == CDFS_FILE) {
		if (pos >= node->size) {
			*rbytes = 0;
			async_data_read_finalize(callid, NULL, 0);
		} else {
			cdfs_lba_t lba = pos / BLOCK_SIZE;
			size_t offset = pos % BLOCK_SIZE;
			
			*rbytes = min(len, BLOCK_SIZE - offset);
			*rbytes = min(*rbytes, node->size - pos);
			
			block_t *block;
			int rc = block_get(&block, service_id, node->lba + lba,
			    BLOCK_FLAGS_NONE);
			if (rc != EOK) {
				async_answer_0(callid, rc);
				return rc;
			}
			
			async_data_read_finalize(callid, block->data + offset,
			    *rbytes);
			rc = block_put(block);
			if (rc != EOK)
				return rc;
		}
	} else {
		link_t *link = list_nth(&node->cs_list, pos);
		if (link == NULL) {
			async_answer_0(callid, ENOENT);
			return ENOENT;
		}
		
		cdfs_dentry_t *dentry =
		    list_get_instance(link, cdfs_dentry_t, link);
		
		*rbytes = 1;
		async_data_read_finalize(callid, dentry->name,
		    str_size(dentry->name) + 1);
	}
	
	return EOK;
}

static int cdfs_write(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *wbytes, aoff64_t *nsize)
{
	/*
	 * As cdfs is a read-only filesystem,
	 * the operation is not supported.
	 */
	
	return ENOTSUP;
}

static int cdfs_truncate(service_id_t service_id, fs_index_t index,
    aoff64_t size)
{
	/*
	 * As cdfs is a read-only filesystem,
	 * the operation is not supported.
	 */
	
	return ENOTSUP;
}

static void cleanup_cache(service_id_t service_id)
{
	if (nodes_cached > NODE_CACHE_SIZE) {
		size_t remove = nodes_cached - NODE_CACHE_SIZE;
		
		// FIXME: this accesses the internals of the hash table
		//        and should be rewritten in a clean way
		
		for (hash_index_t chain = 0; chain < nodes.entries; chain++) {
			for (link_t *link = nodes.entry[chain].head.next;
			    link != &nodes.entry[chain].head;
			    link = link->next) {
				if (remove == 0)
					return;
				
				cdfs_node_t *node =
				    hash_table_get_instance(link, cdfs_node_t, nh_link);
				if (node->opened == 0) {
					link_t *tmp = link;
					link = link->prev;
					
					list_remove(tmp);
					nodes.op->remove_callback(tmp);
					nodes_cached--;
					remove--;
					
					continue;
				}
			}
		}
	}
}

static int cdfs_close(service_id_t service_id, fs_index_t index)
{
	/* Root node is always in memory */
	if (index == 0)
		return EOK;
	
	unsigned long key[] = {
		[NODES_KEY_SRVC] = service_id,
		[NODES_KEY_INDEX] = index
	};
	
	link_t *link = hash_table_find(&nodes, key);
	if (link == 0)
		return ENOENT;
	
	cdfs_node_t *node =
	    hash_table_get_instance(link, cdfs_node_t, nh_link);
	
	assert(node->opened > 0);
	
	node->opened--;
	cleanup_cache(service_id);
	
	return EOK;
}

static int cdfs_destroy(service_id_t service_id, fs_index_t index)
{
	/*
	 * As cdfs is a read-only filesystem,
	 * the operation is not supported.
	 */
	
	return ENOTSUP;
}

static int cdfs_sync(service_id_t service_id, fs_index_t index)
{
	/*
	 * As cdfs is a read-only filesystem,
	 * the sync operation is a no-op.
	 */
	
	return EOK;
}

vfs_out_ops_t cdfs_ops = {
	.mounted = cdfs_mounted,
	.unmounted = cdfs_unmounted,
	.read = cdfs_read,
	.write = cdfs_write,
	.truncate = cdfs_truncate,
	.close = cdfs_close,
	.destroy = cdfs_destroy,
	.sync = cdfs_sync
};

/** Initialize the cdfs server
 *
 */
bool cdfs_init(void)
{
	if (!hash_table_create(&nodes, NODES_BUCKETS, 2, &nodes_ops))
		return false;
	
	return true;
}

/**
 * @}
 */
