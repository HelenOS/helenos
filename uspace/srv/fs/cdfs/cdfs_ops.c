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

#include "cdfs_ops.h"
#include <stdbool.h>
#include <adt/list.h>
#include <adt/hash_table.h>
#include <adt/hash.h>
#include <mem.h>
#include <loc.h>
#include <libfs.h>
#include <errno.h>
#include <block.h>
#include <scsi/mmc.h>
#include <stdlib.h>
#include <str.h>
#include <byteorder.h>
#include <macros.h>
#include <unaligned.h>

#include "cdfs.h"
#include "cdfs_endian.h"

/** Standard CD-ROM block size */
#define BLOCK_SIZE  2048

#define NODE_CACHE_SIZE 200

/** All root nodes have index 0 */
#define CDFS_SOME_ROOT  0

#define CDFS_NODE(node)  ((node) ? (cdfs_node_t *)(node)->data : NULL)
#define FS_NODE(node)    ((node) ? (node)->fs_node : NULL)

#define CDFS_STANDARD_IDENT  "CD001"

enum {
	CDFS_NAME_CURDIR = '\x00',
	CDFS_NAME_PARENTDIR = '\x01'
};

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

/** Directory record */
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

/** Directory record for the root directory */
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
	uint8_t name[1];
} __attribute__((packed)) cdfs_root_dir_t;

typedef struct {
	uint8_t flags; /* reserved in primary */
	
	uint8_t system_ident[32];
	uint8_t ident[32];
	
	uint64_t res1;
	uint32_t_lb lba_size;
	
	uint8_t esc_seq[32]; /* reserved in primary */
	uint16_t_lb set_size;
	uint16_t_lb sequence_nr;
	
	uint16_t_lb block_size;
	uint32_t_lb path_table_size;
	
	uint32_t path_table_lsb;
	uint32_t opt_path_table_lsb;
	uint32_t path_table_msb;
	uint32_t opt_path_table_msb;
	
	cdfs_root_dir_t root_dir;
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
} __attribute__((packed)) cdfs_vol_desc_prisec_t;

typedef struct {
	uint8_t type;
	uint8_t standard_ident[5];
	uint8_t version;
	union {
		cdfs_vol_desc_boot_t boot;
		cdfs_vol_desc_prisec_t prisec;
	} data;
} __attribute__((packed)) cdfs_vol_desc_t;

typedef enum {
	/** ASCII character set / encoding (base ISO 9660) */
	enc_ascii,
	/** UCS-2 character set / encoding (Joliet) */
	enc_ucs2
} cdfs_enc_t;

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
	link_t link;		  /**< Link to list of all instances */
	service_id_t service_id;  /**< Service ID of block device */
	cdfs_enc_t enc;		  /**< Filesystem string encoding */
	char *vol_ident;	  /**< Volume identifier */
} cdfs_t;

typedef struct {
	fs_node_t *fs_node;       /**< FS node */
	fs_index_t index;         /**< Node index */
	cdfs_t *fs;		  /**< File system */
	
	ht_link_t nh_link;        /**< Nodes hash table link */
	cdfs_dentry_type_t type;  /**< Dentry type */
	
	unsigned int lnkcnt;      /**< Link count */
	uint32_t size;            /**< File size if type is CDFS_FILE */
	
	list_t cs_list;           /**< Child's siblings list */
	cdfs_lba_t lba;           /**< LBA of data on disk */
	bool processed;           /**< If all children have been read */
	unsigned int opened;      /**< Opened count */
} cdfs_node_t;

/** String encoding */
enum {
	/** ASCII - standard ISO 9660 */
	ucs2_esc_seq_no = 3,
	/** USC-2 - Joliet */
	ucs2_esc_seq_len = 3
};

/** Joliet SVD UCS-2 escape sequences */
static uint8_t ucs2_esc_seq[ucs2_esc_seq_no][ucs2_esc_seq_len] = {
	{ 0x25, 0x2f, 0x40 },
	{ 0x25, 0x2f, 0x43 },
	{ 0x25, 0x2f, 0x45 }
};

