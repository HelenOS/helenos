/*
 * Copyright (c) 2011 Martin Sucha
 * Copyright (c) 2012 Frantisek Princ
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

/** @addtogroup libext4
 * @{
 */
/**
 * @file  ops.c
 * @brief Operations for ext4 filesystem.
 */

#include <adt/hash_table.h>
#include <adt/hash.h>
#include <errno.h>
#include <fibril_synch.h>
#include <libfs.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>
#include <str.h>
#include <ipc/loc.h>
#include "ext4/balloc.h"
#include "ext4/directory.h"
#include "ext4/directory_index.h"
#include "ext4/extent.h"
#include "ext4/inode.h"
#include "ext4/ops.h"
#include "ext4/filesystem.h"
#include "ext4/fstypes.h"
#include "ext4/superblock.h"

/* Forward declarations of auxiliary functions */

static errno_t ext4_read_directory(ipc_callid_t, aoff64_t, size_t,
    ext4_instance_t *, ext4_inode_ref_t *, size_t *);
static errno_t ext4_read_file(ipc_callid_t, aoff64_t, size_t, ext4_instance_t *,
    ext4_inode_ref_t *, size_t *);
static bool ext4_is_dots(const uint8_t *, size_t);
static errno_t ext4_instance_get(service_id_t, ext4_instance_t **);

/* Forward declarations of ext4 libfs operations. */

static errno_t ext4_root_get(fs_node_t **, service_id_t);
static errno_t ext4_match(fs_node_t **, fs_node_t *, const char *);
static errno_t ext4_node_get(fs_node_t **, service_id_t, fs_index_t);
static errno_t ext4_node_open(fs_node_t *);
       errno_t ext4_node_put(fs_node_t *);
static errno_t ext4_create_node(fs_node_t **, service_id_t, int);
static errno_t ext4_destroy_node(fs_node_t *);
static errno_t ext4_link(fs_node_t *, fs_node_t *, const char *);
static errno_t ext4_unlink(fs_node_t *, fs_node_t *, const char *);
static errno_t ext4_has_children(bool *, fs_node_t *);
static fs_index_t ext4_index_get(fs_node_t *);
static aoff64_t ext4_size_get(fs_node_t *);
static unsigned ext4_lnkcnt_get(fs_node_t *);
static bool ext4_is_directory(fs_node_t *);
static bool ext4_is_file(fs_node_t *node);
static service_id_t ext4_service_get(fs_node_t *node);
static errno_t ext4_size_block(service_id_t, uint32_t *);
static errno_t ext4_total_block_count(service_id_t, uint64_t *);
static errno_t ext4_free_block_count(service_id_t, uint64_t *);

/* Static variables */

static LIST_INITIALIZE(instance_list);
static FIBRIL_MUTEX_INITIALIZE(instance_list_mutex);
static hash_table_t open_nodes;
static FIBRIL_MUTEX_INITIALIZE(open_nodes_lock);

/* Hash table interface for open nodes hash table */

typedef struct {
	service_id_t service_id;
	fs_index_t index;
} node_key_t;

static size_t open_nodes_key_hash(void *key_arg)
{
	node_key_t *key = (node_key_t *)key_arg;
	return hash_combine(key->service_id, key->index);
}

static size_t open_nodes_hash(const ht_link_t *item)
{
	ext4_node_t *enode = hash_table_get_inst(item, ext4_node_t, link);
	return hash_combine(enode->instance->service_id, enode->inode_ref->index);	
}

static bool open_nodes_key_equal(void *key_arg, const ht_link_t *item)
{
	node_key_t *key = (node_key_t *)key_arg;
	ext4_node_t *enode = hash_table_get_inst(item, ext4_node_t, link);
	
	return key->service_id == enode->instance->service_id
		&& key->index == enode->inode_ref->index;
}

static hash_table_ops_t open_nodes_ops = {
	.hash = open_nodes_hash,
	.key_hash = open_nodes_key_hash,
	.key_equal = open_nodes_key_equal,
	.equal = NULL,
	.remove_callback = NULL,
};

/** Basic initialization of the driver.
 *
 * This is only needed to create the hash table
 * for storing open nodes.
 *
 * @return Error code
 *
 */
errno_t ext4_global_init(void)
{
	if (!hash_table_create(&open_nodes, 0, 0, &open_nodes_ops))
		return ENOMEM;
	
	return EOK;
}

/* Finalization of the driver.
 *
 * This is only needed to destroy the hash table.
 *
 * @return Error code
 */
errno_t ext4_global_fini(void)
{
	hash_table_destroy(&open_nodes);
	return EOK;
}

/*
 * Ext4 libfs operations.
 */

/** Get instance from internal table by service_id.
 *
 * @param service_id Device identifier
 * @param inst       Output instance if successful operation
 *
 * @return Error code
 *
 */
errno_t ext4_instance_get(service_id_t service_id, ext4_instance_t **inst)
{
	fibril_mutex_lock(&instance_list_mutex);
	
	if (list_empty(&instance_list)) {
		fibril_mutex_unlock(&instance_list_mutex);
		return EINVAL;
	}
	
	list_foreach(instance_list, link, ext4_instance_t, tmp) {
		if (tmp->service_id == service_id) {
			*inst = tmp;
			fibril_mutex_unlock(&instance_list_mutex);
			return EOK;
		}
	}
	
	fibril_mutex_unlock(&instance_list_mutex);
	return EINVAL;
}

/** Get root node of filesystem specified by service_id.
 * 
 * @param rfn        Output pointer to loaded node
 * @param service_id Device to load root node from
 *
 * @return Error code
 *
 */
errno_t ext4_root_get(fs_node_t **rfn, service_id_t service_id)
{
	return ext4_node_get(rfn, service_id, EXT4_INODE_ROOT_INDEX);
}

/** Check if specified name (component) matches with any directory entry.
 * 
 * If match is found, load and return matching node.
 *
 * @param rfn       Output pointer to node if operation successful
 * @param pfn       Parent directory node
 * @param component Name to check directory for
 *
 * @return Error code
 *
 */
errno_t ext4_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	ext4_node_t *eparent = EXT4_NODE(pfn);
	ext4_filesystem_t *fs = eparent->instance->filesystem;
	
	if (!ext4_inode_is_type(fs->superblock, eparent->inode_ref->inode,
	    EXT4_INODE_MODE_DIRECTORY))
		return ENOTDIR;
	
	/* Try to find entry */
	ext4_directory_search_result_t result;
	errno_t rc = ext4_directory_find_entry(&result, eparent->inode_ref,
	    component);
	if (rc != EOK) {
		if (rc == ENOENT) {
			*rfn = NULL;
			return EOK;
		}
		
		return rc;
	}
	
	/* Load node from search result */
	uint32_t inode = ext4_directory_entry_ll_get_inode(result.dentry);
	rc = ext4_node_get_core(rfn, eparent->instance, inode);
	if (rc != EOK)
		goto exit;

exit:
	;

	/* Destroy search result structure */
	errno_t const rc2 = ext4_directory_destroy_result(&result);
	return rc == EOK ? rc2 : rc;
}

/** Get node specified by index
 *
 * It's wrapper for node_put_core operation
 *
 * @param rfn        Output pointer to loaded node if operation successful
 * @param service_id Device identifier
 * @param index      Node index (here i-node number)
 *
 * @return Error code
 *
 */
errno_t ext4_node_get(fs_node_t **rfn, service_id_t service_id, fs_index_t index)
{
	ext4_instance_t *inst;
	errno_t rc = ext4_instance_get(service_id, &inst);
	if (rc != EOK)
		return rc;
	
	return ext4_node_get_core(rfn, inst, index);
}

/** Main function for getting node from the filesystem. 
 *
 * @param rfn   Output point to loaded node if operation successful
 * @param inst  Instance of filesystem
 * @param index Index of node (i-node number)
 *
 * @return Error code
 *
 */
errno_t ext4_node_get_core(fs_node_t **rfn, ext4_instance_t *inst,
    fs_index_t index)
{
	fibril_mutex_lock(&open_nodes_lock);
	
	/* Check if the node is not already open */
	node_key_t key = {
		.service_id = inst->service_id,
		.index = index
	};
	
	ht_link_t *already_open = hash_table_find(&open_nodes, &key);
	ext4_node_t *enode = NULL;
	if (already_open) {
		enode = hash_table_get_inst(already_open, ext4_node_t, link);
		*rfn = enode->fs_node;
		enode->references++;
		
		fibril_mutex_unlock(&open_nodes_lock);
		return EOK;
	}
	
	/* Prepare new enode */
	enode = malloc(sizeof(ext4_node_t));
	if (enode == NULL) {
		fibril_mutex_unlock(&open_nodes_lock);
		return ENOMEM;
	}
	
	/* Prepare new fs_node and initialize */
	fs_node_t *fs_node = malloc(sizeof(fs_node_t));
	if (fs_node == NULL) {
		free(enode);
		fibril_mutex_unlock(&open_nodes_lock);
		return ENOMEM;
	}
	
	fs_node_initialize(fs_node);
	
	/* Load i-node from filesystem */
	ext4_inode_ref_t *inode_ref;
	errno_t rc = ext4_filesystem_get_inode_ref(inst->filesystem, index,
	    &inode_ref);
	if (rc != EOK) {
		free(enode);
		free(fs_node);
		fibril_mutex_unlock(&open_nodes_lock);
		return rc;
	}
	
	/* Initialize enode */
	enode->inode_ref = inode_ref;
	enode->instance = inst;
	enode->references = 1;
	enode->fs_node = fs_node;
	
	fs_node->data = enode;
	*rfn = fs_node;
	
	hash_table_insert(&open_nodes, &enode->link);
	inst->open_nodes_count++;
	
	fibril_mutex_unlock(&open_nodes_lock);
	
	return EOK;
}

/** Put previously loaded node.
 *
 * @param enode Node to put back
 *
 * @return Error code
 *
 */
static errno_t ext4_node_put_core(ext4_node_t *enode)
{
	hash_table_remove_item(&open_nodes, &enode->link);
	assert(enode->instance->open_nodes_count > 0);
	enode->instance->open_nodes_count--;
	
	/* Put inode back in filesystem */
	errno_t rc = ext4_filesystem_put_inode_ref(enode->inode_ref);
	if (rc != EOK)
		return rc;
	
	/* Destroy data structure */
	free(enode->fs_node);
	free(enode);
	
	return EOK;
}