/** List of all instances */
static LIST_INITIALIZE(cdfs_instances);

/** Shared index of nodes */
static fs_index_t cdfs_index = 1;

/** Number of currently cached nodes */
static size_t nodes_cached = 0;

/** Hash table of all cdfs nodes */
static hash_table_t nodes;

/* 
 * Hash table support functions.
 */

typedef struct {
	service_id_t service_id;
    fs_index_t index;
} ht_key_t;

static size_t nodes_key_hash(void *k)
{
	ht_key_t *key = (ht_key_t*)k;
	return hash_combine(key->service_id, key->index);
}

static size_t nodes_hash(const ht_link_t *item)
{
	cdfs_node_t *node = hash_table_get_inst(item, cdfs_node_t, nh_link);
	return hash_combine(node->fs->service_id, node->index);
}

static bool nodes_key_equal(void *k, const ht_link_t *item)
{
	cdfs_node_t *node = hash_table_get_inst(item, cdfs_node_t, nh_link);
	ht_key_t *key = (ht_key_t*)k;
	
	return key->service_id == node->fs->service_id && key->index == node->index;
}

static void nodes_remove_callback(ht_link_t *item)
{
	cdfs_node_t *node = hash_table_get_inst(item, cdfs_node_t, nh_link);
	
	if (node->type == CDFS_DIRECTORY) {
		link_t *link;
		while ((link = list_first(&node->cs_list)) != NULL) {
			cdfs_dentry_t *dentry = list_get_instance(link, cdfs_dentry_t, link);
			list_remove(&dentry->link);
			free(dentry);
		}
	}
	
	free(node->fs_node);
	free(node);
}

/** Nodes hash table operations */
static hash_table_ops_t nodes_ops = {
	.hash = nodes_hash,
	.key_hash = nodes_key_hash,
	.key_equal = nodes_key_equal,
	.equal = NULL,
	.remove_callback = nodes_remove_callback
};

static errno_t cdfs_node_get(fs_node_t **rfn, service_id_t service_id,
    fs_index_t index)
{
	ht_key_t key = {
		.index = index,
		.service_id = service_id
	};
	
	ht_link_t *link = hash_table_find(&nodes, &key);
	if (link) {
		cdfs_node_t *node =
		    hash_table_get_inst(link, cdfs_node_t, nh_link);
		
		*rfn = FS_NODE(node);
	} else
		*rfn = NULL;
	
	return EOK;
}

static errno_t cdfs_root_get(fs_node_t **rfn, service_id_t service_id)
{
	return cdfs_node_get(rfn, service_id, CDFS_SOME_ROOT);
}

static void cdfs_node_initialize(cdfs_node_t *node)
{
	node->fs_node = NULL;
	node->index = 0;
	node->fs = NULL;
	node->type = CDFS_NONE;
	node->lnkcnt = 0;
	node->size = 0;
	node->lba = 0;
	node->processed = false;
	node->opened = 0;
	
	list_initialize(&node->cs_list);
}

static errno_t create_node(fs_node_t **rfn, cdfs_t *fs, int lflag,
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
	errno_t rc = cdfs_root_get(&rootfn, fs->service_id);
	
	assert(rc == EOK);
	
	if (!rootfn)
		node->index = CDFS_SOME_ROOT;
	else
		node->index = index;
	
	node->fs = fs;
	
	if (lflag & L_DIRECTORY)
		node->type = CDFS_DIRECTORY;
	else
		node->type = CDFS_FILE;
	
	/* Insert the new node into the nodes hash table. */
	hash_table_insert(&nodes, &node->nh_link);
	
	*rfn = FS_NODE(node);
	nodes_cached++;
	
	return EOK;
}