/** Open node.
 *
 * This operation is stateless in this driver.
 *
 * @param fn Node to open
 *
 * @return EOK
 *
 */
errno_t ext4_node_open(fs_node_t *fn)
{
	/* Stateless operation */
	return EOK;
}

/** Put previously loaded node.
 *
 * A wrapper for node_put_core operation
 *
 * @param fn Node to put back
 * @return Error code
 *
 */
errno_t ext4_node_put(fs_node_t *fn)
{
	fibril_mutex_lock(&open_nodes_lock);
	
	ext4_node_t *enode = EXT4_NODE(fn);
	assert(enode->references > 0);
	enode->references--;
	if (enode->references == 0) {
		errno_t rc = ext4_node_put_core(enode);
		if (rc != EOK) {
			fibril_mutex_unlock(&open_nodes_lock);
			return rc;
		}
	}
	
	fibril_mutex_unlock(&open_nodes_lock);
	
	return EOK;
}

/** Create new node in filesystem.
 *
 * @param rfn        Output pointer to newly created node if successful
 * @param service_id Device identifier, where the filesystem is
 * @param flags      Flags for specification of new node parameters
 *
 * @return Error code
 *
 */
errno_t ext4_create_node(fs_node_t **rfn, service_id_t service_id, int flags)
{
	/* Allocate enode */
	ext4_node_t *enode;
	enode = malloc(sizeof(ext4_node_t));
	if (enode == NULL)
		return ENOMEM;
	
	/* Allocate fs_node */
	fs_node_t *fs_node;
	fs_node = malloc(sizeof(fs_node_t));
	if (fs_node == NULL) {
		free(enode);
		return ENOMEM;
	}
	
	/* Load instance */
	ext4_instance_t *inst;
	errno_t rc = ext4_instance_get(service_id, &inst);
	if (rc != EOK) {
		free(enode);
		free(fs_node);
		return rc;
	}
	
	/* Allocate new i-node in filesystem */
	ext4_inode_ref_t *inode_ref;
	rc = ext4_filesystem_alloc_inode(inst->filesystem, &inode_ref, flags);
	if (rc != EOK) {
		free(enode);
		free(fs_node);
		return rc;
	}
	
	/* Do some interconnections in references */
	enode->inode_ref = inode_ref;
	enode->instance = inst;
	enode->references = 1;
	
	fibril_mutex_lock(&open_nodes_lock);
	hash_table_insert(&open_nodes, &enode->link);
	fibril_mutex_unlock(&open_nodes_lock);
	inst->open_nodes_count++;
	
	enode->inode_ref->dirty = true;
	
	fs_node_initialize(fs_node);
	fs_node->data = enode;
	enode->fs_node = fs_node;
	*rfn = fs_node;
	
	return EOK;
}

/** Destroy existing node.
 *
 * @param fs Node to destroy
 *
 * @return Error code
 *
 */
errno_t ext4_destroy_node(fs_node_t *fn)
{
	/* If directory, check for children */
	bool has_children;
	errno_t rc = ext4_has_children(&has_children, fn);
	if (rc != EOK) {
		ext4_node_put(fn);
		return rc;
	}
	
	if (has_children) {
		ext4_node_put(fn);
		return EINVAL;
	}
	
	ext4_node_t *enode = EXT4_NODE(fn);
	ext4_inode_ref_t *inode_ref = enode->inode_ref;
	
	/* Release data blocks */
	rc = ext4_filesystem_truncate_inode(inode_ref, 0);
	if (rc != EOK) {
		ext4_node_put(fn);
		return rc;
	}
	
	/*
	 * TODO: Sset real deletion time when it will be supported.
	 * Temporary set fake deletion time.
	 */
	ext4_inode_set_deletion_time(inode_ref->inode, 0xdeadbeef);
	inode_ref->dirty = true;
	
	/* Free inode */
	rc = ext4_filesystem_free_inode(inode_ref);
	if (rc != EOK) {
		ext4_node_put(fn);
		return rc;
	}
	
	return ext4_node_put(fn);
}

/** Link the specfied node to directory.
 *
 * @param pfn  Parent node to link in
 * @param cfn  Node to be linked
 * @param name Name which will be assigned to directory entry
 *
 * @return Error code
 *
 */
errno_t ext4_link(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	/* Check maximum name length */
	if (str_size(name) > EXT4_DIRECTORY_FILENAME_LEN)
		return ENAMETOOLONG;
	
	ext4_node_t *parent = EXT4_NODE(pfn);
	ext4_node_t *child = EXT4_NODE(cfn);
	ext4_filesystem_t *fs = parent->instance->filesystem;
	
	/* Add entry to parent directory */
	errno_t rc = ext4_directory_add_entry(parent->inode_ref, name,
	    child->inode_ref);
	if (rc != EOK)
		return rc;
	
	/* Fill new dir -> add '.' and '..' entries */
	if (ext4_inode_is_type(fs->superblock, child->inode_ref->inode,
	    EXT4_INODE_MODE_DIRECTORY)) {
		rc = ext4_directory_add_entry(child->inode_ref, ".",
		    child->inode_ref);
		if (rc != EOK) {
			ext4_directory_remove_entry(parent->inode_ref, name);
			return rc;
		}
		
		rc = ext4_directory_add_entry(child->inode_ref, "..",
		    parent->inode_ref);
		if (rc != EOK) {
			ext4_directory_remove_entry(parent->inode_ref, name);
			ext4_directory_remove_entry(child->inode_ref, ".");
			return rc;
		}
		
		/* Initialize directory index if supported */
		if (ext4_superblock_has_feature_compatible(fs->superblock,
		    EXT4_FEATURE_COMPAT_DIR_INDEX)) {
			rc = ext4_directory_dx_init(child->inode_ref);
			if (rc != EOK)
				return rc;
			
			ext4_inode_set_flag(child->inode_ref->inode,
			    EXT4_INODE_FLAG_INDEX);
			child->inode_ref->dirty = true;
		}
	
		uint16_t parent_links =
		    ext4_inode_get_links_count(parent->inode_ref->inode);
		parent_links++;
		ext4_inode_set_links_count(parent->inode_ref->inode, parent_links);
		
		parent->inode_ref->dirty = true;
	}
	
	uint16_t child_links =
	    ext4_inode_get_links_count(child->inode_ref->inode);
	child_links++;
	ext4_inode_set_links_count(child->inode_ref->inode, child_links);
	
	child->inode_ref->dirty = true;
	
	return EOK;
}

/** Unlink node from specified directory.
 *
 * @param pfn  Parent node to delete node from
 * @param cfn  Child node to be unlinked from directory
 * @param name Name of entry that will be removed
 *
 * @return Error code
 *
 */
errno_t ext4_unlink(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	bool has_children;
	errno_t rc = ext4_has_children(&has_children, cfn);
	if (rc != EOK)
		return rc;
	
	/* Cannot unlink non-empty node */
	if (has_children)
		return ENOTEMPTY;
	
	/* Remove entry from parent directory */
	ext4_inode_ref_t *parent = EXT4_NODE(pfn)->inode_ref;
	rc = ext4_directory_remove_entry(parent, name);
	if (rc != EOK)
		return rc;
	
	/* Decrement links count */
	ext4_inode_ref_t *child_inode_ref = EXT4_NODE(cfn)->inode_ref;
	
	uint32_t lnk_count =
	    ext4_inode_get_links_count(child_inode_ref->inode);
	lnk_count--;
	
	/* If directory - handle links from parent */
	if ((lnk_count <= 1) && (ext4_is_directory(cfn))) {
		assert(lnk_count == 1);
		
		lnk_count--;
		
		ext4_inode_ref_t *parent_inode_ref = EXT4_NODE(pfn)->inode_ref;
		
		uint32_t parent_lnk_count = ext4_inode_get_links_count(
		    parent_inode_ref->inode);
		
		parent_lnk_count--;
		ext4_inode_set_links_count(parent_inode_ref->inode, parent_lnk_count);
		
		parent->dirty = true;
	}

	/*
	 * TODO: Update timestamps of the parent
	 * (when we have wall-clock time).
	 *
	 * ext4_inode_set_change_inode_time(parent->inode, (uint32_t) now);
	 * ext4_inode_set_modification_time(parent->inode, (uint32_t) now);
	 * parent->dirty = true;
	 */
	
	/*
	 * TODO: Update timestamp for inode.
	 *
	 * ext4_inode_set_change_inode_time(child_inode_ref->inode,
	 *     (uint32_t) now);
	 */
	
	ext4_inode_set_links_count(child_inode_ref->inode, lnk_count);
	child_inode_ref->dirty = true;
	
	return EOK;
}

/** Check if specified node has children.
 *
 * For files is response allways false and check is executed only for directories.
 *
 * @param has_children Output value for response
 * @param fn           Node to check
 *
 * @return Error code
 *
 */
errno_t ext4_has_children(bool *has_children, fs_node_t *fn)
{
	ext4_node_t *enode = EXT4_NODE(fn);
	ext4_filesystem_t *fs = enode->instance->filesystem;
	
	/* Check if node is directory */
	if (!ext4_inode_is_type(fs->superblock, enode->inode_ref->inode,
	    EXT4_INODE_MODE_DIRECTORY)) {
		*has_children = false;
		return EOK;
	}
	
	ext4_directory_iterator_t it;
	errno_t rc = ext4_directory_iterator_init(&it, enode->inode_ref, 0);
	if (rc != EOK)
		return rc;
	
	/* Find a non-empty directory entry */
	bool found = false;
	while (it.current != NULL) {
		if (it.current->inode != 0) {
			uint16_t name_size =
			    ext4_directory_entry_ll_get_name_length(fs->superblock,
			    it.current);
			if (!ext4_is_dots(it.current->name, name_size)) {
				found = true;
				break;
			}
		}
		
		rc = ext4_directory_iterator_next(&it);
		if (rc != EOK) {
			ext4_directory_iterator_fini(&it);
			return rc;
		}
	}
	
	rc = ext4_directory_iterator_fini(&it);
	if (rc != EOK)
		return rc;
	
	*has_children = found;
	
	return EOK;
}