static errno_t link_node(fs_node_t *pfn, fs_node_t *fn, const char *name)
{
	cdfs_node_t *parent = CDFS_NODE(pfn);
	cdfs_node_t *node = CDFS_NODE(fn);
	
	assert(parent->type == CDFS_DIRECTORY);
	
	/* Check for duplicate entries */
	list_foreach(parent->cs_list, link, cdfs_dentry_t, dentry) {
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

/** Decode CDFS string.
 *
 * @param data	Pointer to string data
 * @param dsize	Size of data in bytes
 * @param enc	String encoding
 * @return	Decoded string
 */
static char *cdfs_decode_str(void *data, size_t dsize, cdfs_enc_t enc)
{
	errno_t rc;
	char *str;
	uint16_t *buf;
	
	switch (enc) {
	case enc_ascii:
		str = malloc(dsize + 1);
		if (str == NULL)
			return NULL;
		memcpy(str, data, dsize);
		str[dsize] = '\0';
		break;
	case enc_ucs2:
		buf = calloc(dsize + 2, 1);
		if (buf == NULL)
			return NULL;
		
		size_t i;
		for (i = 0; i < dsize / sizeof(uint16_t); i++) {
			buf[i] = uint16_t_be2host(
			    ((unaligned_uint16_t *)data)[i]);
		}
		
		size_t dstr_size = dsize / sizeof(uint16_t) * 4 + 1;
		str = malloc(dstr_size);
		if (str == NULL)
			return NULL;
		
		rc = utf16_to_str(str, dstr_size, buf);
		free(buf);
		
		if (rc != EOK)
			return NULL;
		break;
	default:
		assert(false);
		str = NULL;
	}
	
	return str;
}

/** Decode file name.
 *
 * @param data	File name buffer
 * @param dsize	Fine name buffer size
 * @param enc   Encoding
 * @param dtype	Directory entry type
 * @return	Decoded file name (allocated string)
 */
static char *cdfs_decode_name(void *data, size_t dsize, cdfs_enc_t enc,
    cdfs_dentry_type_t dtype)
{
	char *name;
	char *dot;
	char *scolon;
	
	name = cdfs_decode_str(data, dsize, enc);
	if (name == NULL)
		return NULL;
	
	if (dtype == CDFS_DIRECTORY)
		return name;
	
	dot = str_chr(name, '.');
	
	if (dot != NULL) {
		scolon = str_chr(dot, ';');
		if (scolon != NULL) {
			/* Trim version part */
			*scolon = '\0';
		}
	
		/* If the extension is an empty string, trim the dot separator. */
		if (dot[1] == '\0')
			*dot = '\0';
	}
	
	return name;
}

/** Decode volume identifier.
 *
 * @param data	Volume identifier buffer
 * @param dsize	Volume identifier buffer size
 * @param enc   Encoding
 * @return	Decoded volume identifier (allocated string)
 */
static char *cdfs_decode_vol_ident(void *data, size_t dsize, cdfs_enc_t enc)
{
	char *ident;
	size_t i;
	
	ident = cdfs_decode_str(data, dsize, enc);
	if (ident == NULL)
		return NULL;
	
	/* Trim trailing spaces */
	i = str_size(ident);
	while (i > 0 && ident[i - 1] == ' ')
		--i;
	ident[i] = '\0';
	
	return ident;
}

static errno_t cdfs_readdir(cdfs_t *fs, fs_node_t *fs_node)
{
	cdfs_node_t *node = CDFS_NODE(fs_node);
	assert(node);
	
	if (node->processed)
		return EOK;
	
	uint32_t blocks = node->size / BLOCK_SIZE;
	if ((node->size % BLOCK_SIZE) != 0)
		blocks++;
	
	for (uint32_t i = 0; i < blocks; i++) {
		block_t *block;
		errno_t rc = block_get(&block, fs->service_id, node->lba + i, BLOCK_FLAGS_NONE);
		if (rc != EOK)
			return rc;
		
		cdfs_dir_t *dir;
		
		for (size_t offset = 0; offset < BLOCK_SIZE;
		    offset += dir->length) {
			dir = (cdfs_dir_t *) (block->data + offset);
			if (dir->length == 0)
				break;
			if (offset + dir->length > BLOCK_SIZE) {
				/* XXX Incorrect FS structure */
				break;
			}
			
			cdfs_dentry_type_t dentry_type;
			if (dir->flags & DIR_FLAG_DIRECTORY)
				dentry_type = CDFS_DIRECTORY;
			else
				dentry_type = CDFS_FILE;
			
			/* Skip special entries */
			
			if (dir->name_length == 1 &&
			    dir->name[0] == CDFS_NAME_CURDIR)
				continue;
			if (dir->name_length == 1 &&
			    dir->name[0] == CDFS_NAME_PARENTDIR)
				continue;
			
			// FIXME: hack - indexing by dentry byte offset on disc
			
			fs_node_t *fn;
			errno_t rc = create_node(&fn, fs, dentry_type,
			    (node->lba + i) * BLOCK_SIZE + offset);
			if (rc != EOK)
				return rc;

			assert(fn != NULL);
			
			cdfs_node_t *cur = CDFS_NODE(fn);
			cur->lba = uint32_lb(dir->lba);
			cur->size = uint32_lb(dir->size);
			
			char *name = cdfs_decode_name(dir->name,
			    dir->name_length, node->fs->enc, dentry_type);
			if (name == NULL)
				return EIO;
			
			// FIXME: check return value
			
			link_node(fs_node, fn, name);
			free(name);
			
			if (dentry_type == CDFS_FILE)
				cur->processed = true;
		}
		
		block_put(block);
	}
	
	node->processed = true;
	return EOK;
}

static fs_node_t *get_uncached_node(cdfs_t *fs, fs_index_t index)
{
	cdfs_lba_t lba = index / BLOCK_SIZE;
	size_t offset = index % BLOCK_SIZE;
	
	block_t *block;
	errno_t rc = block_get(&block, fs->service_id, lba, BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return NULL;
	
	cdfs_dir_t *dir = (cdfs_dir_t *) (block->data + offset);
	
	cdfs_dentry_type_t dentry_type;
	if (dir->flags & DIR_FLAG_DIRECTORY)
		dentry_type = CDFS_DIRECTORY;
	else
		dentry_type = CDFS_FILE;
	
	fs_node_t *fn;
	rc = create_node(&fn, fs, dentry_type, index);
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

static fs_node_t *get_cached_node(cdfs_t *fs, fs_index_t index)
{
	ht_key_t key = {
		.index = index,
		.service_id = fs->service_id
	};
	
	ht_link_t *link = hash_table_find(&nodes, &key);
	if (link) {
		cdfs_node_t *node =
		    hash_table_get_inst(link, cdfs_node_t, nh_link);
		return FS_NODE(node);
	}
	
	return get_uncached_node(fs, index);
}

static errno_t cdfs_match(fs_node_t **fn, fs_node_t *pfn, const char *component)
{
	cdfs_node_t *parent = CDFS_NODE(pfn);
	
	if (!parent->processed) {
		errno_t rc = cdfs_readdir(parent->fs, pfn);
		if (rc != EOK)
			return rc;
	}
	
	list_foreach(parent->cs_list, link, cdfs_dentry_t, dentry) {
		if (str_cmp(dentry->name, component) == 0) {
			*fn = get_cached_node(parent->fs, dentry->index);
			return EOK;
		}
	}
	
	*fn = NULL;
	return EOK;
}

static errno_t cdfs_node_open(fs_node_t *fn)
{
	cdfs_node_t *node = CDFS_NODE(fn);
	
	if (!node->processed)
		cdfs_readdir(node->fs, fn);
	
	node->opened++;
	return EOK;
}

static errno_t cdfs_node_put(fs_node_t *fn)
{
	/* Nothing to do */
	return EOK;
}

static errno_t cdfs_create_node(fs_node_t **fn, service_id_t service_id, int lflag)
{
	/* Read-only */
	return ENOTSUP;
}

static errno_t cdfs_destroy_node(fs_node_t *fn)
{
	/* Read-only */
	return ENOTSUP;
}

static errno_t cdfs_link_node(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	/* Read-only */
	return ENOTSUP;
}

static errno_t cdfs_unlink_node(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	/* Read-only */
	return ENOTSUP;
}

static errno_t cdfs_has_children(bool *has_children, fs_node_t *fn)
{
	cdfs_node_t *node = CDFS_NODE(fn);
	
	if ((node->type == CDFS_DIRECTORY) && (!node->processed))
		cdfs_readdir(node->fs, fn);
	
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

static errno_t cdfs_size_block(service_id_t service_id, uint32_t *size)
{
	*size = BLOCK_SIZE;
	
	return EOK; 
}

static errno_t cdfs_total_block_count(service_id_t service_id, uint64_t *count)
{
	*count = 0;
	
	return EOK;
}

static errno_t cdfs_free_block_count(service_id_t service_id, uint64_t *count)
{
	*count = 0;
	
	return EOK;
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
	.service_get = cdfs_service_get,
	.size_block = cdfs_size_block,
	.total_block_count = cdfs_total_block_count,
	.free_block_count = cdfs_free_block_count
};

/** Verify that escape sequence corresonds to one of the allowed encoding
 * escape sequences allowed for Joliet. */
static errno_t cdfs_verify_joliet_esc_seq(uint8_t *seq)
{
	size_t i, j, k;
	bool match;
	
	i = 0;
	while (i + ucs2_esc_seq_len <= 32) {
		if (seq[i] == 0)
			break;
		
		for (j = 0; j < ucs2_esc_seq_no; j++) {
			match = true;
			for (k = 0; k < ucs2_esc_seq_len; k++)
				if (seq[i + k] != ucs2_esc_seq[j][k])
					match = false;
			if (match) {
				break;
			}
		}
		
		if (!match)
			return EINVAL;
		
		i += ucs2_esc_seq_len;
	}
	
	while (i < 32) {
		if (seq[i] != 0)
			return EINVAL;
		++i;
	}
	
	return EOK;
}

/** Find Joliet supplementary volume descriptor.
 *
 * @param sid		Block device service ID
 * @param altroot	First filesystem block
 * @param rlba		Place to store LBA of root dir
 * @param rsize		Place to store size of root dir
 * @param vol_ident	Place to store pointer to volume identifier
 * @return 		EOK if found, ENOENT if not
 */
static errno_t cdfs_find_joliet_svd(service_id_t sid, cdfs_lba_t altroot,
    uint32_t *rlba, uint32_t *rsize, char **vol_ident)
{
	cdfs_lba_t bi;

	for (bi = altroot + 17; ; bi++) {
		block_t *block;
		errno_t rc = block_get(&block, sid, bi, BLOCK_FLAGS_NONE);
		if (rc != EOK)
			break;
		
		cdfs_vol_desc_t *vol_desc = (cdfs_vol_desc_t *) block->data;
		
		if (vol_desc->type == VOL_DESC_SET_TERMINATOR) {
			block_put(block);
			break;
		}
		
		if ((vol_desc->type != VOL_DESC_SUPPLEMENTARY) ||
		    (memcmp(vol_desc->standard_ident, CDFS_STANDARD_IDENT, 5) != 0) ||
		    (vol_desc->version != 1)) {
			block_put(block);
			continue;
		}
		
		uint16_t set_size = uint16_lb(vol_desc->data.prisec.set_size);
		if (set_size > 1) {
			/*
			 * Technically, we don't support multi-disc sets.
			 * But one can encounter erroneously mastered
			 * images in the wild and it might actually work
			 * for the first disc in the set.
			 */
		}
		
		uint16_t sequence_nr = uint16_lb(vol_desc->data.prisec.sequence_nr);
		if (sequence_nr != 1) {
			/*
		    	 * We only support the first disc
			 * in multi-disc sets.
			 */
			block_put(block);
			continue;
		}
		
		uint16_t block_size = uint16_lb(vol_desc->data.prisec.block_size);
		if (block_size != BLOCK_SIZE) {
			block_put(block);
			continue;
		}
		
		rc = cdfs_verify_joliet_esc_seq(vol_desc->data.prisec.esc_seq);
		if (rc != EOK)
			continue;
		*rlba = uint32_lb(vol_desc->data.prisec.root_dir.lba);
		*rsize = uint32_lb(vol_desc->data.prisec.root_dir.size);

		*vol_ident = cdfs_decode_vol_ident(vol_desc->data.prisec.ident,
		    32, enc_ucs2);

		block_put(block);
		return EOK;
	}
	
	return ENOENT;
}

/** Read the volume descriptors. */
static errno_t iso_read_vol_desc(service_id_t sid, cdfs_lba_t altroot,
    uint32_t *rlba, uint32_t *rsize, cdfs_enc_t *enc, char **vol_ident)
{
	/* First 16 blocks of isofs are empty */
	block_t *block;
	errno_t rc = block_get(&block, sid, altroot + 16, BLOCK_FLAGS_NONE);
	if (rc != EOK)
		return rc;
	
	cdfs_vol_desc_t *vol_desc = (cdfs_vol_desc_t *) block->data;
	
	/*
	 * Test for primary volume descriptor
	 * and standard compliance.
	 */
	if ((vol_desc->type != VOL_DESC_PRIMARY) ||
	    (memcmp(vol_desc->standard_ident, CDFS_STANDARD_IDENT, 5) != 0) ||
	    (vol_desc->version != 1)) {
		block_put(block);
		return ENOTSUP;
	}
	
	uint16_t set_size = uint16_lb(vol_desc->data.prisec.set_size);
	if (set_size > 1) {
		/*
		 * Technically, we don't support multi-disc sets.
		 * But one can encounter erroneously mastered
		 * images in the wild and it might actually work
		 * for the first disc in the set.
		 */
	}
	
	uint16_t sequence_nr = uint16_lb(vol_desc->data.prisec.sequence_nr);
	if (sequence_nr != 1) {
		/*
		 * We only support the first disc
		 * in multi-disc sets.
		 */
		block_put(block);
		return ENOTSUP;
	}
	
	uint16_t block_size = uint16_lb(vol_desc->data.prisec.block_size);
	if (block_size != BLOCK_SIZE) {
		block_put(block);
		return ENOTSUP;
	}
	
	// TODO: implement path table support
	
	/* Search for Joliet SVD */
	
	uint32_t jrlba;
	uint32_t jrsize;
	
	rc = cdfs_find_joliet_svd(sid, altroot, &jrlba, &jrsize, vol_ident);
	if (rc == EOK) {
		/* Found */
		*rlba = jrlba;
		*rsize = jrsize;
		*enc = enc_ucs2;
	} else {
		*rlba = uint32_lb(vol_desc->data.prisec.root_dir.lba);
		*rsize = uint32_lb(vol_desc->data.prisec.root_dir.size);
		*enc = enc_ascii;
		*vol_ident = cdfs_decode_vol_ident(vol_desc->data.prisec.ident,
		    32, enc_ascii);
	}
	
	block_put(block);
	return EOK;
}

static errno_t iso_readfs(cdfs_t *fs, fs_node_t *rfn,
    cdfs_lba_t altroot)
{
	cdfs_node_t *node = CDFS_NODE(rfn);
	
	errno_t rc = iso_read_vol_desc(fs->service_id, altroot, &node->lba,
	    &node->size, &fs->enc, &fs->vol_ident);
	if (rc != EOK)
		return rc;
	
	return cdfs_readdir(fs, rfn);
}

/* Mount a session with session start offset
 *
 */
static cdfs_t *cdfs_fs_create(service_id_t sid, cdfs_lba_t altroot)
{
	cdfs_t *fs = NULL;
	fs_node_t *rfn = NULL;

	fs = calloc(1, sizeof(cdfs_t));
	if (fs == NULL)
		goto error;
	
	fs->service_id = sid;
	
	/* Create root node */
	errno_t rc = create_node(&rfn, fs, L_DIRECTORY, cdfs_index++);
	
	if ((rc != EOK) || (!rfn))
		goto error;
	
	/* FS root is not linked */
	CDFS_NODE(rfn)->lnkcnt = 0;
	CDFS_NODE(rfn)->lba = 0;
	CDFS_NODE(rfn)->processed = false;
	
	/* Check if there is cdfs in given session */
	if (iso_readfs(fs, rfn, altroot) != EOK)
		goto error;
	
	list_append(&fs->link, &cdfs_instances);
	return fs;
error:
	// XXX destroy node
	free(fs);
	return NULL;
}

static errno_t cdfs_fsprobe(service_id_t service_id, vfs_fs_probe_info_t *info)
{
	char *vol_ident;

	/* Initialize the block layer */
	errno_t rc = block_init(service_id, BLOCK_SIZE);
	if (rc != EOK)
		return rc;
	
	cdfs_lba_t altroot = 0;
	
	/*
	 * Read TOC multisession information and get the start address
	 * of the first track in the last session
	 */
	scsi_toc_multisess_data_t toc;

	rc = block_read_toc(service_id, 1, &toc, sizeof(toc));
	if (rc == EOK && (uint16_t_be2host(toc.toc_len) == 10))
		altroot = uint32_t_be2host(toc.ftrack_lsess.start_addr);
	
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
		return EOK;
	}
	
	/* Read volume descriptors */
	uint32_t rlba;
	uint32_t rsize;
	cdfs_enc_t enc;
	rc = iso_read_vol_desc(service_id, altroot, &rlba, &rsize, &enc,
	    &vol_ident);
	if (rc != EOK) {
		block_cache_fini(service_id);
		block_fini(service_id);
		return rc;
	}
	
	str_cpy(info->label, FS_LABEL_MAXLEN + 1, vol_ident);
	free(vol_ident);
	
	block_cache_fini(service_id);
	block_fini(service_id);
	return EOK;
}

static errno_t cdfs_mounted(service_id_t service_id, const char *opts,
    fs_index_t *index, aoff64_t *size)
{
	/* Initialize the block layer */
	errno_t rc = block_init(service_id, BLOCK_SIZE);
	if (rc != EOK)
		return rc;
	
	cdfs_lba_t altroot = 0;
	
	if (str_lcmp(opts, "altroot=", 8) == 0) {
		/* User-defined alternative root on a multi-session disk */
		if (str_uint32_t(opts + 8, NULL, 0, false, &altroot) != EOK)
			altroot = 0;
	} else {
		/*
		 * Read TOC multisession information and get the start address
		 * of the first track in the last session
		 */
		scsi_toc_multisess_data_t toc;

		rc = block_read_toc(service_id, 1, &toc, sizeof(toc));
		if (rc == EOK && (uint16_t_be2host(toc.toc_len) == 10))
			altroot = uint32_t_be2host(toc.ftrack_lsess.start_addr);
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
	
	/* Create cdfs instance */
	if (cdfs_fs_create(service_id, altroot) == NULL) {
		block_cache_fini(service_id);
		block_fini(service_id);
		
		return ENOMEM;
	}
	
	rc = cdfs_root_get(&rootfn, service_id);
	assert(rc == EOK);
	
	cdfs_node_t *root = CDFS_NODE(rootfn);
	*index = root->index;
	*size = root->size;
	
	return EOK;
}

static bool rm_service_id_nodes(ht_link_t *item, void *arg) 
{
	service_id_t service_id = *(service_id_t*)arg;
	cdfs_node_t *node = hash_table_get_inst(item, cdfs_node_t, nh_link);
	
	if (node->fs->service_id == service_id) {
		hash_table_remove_item(&nodes, &node->nh_link);
	}
	
	return true;
}

static void cdfs_fs_destroy(cdfs_t *fs)
{
	list_remove(&fs->link);
	hash_table_apply(&nodes, rm_service_id_nodes, &fs->service_id);
	block_cache_fini(fs->service_id);
	block_fini(fs->service_id);
	free(fs->vol_ident);
	free(fs);
}

static cdfs_t *cdfs_find_by_sid(service_id_t service_id)
{
	list_foreach(cdfs_instances, link, cdfs_t, fs) {
		if (fs->service_id == service_id)
			return fs;
	}
	
	return NULL;
}

static errno_t cdfs_unmounted(service_id_t service_id)
{
	cdfs_t *fs;

	fs = cdfs_find_by_sid(service_id);
	if (fs == NULL)
		return ENOENT;
	
	cdfs_fs_destroy(fs);
	return EOK;
}

static errno_t cdfs_read(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *rbytes)
{
	ht_key_t key = {
		.index = index,
		.service_id = service_id
	};
	
	ht_link_t *link = hash_table_find(&nodes, &key);
	if (link == NULL)
		return ENOENT;
	
	cdfs_node_t *node =
	    hash_table_get_inst(link, cdfs_node_t, nh_link);
	
	if (!node->processed) {
		errno_t rc = cdfs_readdir(node->fs, FS_NODE(node));
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
			errno_t rc = block_get(&block, service_id, node->lba + lba,
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

static errno_t cdfs_write(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *wbytes, aoff64_t *nsize)
{
	/*
	 * As cdfs is a read-only filesystem,
	 * the operation is not supported.
	 */
	
	return ENOTSUP;
}

static errno_t cdfs_truncate(service_id_t service_id, fs_index_t index,
    aoff64_t size)
{
	/*
	 * As cdfs is a read-only filesystem,
	 * the operation is not supported.
	 */
	
	return ENOTSUP;
}

static bool cache_remove_closed(ht_link_t *item, void *arg)
{
	size_t *premove_cnt = (size_t*)arg;
	
	/* Some nodes were requested to be removed from the cache. */
	if (0 < *premove_cnt) {
		cdfs_node_t *node = hash_table_get_inst(item, cdfs_node_t, nh_link);

		if (!node->opened) {
			hash_table_remove_item(&nodes, item);
			
			--nodes_cached;
			--*premove_cnt;
		}
	}
	
	/* Only continue if more nodes were requested to be removed. */
	return 0 < *premove_cnt;
}

static void cleanup_cache(service_id_t service_id)
{
	if (nodes_cached > NODE_CACHE_SIZE) {
		size_t remove_cnt = nodes_cached - NODE_CACHE_SIZE;
		
		if (0 < remove_cnt)
			hash_table_apply(&nodes, cache_remove_closed, &remove_cnt);
	}
}

static errno_t cdfs_close(service_id_t service_id, fs_index_t index)
{
	/* Root node is always in memory */
	if (index == 0)
		return EOK;
	
	ht_key_t key = {
		.index = index,
		.service_id = service_id
	};
	
	ht_link_t *link = hash_table_find(&nodes, &key);
	if (link == 0)
		return ENOENT;
	
	cdfs_node_t *node =
	    hash_table_get_inst(link, cdfs_node_t, nh_link);
	
	assert(node->opened > 0);
	
	node->opened--;
	cleanup_cache(service_id);
	
	return EOK;
}

static errno_t cdfs_destroy(service_id_t service_id, fs_index_t index)
{
	/*
	 * As cdfs is a read-only filesystem,
	 * the operation is not supported.
	 */
	
	return ENOTSUP;
}

static errno_t cdfs_sync(service_id_t service_id, fs_index_t index)
{
	/*
	 * As cdfs is a read-only filesystem,
	 * the sync operation is a no-op.
	 */
	
	return EOK;
}

vfs_out_ops_t cdfs_ops = {
	.fsprobe = cdfs_fsprobe,
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
	if (!hash_table_create(&nodes, 0, 0, &nodes_ops))
		return false;
	
	return true;
}

/**
 * @}
 */