/** Unpack index number from node.
 *
 * @param fn Node to load index from
 *
 * @return Index number of i-node
 *
 */
fs_index_t ext4_index_get(fs_node_t *fn)
{
	ext4_node_t *enode = EXT4_NODE(fn);
	return enode->inode_ref->index;
}

/** Get real size of file / directory.
 *
 * @param fn Node to get size of
 *
 * @return Real size of node
 *
 */
aoff64_t ext4_size_get(fs_node_t *fn)
{
	ext4_node_t *enode = EXT4_NODE(fn);
	ext4_superblock_t *sb = enode->instance->filesystem->superblock;
	return ext4_inode_get_size(sb, enode->inode_ref->inode);
}

/** Get number of links to specified node.
 *
 * @param fn Node to get links to
 *
 * @return Number of links
 *
 */
unsigned ext4_lnkcnt_get(fs_node_t *fn)
{
	ext4_node_t *enode = EXT4_NODE(fn);
	uint32_t lnkcnt = ext4_inode_get_links_count(enode->inode_ref->inode);
	
	if (ext4_is_directory(fn)) {
		if (lnkcnt > 1)
			return 1;
		else
			return 0;
	}
	
	/* For regular files return real links count */
	return lnkcnt;
}

/** Check if node is directory.
 *
 * @param fn Node to check
 *
 * @return Result of check
 *
 */
bool ext4_is_directory(fs_node_t *fn)
{
	ext4_node_t *enode = EXT4_NODE(fn);
	ext4_superblock_t *sb = enode->instance->filesystem->superblock;
	
	return ext4_inode_is_type(sb, enode->inode_ref->inode,
	    EXT4_INODE_MODE_DIRECTORY);
}

/** Check if node is regular file.
 *
 * @param fn Node to check
 *
 * @return Result of check
 *
 */
bool ext4_is_file(fs_node_t *fn)
{
	ext4_node_t *enode = EXT4_NODE(fn);
	ext4_superblock_t *sb = enode->instance->filesystem->superblock;
	
	return ext4_inode_is_type(sb, enode->inode_ref->inode,
	    EXT4_INODE_MODE_FILE);
}

/** Extract device identifier from node.
 *
 * @param node Node to extract id from
 *
 * @return id of device, where is the filesystem
 *
 */
service_id_t ext4_service_get(fs_node_t *fn)
{
	ext4_node_t *enode = EXT4_NODE(fn);
	return enode->instance->service_id;
}

errno_t ext4_size_block(service_id_t service_id, uint32_t *size)
{
	ext4_instance_t *inst;
	errno_t rc = ext4_instance_get(service_id, &inst);
	if (rc != EOK)
		return rc;

	if (NULL == inst)
		return ENOENT;

	ext4_superblock_t *sb = inst->filesystem->superblock;
	*size = ext4_superblock_get_block_size(sb);

	return EOK;
}

errno_t ext4_total_block_count(service_id_t service_id, uint64_t *count)
{
	ext4_instance_t *inst;
	errno_t rc = ext4_instance_get(service_id, &inst);
	if (rc != EOK)
		return rc;

	if (NULL == inst)
		return ENOENT;

	ext4_superblock_t *sb = inst->filesystem->superblock;
	*count = ext4_superblock_get_blocks_count(sb);

	return EOK;
}

errno_t ext4_free_block_count(service_id_t service_id, uint64_t *count)
{
	ext4_instance_t *inst;
	errno_t rc = ext4_instance_get(service_id, &inst);
	if (rc != EOK)
		return rc;

	ext4_superblock_t *sb = inst->filesystem->superblock;
	*count = ext4_superblock_get_free_blocks_count(sb);

	return EOK;
}

/*
 * libfs operations.
 */
libfs_ops_t ext4_libfs_ops = {
	.root_get = ext4_root_get,
	.match = ext4_match,
	.node_get = ext4_node_get,
	.node_open = ext4_node_open,
	.node_put = ext4_node_put,
	.create = ext4_create_node,
	.destroy = ext4_destroy_node,
	.link = ext4_link,
	.unlink = ext4_unlink,
	.has_children = ext4_has_children,
	.index_get = ext4_index_get,
	.size_get = ext4_size_get,
	.lnkcnt_get = ext4_lnkcnt_get,
	.is_directory = ext4_is_directory,
	.is_file = ext4_is_file,
	.service_get = ext4_service_get,
	.size_block = ext4_size_block,
	.total_block_count = ext4_total_block_count,
	.free_block_count = ext4_free_block_count
};

/*
 * VFS operations.
 */

/** Probe operation.
 *
 * Try to get information about specified filesystem from device.
 *
 * @param sevice_id Service ID
 * @param info Place to store information
 *
 * @return Error code
 */
static errno_t ext4_fsprobe(service_id_t service_id, vfs_fs_probe_info_t *info)
{
	return ext4_filesystem_probe(service_id);
}

/** Mount operation.
 *
 * Try to mount specified filesystem from device.
 *
 * @param service_id Identifier of device
 * @param opts       Mount options
 * @param index      Output value - index of root node
 * @param size       Output value - size of root node
 *
 * @return Error code
 *
 */
static errno_t ext4_mounted(service_id_t service_id, const char *opts,
    fs_index_t *index, aoff64_t *size)
{
	ext4_filesystem_t *fs;
	
	/* Allocate instance structure */
	ext4_instance_t *inst = (ext4_instance_t *)
	    malloc(sizeof(ext4_instance_t));
	if (inst == NULL)
		return ENOMEM;
	
	enum cache_mode cmode;
	if (str_cmp(opts, "wtcache") == 0)
		cmode = CACHE_MODE_WT;
	else
		cmode = CACHE_MODE_WB;
	
	/* Initialize instance */
	link_initialize(&inst->link);
	inst->service_id = service_id;
	inst->open_nodes_count = 0;
	
	/* Initialize the filesystem */
	aoff64_t rnsize;
	errno_t rc = ext4_filesystem_open(inst, service_id, cmode, &rnsize, &fs);
	if (rc != EOK) {
		free(inst);
		return rc;
	}
	
	/* Add instance to the list */
	fibril_mutex_lock(&instance_list_mutex);
	list_append(&inst->link, &instance_list);
	fibril_mutex_unlock(&instance_list_mutex);
	
	*index = EXT4_INODE_ROOT_INDEX;
	*size = rnsize;
	
	return EOK;
}

/** Unmount operation.
 *
 * Correctly release the filesystem.
 *
 * @param service_id Device to be unmounted
 *
 * @return Error code
 *
 */
static errno_t ext4_unmounted(service_id_t service_id)
{
	ext4_instance_t *inst;
	errno_t rc = ext4_instance_get(service_id, &inst);
	if (rc != EOK)
		return rc;
	
	fibril_mutex_lock(&open_nodes_lock);
	
	if (inst->open_nodes_count != 0) {
		fibril_mutex_unlock(&open_nodes_lock);
		return EBUSY;
	}
	
	/* Remove the instance from the list */
	fibril_mutex_lock(&instance_list_mutex);
	list_remove(&inst->link);
	fibril_mutex_unlock(&instance_list_mutex);
	
	fibril_mutex_unlock(&open_nodes_lock);
	
	rc = ext4_filesystem_close(inst->filesystem);
	if (rc != EOK) {
		fibril_mutex_lock(&instance_list_mutex);
		list_append(&inst->link, &instance_list);
		fibril_mutex_unlock(&instance_list_mutex);
	}

	free(inst);
	return EOK;
}

/** Read bytes from node.
 *
 * @param service_id Device to read data from
 * @param index      Number of node to read from
 * @param pos        Position where the read should be started
 * @param rbytes     Output value, where the real size was returned
 *
 * @return Error code
 *
 */
static errno_t ext4_read(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *rbytes)
{
	/*
	 * Receive the read request.
	 */
	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EINVAL);
		return EINVAL;
	}
	
	ext4_instance_t *inst;
	errno_t rc = ext4_instance_get(service_id, &inst);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return rc;
	}
	
	/* Load i-node */
	ext4_inode_ref_t *inode_ref;
	rc = ext4_filesystem_get_inode_ref(inst->filesystem, index, &inode_ref);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return rc;
	}
	
	/* Read from i-node by type */
	if (ext4_inode_is_type(inst->filesystem->superblock, inode_ref->inode,
	    EXT4_INODE_MODE_FILE)) {
		rc = ext4_read_file(callid, pos, size, inst, inode_ref,
		    rbytes);
	} else if (ext4_inode_is_type(inst->filesystem->superblock,
	    inode_ref->inode, EXT4_INODE_MODE_DIRECTORY)) {
		rc = ext4_read_directory(callid, pos, size, inst, inode_ref,
		    rbytes);
	} else {
		/* Other inode types not supported */
		async_answer_0(callid, ENOTSUP);
		rc = ENOTSUP;
	}
	
	errno_t const rc2 = ext4_filesystem_put_inode_ref(inode_ref);
	
	return rc == EOK ? rc2 : rc;
}

/** Check if filename is dot or dotdot (reserved names).
 *
 * @param name      Name to check
 * @param name_size Length of string name
 *
 * @return Result of the check
 *
 */
bool ext4_is_dots(const uint8_t *name, size_t name_size)
{
	if ((name_size == 1) && (name[0] == '.'))
		return true;
	
	if ((name_size == 2) && (name[0] == '.') && (name[1] == '.'))
		return true;
	
	return false;
}

/** Read data from directory.
 *
 * @param callid    IPC id of call (for communication)
 * @param pos       Position to start reading from
 * @param size      How many bytes to read
 * @param inst      Filesystem instance
 * @param inode_ref Node to read data from
 * @param rbytes    Output value to return real number of bytes was read
 *
 * @return Error code
 *
 */
errno_t ext4_read_directory(ipc_callid_t callid, aoff64_t pos, size_t size,
    ext4_instance_t *inst, ext4_inode_ref_t *inode_ref, size_t *rbytes)
{
	ext4_directory_iterator_t it;
	errno_t rc = ext4_directory_iterator_init(&it, inode_ref, pos);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return rc;
	}
	
	/*
	 * Find next interesting directory entry.
	 * We want to skip . and .. entries
	 * as these are not used in HelenOS
	 */
	bool found = false;
	while (it.current != NULL) {
		if (it.current->inode == 0)
			goto skip;
		
		uint16_t name_size = ext4_directory_entry_ll_get_name_length(
		    inst->filesystem->superblock, it.current);
		
		/* Skip . and .. */
		if (ext4_is_dots(it.current->name, name_size))
			goto skip;
		
		/*
		 * The on-disk entry does not contain \0 at the end
		 * end of entry name, so we copy it to new buffer
		 * and add the \0 at the end
		 */
		uint8_t *buf = malloc(name_size + 1);
		if (buf == NULL) {
			ext4_directory_iterator_fini(&it);
			async_answer_0(callid, ENOMEM);
			return ENOMEM;
		}
		
		memcpy(buf, &it.current->name, name_size);
		*(buf + name_size) = 0;
		found = true;
		
		(void) async_data_read_finalize(callid, buf, name_size + 1);
		free(buf);
		break;
		
skip:
		rc = ext4_directory_iterator_next(&it);
		if (rc != EOK) {
			ext4_directory_iterator_fini(&it);
			async_answer_0(callid, rc);
			return rc;
		}
	}
	
	uint64_t next;
	if (found) {
		rc = ext4_directory_iterator_next(&it);
		if (rc != EOK)
			return rc;
		
		next = it.current_offset;
	}
	
	rc = ext4_directory_iterator_fini(&it);
	if (rc != EOK)
		return rc;
	
	/* Prepare return values */
	if (found) {
		*rbytes = next - pos;
		return EOK;
	} else {
		async_answer_0(callid, ENOENT);
		return ENOENT;
	}
}

/** Read data from file.
 *
 * @param callid    IPC id of call (for communication)
 * @param pos       Position to start reading from
 * @param size      How many bytes to read
 * @param inst      Filesystem instance
 * @param inode_ref Node to read data from
 * @param rbytes    Output value to return real number of bytes was read
 *
 * @return Error code
 *
 */
errno_t ext4_read_file(ipc_callid_t callid, aoff64_t pos, size_t size,
    ext4_instance_t *inst, ext4_inode_ref_t *inode_ref, size_t *rbytes)
{
	ext4_superblock_t *sb = inst->filesystem->superblock;
	uint64_t file_size = ext4_inode_get_size(sb, inode_ref->inode);
	
	if (pos >= file_size) {
		/* Read 0 bytes successfully */
		async_data_read_finalize(callid, NULL, 0);
		*rbytes = 0;
		return EOK;
	}
	
	/* For now, we only read data from one block at a time */
	uint32_t block_size = ext4_superblock_get_block_size(sb);
	aoff64_t file_block = pos / block_size;
	uint32_t offset_in_block = pos % block_size;
	uint32_t bytes = min(block_size - offset_in_block, size);
	
	/* Handle end of file */
	if (pos + bytes > file_size)
		bytes = file_size - pos;
	
	/* Get the real block number */
	uint32_t fs_block;
	errno_t rc = ext4_filesystem_get_inode_data_block_index(inode_ref,
	    file_block, &fs_block);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return rc;
	}
	
	/*
	 * Check for sparse file.
	 * If ext4_filesystem_get_inode_data_block_index returned
	 * fs_block == 0, it means that the given block is not allocated for the
	 * file and we need to return a buffer of zeros
	 */
	uint8_t *buffer;
	if (fs_block == 0) {
		buffer = malloc(bytes);
		if (buffer == NULL) {
			async_answer_0(callid, ENOMEM);
			return ENOMEM;
		}
		
		memset(buffer, 0, bytes);
		
		rc = async_data_read_finalize(callid, buffer, bytes);
		*rbytes = bytes;
		
		free(buffer);
		return rc;
	}
	
	/* Usual case - we need to read a block from device */
	block_t *block;
	rc = block_get(&block, inst->service_id, fs_block, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return rc;
	}
	
	assert(offset_in_block + bytes <= block_size);
	rc = async_data_read_finalize(callid, block->data + offset_in_block, bytes);
	if (rc != EOK) {
		block_put(block);
		return rc;
	}
	
	rc = block_put(block);
	if (rc != EOK)
		return rc;
	
	*rbytes = bytes;
	return EOK;
}

/** Write bytes to file
 *
 * @param service_id Device identifier
 * @param index      I-node number of file
 * @param pos        Position in file to start reading from
 * @param wbytes     Output value - real number of written bytes
 * @param nsize      Output value - new size of i-node
 *
 * @return Error code
 *
 */
static errno_t ext4_write(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *wbytes, aoff64_t *nsize)
{
	fs_node_t *fn;
	errno_t rc = ext4_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	
	ipc_callid_t callid;
	size_t len;
	if (!async_data_write_receive(&callid, &len)) {
		rc = EINVAL;
		async_answer_0(callid, rc);
		goto exit;
	}
	
	ext4_node_t *enode = EXT4_NODE(fn);
	ext4_filesystem_t *fs = enode->instance->filesystem;
	
	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);
	
	/* Prevent writing to more than one block */
	uint32_t bytes = min(len, block_size - (pos % block_size));
	
	int flags = BLOCK_FLAGS_NONE;
	if (bytes == block_size)
		flags = BLOCK_FLAGS_NOREAD;
	
	uint32_t iblock =  pos / block_size;
	uint32_t fblock;
	
	/* Load inode */
	ext4_inode_ref_t *inode_ref = enode->inode_ref;
	rc = ext4_filesystem_get_inode_data_block_index(inode_ref, iblock,
	    &fblock);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		goto exit;
	}
	
	/* Check for sparse file */
	if (fblock == 0) {
		if ((ext4_superblock_has_feature_incompatible(fs->superblock,
		    EXT4_FEATURE_INCOMPAT_EXTENTS)) &&
		    (ext4_inode_has_flag(inode_ref->inode, EXT4_INODE_FLAG_EXTENTS))) {
			uint32_t last_iblock =
			    ext4_inode_get_size(fs->superblock, inode_ref->inode) /
			    block_size;
			
			while (last_iblock < iblock) {
				rc = ext4_extent_append_block(inode_ref, &last_iblock,
				    &fblock, true);
				if (rc != EOK) {
					async_answer_0(callid, rc);
					goto exit;
				}
			}
			
			rc = ext4_extent_append_block(inode_ref, &last_iblock,
			    &fblock, false);
			if (rc != EOK) {
				async_answer_0(callid, rc);
				goto exit;
			}
		} else {
			rc = ext4_balloc_alloc_block(inode_ref, &fblock);
			if (rc != EOK) {
				async_answer_0(callid, rc);
				goto exit;
			}
			
			rc = ext4_filesystem_set_inode_data_block_index(inode_ref,
			    iblock, fblock);
			if (rc != EOK) {
				ext4_balloc_free_block(inode_ref, fblock);
				async_answer_0(callid, rc);
				goto exit;
			}
		}
		
		flags = BLOCK_FLAGS_NOREAD;
		inode_ref->dirty = true;
	}
	
	/* Load target block */
	block_t *write_block;
	rc = block_get(&write_block, service_id, fblock, flags);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		goto exit;
	}
	
	if (flags == BLOCK_FLAGS_NOREAD)
		memset(write_block->data, 0, block_size);

	rc = async_data_write_finalize(callid, write_block->data +
	    (pos % block_size), bytes);
	if (rc != EOK) {
		block_put(write_block);
		goto exit;
	}

	write_block->dirty = true;

	rc = block_put(write_block);
	if (rc != EOK)
		goto exit;

	/* Do some counting */
	uint32_t old_inode_size = ext4_inode_get_size(fs->superblock,
	    inode_ref->inode);
	if (pos + bytes > old_inode_size) {
		ext4_inode_set_size(inode_ref->inode, pos + bytes);
		inode_ref->dirty = true;
	}

	*nsize = ext4_inode_get_size(fs->superblock, inode_ref->inode);
	*wbytes = bytes;

exit:
	;

	errno_t const rc2 = ext4_node_put(fn);
	return rc == EOK ? rc2 : rc;
}

/** Truncate file.
 *
 * Only the direction to shorter file is supported.
 *
 * @param service_id Device identifier
 * @param index      Index if node to truncated
 * @param new_size   New size of file
 *
 * @return Error code
 *
 */
static errno_t ext4_truncate(service_id_t service_id, fs_index_t index,
    aoff64_t new_size)
{
	fs_node_t *fn;
	errno_t rc = ext4_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	
	ext4_node_t *enode = EXT4_NODE(fn);
	ext4_inode_ref_t *inode_ref = enode->inode_ref;
	
	rc = ext4_filesystem_truncate_inode(inode_ref, new_size);
	errno_t const rc2 = ext4_node_put(fn);
	
	return rc == EOK ? rc2 : rc;
}

/** Close file.
 *
 * @param service_id Device identifier
 * @param index      I-node number
 *
 * @return Error code
 *
 */
static errno_t ext4_close(service_id_t service_id, fs_index_t index)
{
	return EOK;
}

/** Destroy node specified by index.
 *
 * @param service_id Device identifier
 * @param index      I-node to destroy
 *
 * @return Error code
 *
 */
static errno_t ext4_destroy(service_id_t service_id, fs_index_t index)
{
	fs_node_t *fn;
	errno_t rc = ext4_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	
	/* Destroy the inode */
	return ext4_destroy_node(fn);
}

/** Enforce inode synchronization (write) to device.
 *
 * @param service_id Device identifier
 * @param index      I-node number.
 *
 */
static errno_t ext4_sync(service_id_t service_id, fs_index_t index)
{
	fs_node_t *fn;
	errno_t rc = ext4_node_get(&fn, service_id, index);
	if (rc != EOK)
		return rc;
	
	ext4_node_t *enode = EXT4_NODE(fn);
	enode->inode_ref->dirty = true;
	
	return ext4_node_put(fn);
}

/** VFS operations
 *
 */
vfs_out_ops_t ext4_ops = {
	.fsprobe = ext4_fsprobe,
	.mounted = ext4_mounted,
	.unmounted = ext4_unmounted,
	.read = ext4_read,
	.write = ext4_write,
	.truncate = ext4_truncate,
	.close = ext4_close,
	.destroy = ext4_destroy,
	.sync = ext4_sync
};

/**
 * @}
 */
