/*
 * Copyright (c) 2011 Frantisek Princ
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
 * @file	ext4fs_ops.c
 * @brief	VFS operations for EXT4 filesystem.
 */

#include <errno.h>
#include <fibril_synch.h>
#include <libext4.h>
#include <libfs.h>
#include <macros.h>
#include <malloc.h>
#include <string.h>
#include <adt/hash_table.h>
#include <ipc/loc.h>
#include "ext4fs.h"
#include "../../vfs/vfs.h"

#define EXT4FS_NODE(node)	((node) ? (ext4fs_node_t *) (node)->data : NULL)

#define OPEN_NODES_KEYS 2
#define OPEN_NODES_DEV_HANDLE_KEY 0
#define OPEN_NODES_INODE_KEY 1
#define OPEN_NODES_BUCKETS 256

typedef struct ext4fs_instance {
	link_t link;
	service_id_t service_id;
	ext4_filesystem_t *filesystem;
	unsigned int open_nodes_count;
} ext4fs_instance_t;

typedef struct ext4fs_node {
	ext4fs_instance_t *instance;
	ext4_inode_ref_t *inode_ref;
	fs_node_t *fs_node;
	link_t link;
	unsigned int references;
} ext4fs_node_t;

/*
 * Forward declarations of auxiliary functions
 */

static int ext4fs_read_directory(ipc_callid_t, aoff64_t, size_t,
    ext4fs_instance_t *, ext4_inode_ref_t *, size_t *);
static int ext4fs_read_file(ipc_callid_t, aoff64_t, size_t, ext4fs_instance_t *,
    ext4_inode_ref_t *, size_t *);
static bool ext4fs_is_dots(const uint8_t *, size_t);
static int ext4fs_instance_get(service_id_t, ext4fs_instance_t **);
static int ext4fs_node_get_core(fs_node_t **, ext4fs_instance_t *, fs_index_t);
static int ext4fs_node_put_core(ext4fs_node_t *);

/*
 * Forward declarations of EXT4 libfs operations.
 */
static int ext4fs_root_get(fs_node_t **, service_id_t);
static int ext4fs_match(fs_node_t **, fs_node_t *, const char *);
static int ext4fs_node_get(fs_node_t **, service_id_t, fs_index_t);
static int ext4fs_node_open(fs_node_t *);
static int ext4fs_node_put(fs_node_t *);
static int ext4fs_create_node(fs_node_t **, service_id_t, int);
static int ext4fs_destroy_node(fs_node_t *);
static int ext4fs_link(fs_node_t *, fs_node_t *, const char *);
static int ext4fs_unlink(fs_node_t *, fs_node_t *, const char *);
static int ext4fs_has_children(bool *, fs_node_t *);
static fs_index_t ext4fs_index_get(fs_node_t *);
static aoff64_t ext4fs_size_get(fs_node_t *);
static unsigned ext4fs_lnkcnt_get(fs_node_t *);
static bool ext4fs_is_directory(fs_node_t *);
static bool ext4fs_is_file(fs_node_t *node);
static service_id_t ext4fs_service_get(fs_node_t *node);

/*
 * Static variables
 */
static LIST_INITIALIZE(instance_list);
static FIBRIL_MUTEX_INITIALIZE(instance_list_mutex);
static hash_table_t open_nodes;
static FIBRIL_MUTEX_INITIALIZE(open_nodes_lock);

/* Hash table interface for open nodes hash table */
static hash_index_t open_nodes_hash(unsigned long key[])
{
	/* TODO: This is very simple and probably can be improved */
	return key[OPEN_NODES_INODE_KEY] % OPEN_NODES_BUCKETS;
}

static int open_nodes_compare(unsigned long key[], hash_count_t keys,
    link_t *item)
{
	ext4fs_node_t *enode = hash_table_get_instance(item, ext4fs_node_t, link);
	assert(keys > 0);
	if (enode->instance->service_id !=
	    ((service_id_t) key[OPEN_NODES_DEV_HANDLE_KEY])) {
		return false;
	}
	if (keys == 1) {
		return true;
	}
	assert(keys == 2);
	return (enode->inode_ref->index == key[OPEN_NODES_INODE_KEY]);
}

static void open_nodes_remove_cb(link_t *link)
{
	/* We don't use remove callback for this hash table */
}

static hash_table_operations_t open_nodes_ops = {
	.hash = open_nodes_hash,
	.compare = open_nodes_compare,
	.remove_callback = open_nodes_remove_cb,
};


int ext4fs_global_init(void)
{
	if (!hash_table_create(&open_nodes, OPEN_NODES_BUCKETS,
	    OPEN_NODES_KEYS, &open_nodes_ops)) {
		return ENOMEM;
	}
	return EOK;
}


int ext4fs_global_fini(void)
{
	hash_table_destroy(&open_nodes);
	return EOK;
}


/*
 * EXT4 libfs operations.
 */

int ext4fs_instance_get(service_id_t service_id, ext4fs_instance_t **inst)
{
	fibril_mutex_lock(&instance_list_mutex);

	if (list_empty(&instance_list)) {
		fibril_mutex_unlock(&instance_list_mutex);
		return EINVAL;
	}

	ext4fs_instance_t *tmp;
	list_foreach(instance_list, link) {
		tmp = list_get_instance(link, ext4fs_instance_t, link);

		if (tmp->service_id == service_id) {
			*inst = tmp;
			fibril_mutex_unlock(&instance_list_mutex);
			return EOK;
		}
	}

	fibril_mutex_unlock(&instance_list_mutex);
	return EINVAL;
}


int ext4fs_root_get(fs_node_t **rfn, service_id_t service_id)
{
	return ext4fs_node_get(rfn, service_id, EXT4_INODE_ROOT_INDEX);
}


int ext4fs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	int rc;

	ext4fs_node_t *eparent = EXT4FS_NODE(pfn);
	ext4_filesystem_t *fs = eparent->instance->filesystem;

	if (!ext4_inode_is_type(fs->superblock, eparent->inode_ref->inode,
	    EXT4_INODE_MODE_DIRECTORY)) {
		return ENOTDIR;
	}

	ext4_directory_iterator_t it;
	rc = ext4_directory_iterator_init(&it, fs, eparent->inode_ref, 0);
	if (rc != EOK) {
		return rc;
	}

	rc = ext4_directory_find_entry(&it, eparent->inode_ref, component);
	if (rc != EOK) {
		ext4_directory_iterator_fini(&it);
		return rc;
	}

	uint32_t inode = ext4_directory_entry_ll_get_inode(it.current);

	rc = ext4fs_node_get_core(rfn, eparent->instance, inode);
	if (rc != EOK) {
		ext4_directory_iterator_fini(&it);
		return rc;
	}

	ext4_directory_iterator_fini(&it);
	return EOK;
}


int ext4fs_node_get(fs_node_t **rfn, service_id_t service_id, fs_index_t index)
{
	int rc;

	ext4fs_instance_t *inst;
	rc = ext4fs_instance_get(service_id, &inst);
	if (rc != EOK) {
		return rc;
	}

	return ext4fs_node_get_core(rfn, inst, index);
}


int ext4fs_node_get_core(fs_node_t **rfn, ext4fs_instance_t *inst,
		fs_index_t index)
{
	int rc;

	fibril_mutex_lock(&open_nodes_lock);

	/* Check if the node is not already open */
	unsigned long key[] = {
		[OPEN_NODES_DEV_HANDLE_KEY] = inst->service_id,
		[OPEN_NODES_INODE_KEY] = index,
	};

	link_t *already_open = hash_table_find(&open_nodes, key);
	ext4fs_node_t *enode = NULL;
	if (already_open) {
		enode = hash_table_get_instance(already_open, ext4fs_node_t, link);
		*rfn = enode->fs_node;
		enode->references++;

		fibril_mutex_unlock(&open_nodes_lock);
		return EOK;
	}

	enode = malloc(sizeof(ext4fs_node_t));
	if (enode == NULL) {
		fibril_mutex_unlock(&open_nodes_lock);
		return ENOMEM;
	}

	fs_node_t *fs_node = malloc(sizeof(fs_node_t));
	if (fs_node == NULL) {
		free(enode);
		fibril_mutex_unlock(&open_nodes_lock);
		return ENOMEM;
	}
	fs_node_initialize(fs_node);

	ext4_inode_ref_t *inode_ref;
	rc = ext4_filesystem_get_inode_ref(inst->filesystem, index, &inode_ref);
	if (rc != EOK) {
		free(enode);
		free(fs_node);
		fibril_mutex_unlock(&open_nodes_lock);
		return rc;
	}

	enode->inode_ref = inode_ref;
	enode->instance = inst;
	enode->references = 1;
	enode->fs_node = fs_node;
	link_initialize(&enode->link);

	fs_node->data = enode;
	*rfn = fs_node;

	hash_table_insert(&open_nodes, key, &enode->link);
	inst->open_nodes_count++;

	fibril_mutex_unlock(&open_nodes_lock);

	return EOK;
}


int ext4fs_node_put_core(ext4fs_node_t *enode)
{
	int rc;
	unsigned long key[] = {
		[OPEN_NODES_DEV_HANDLE_KEY] = enode->instance->service_id,
		[OPEN_NODES_INODE_KEY] = enode->inode_ref->index,
	};

	hash_table_remove(&open_nodes, key, OPEN_NODES_KEYS);
	assert(enode->instance->open_nodes_count > 0);
	enode->instance->open_nodes_count--;

	rc = ext4_filesystem_put_inode_ref(enode->inode_ref);
	if (rc != EOK) {
		return rc;
	}

	free(enode->fs_node);
	free(enode);

	return EOK;
}


int ext4fs_node_open(fs_node_t *fn)
{
	// Stateless operation
	return EOK;
}

int ext4fs_node_put(fs_node_t *fn)
{
	int rc;

	fibril_mutex_lock(&open_nodes_lock);

	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	assert(enode->references > 0);
	enode->references--;
	if (enode->references == 0) {
		rc = ext4fs_node_put_core(enode);
		if (rc != EOK) {
			fibril_mutex_unlock(&open_nodes_lock);
			return rc;
		}
	}

	fibril_mutex_unlock(&open_nodes_lock);

	return EOK;
}


int ext4fs_create_node(fs_node_t **rfn, service_id_t service_id, int flags)
{
	EXT4FS_DBG("");
	int rc;

	ext4fs_node_t *enode;
	enode = malloc(sizeof(ext4fs_node_t));
	if (enode == NULL) {
		return ENOMEM;
	}

	fs_node_t *fs_node;
	fs_node = malloc(sizeof(fs_node_t));
	if (fs_node == NULL) {
		free(enode);
		return ENOMEM;
	}

	ext4fs_instance_t *inst;
	rc = ext4fs_instance_get(service_id, &inst);
	if (rc != EOK) {
		free(enode);
		free(fs_node);
		return rc;
	}

	ext4_inode_ref_t *inode_ref;
	rc = ext4_filesystem_alloc_inode(inst->filesystem, &inode_ref, flags);
	if (rc != EOK) {
		free(enode);
		free(fs_node);
		return rc;
	}

	EXT4FS_DBG("allocated");

	enode->inode_ref = inode_ref;
	enode->instance = inst;
	enode->references = 1;

	link_initialize(&enode->link);

	unsigned long key[] = {
		[OPEN_NODES_DEV_HANDLE_KEY] = inst->service_id,
		[OPEN_NODES_INODE_KEY] = inode_ref->index,
	};

	fibril_mutex_lock(&open_nodes_lock);
	hash_table_insert(&open_nodes, key, &enode->link);
	fibril_mutex_unlock(&open_nodes_lock);
	inst->open_nodes_count++;

	enode->inode_ref->dirty = true;

	fs_node_initialize(fs_node);
	fs_node->data = enode;
	enode->fs_node = fs_node;
	*rfn = fs_node;

	EXT4FS_DBG("finished");

	// TODO
	return EOK;
}


int ext4fs_destroy_node(fs_node_t *fn)
{
	EXT4FS_DBG("");
	int rc;

	bool has_children;
	rc = ext4fs_has_children(&has_children, fn);
	if (rc != EOK) {
		ext4fs_node_put(fn);
		return rc;
	}

	if (has_children) {
		EXT4FS_DBG("destroying non-empty node");
		ext4fs_node_put(fn);
		return EINVAL;
	}

	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	ext4_filesystem_t *fs = enode->instance->filesystem;
	ext4_inode_ref_t *inode_ref = enode->inode_ref;

	rc = ext4_filesystem_truncate_inode(fs, inode_ref, 0);
	if (rc != EOK) {
		ext4fs_node_put(fn);
		return rc;
	}

	uint32_t rev_level = ext4_superblock_get_rev_level(fs->superblock);
	if (rev_level > 0) {
		ext4_filesystem_delete_orphan(fs, inode_ref);
	}

	// TODO set real deletion time
//	time_t now = time(NULL);
	time_t now = ext4_inode_get_change_inode_time(inode_ref->inode);
	ext4_inode_set_deletion_time(inode_ref->inode, (uint32_t)now);
	inode_ref->dirty = true;

	rc = ext4_filesystem_free_inode(fs, inode_ref);
	if (rc != EOK) {
		ext4fs_node_put(fn);
		return rc;
	}

	ext4fs_node_put(fn);
	return EOK;
}


int ext4fs_link(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	EXT4FS_DBG("");

	int rc;

	// Check maximum name length
	if (strlen(name) > EXT4_DIRECTORY_FILENAME_LEN) {
		return ENAMETOOLONG;
	}

	EXT4FS_DBG("name checked");

	ext4fs_node_t *parent = EXT4FS_NODE(pfn);
	ext4fs_node_t *child = EXT4FS_NODE(cfn);
	ext4_filesystem_t *fs = parent->instance->filesystem;

	// Add entry to parent directory
	rc = ext4_directory_add_entry(fs, parent->inode_ref, name, child->inode_ref);
	if (rc != EOK) {
		return rc;
	}

	EXT4FS_DBG("dentry added");

	// Fill new dir -> add '.' and '..' entries
	if (ext4_inode_is_type(fs->superblock, child->inode_ref->inode, EXT4_INODE_MODE_DIRECTORY)) {

		rc = ext4_directory_add_entry(fs, child->inode_ref, ".", child->inode_ref);
		if (rc != EOK) {
			ext4_directory_remove_entry(fs, parent->inode_ref, name);
			return rc;
		}

		EXT4FS_DBG("added dot");

		rc = ext4_directory_add_entry(fs, child->inode_ref, "..", parent->inode_ref);
		if (rc != EOK) {
			ext4_directory_remove_entry(fs, parent->inode_ref, name);
			ext4_directory_remove_entry(fs, child->inode_ref, ".");
			return rc;
		}

		EXT4FS_DBG("added dotdot");

		uint16_t parent_links = ext4_inode_get_links_count(parent->inode_ref->inode);
		parent_links++;
		ext4_inode_set_links_count(parent->inode_ref->inode, parent_links);

		parent->inode_ref->dirty = true;

	}

	uint16_t child_links = ext4_inode_get_links_count(child->inode_ref->inode);
	child_links++;
	ext4_inode_set_links_count(child->inode_ref->inode, child_links);

	child->inode_ref->dirty = true;

	return EOK;
}


int ext4fs_unlink(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
//	EXT4FS_DBG("unlinking \%s", name);

	int rc;

	bool has_children;
	rc = ext4fs_has_children(&has_children, cfn);
	if (rc != EOK) {
		EXT4FS_DBG("\%s error: \%u", name, rc);
		return rc;
	}

	// Cannot unlink non-empty node
	if (has_children) {
		return ENOTEMPTY;
	}

	// Remove entry from parent directory
	ext4_inode_ref_t *parent = EXT4FS_NODE(pfn)->inode_ref;
	ext4_filesystem_t *fs = EXT4FS_NODE(pfn)->instance->filesystem;
	rc = ext4_directory_remove_entry(fs, parent, name);
	if (rc != EOK) {
		EXT4FS_DBG("\%s removing entry failed: \%u", name, rc);
		return rc;
	}

	// Decrement links count
	ext4_inode_ref_t * child_inode_ref = EXT4FS_NODE(cfn)->inode_ref;

	uint32_t lnk_count = ext4_inode_get_links_count(child_inode_ref->inode);
	lnk_count--;

	// If directory - handle links from parent
	if (lnk_count <= 1 && ext4fs_is_directory(cfn)) {

		assert(lnk_count == 1);
		lnk_count--;

		ext4_inode_ref_t *parent_inode_ref = EXT4FS_NODE(pfn)->inode_ref;

		uint32_t parent_lnk_count = ext4_inode_get_links_count(
				parent_inode_ref->inode);

		parent_lnk_count--;
		ext4_inode_set_links_count(parent_inode_ref->inode, parent_lnk_count);

		parent->dirty = true;
	}

	uint32_t rev_level = ext4_superblock_get_rev_level(fs->superblock);
	if ((rev_level > 0) && (lnk_count == 0)) {
		ext4_filesystem_add_orphan(fs, child_inode_ref);
	}

	// TODO set timestamps for parent (when we have wall-clock time)
//	time_t now = time(NULL);
//	ext4_inode_set_change_inode_time(parent->inode, (uint32_t)now);
//	ext4_inode_set_modification_time(parent->inode, (uint32_t)now);
//	parent->dirty = true;

	// TODO set timestamp for inode
//	ext4_inode_set_change_inode_time(child_inode_ref->inode, (uint32_t)now);
	ext4_inode_set_links_count(child_inode_ref->inode, lnk_count);
	child_inode_ref->dirty = true;

	return EOK;
}


int ext4fs_has_children(bool *has_children, fs_node_t *fn)
{
	int rc;

	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	ext4_filesystem_t *fs = enode->instance->filesystem;

	if (!ext4_inode_is_type(fs->superblock, enode->inode_ref->inode,
	    EXT4_INODE_MODE_DIRECTORY)) {
		*has_children = false;
		return EOK;
	}

	ext4_directory_iterator_t it;
	rc = ext4_directory_iterator_init(&it, fs, enode->inode_ref, 0);
	if (rc != EOK) {
		return rc;
	}

	/* Find a non-empty directory entry */
	bool found = false;
	while (it.current != NULL) {
		if (it.current->inode != 0) {
			uint16_t name_size = ext4_directory_entry_ll_get_name_length(fs->superblock,
				it.current);
			if (!ext4fs_is_dots(it.current->name, name_size)) {
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
	if (rc != EOK) {
		return rc;
	}

	*has_children = found;

	return EOK;
}


fs_index_t ext4fs_index_get(fs_node_t *fn)
{
	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	return enode->inode_ref->index;
}


aoff64_t ext4fs_size_get(fs_node_t *fn)
{
	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	ext4_superblock_t *sb = enode->instance->filesystem->superblock;
	return ext4_inode_get_size(sb, enode->inode_ref->inode);
}


unsigned ext4fs_lnkcnt_get(fs_node_t *fn)
{
	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	uint32_t lnkcnt = ext4_inode_get_links_count(enode->inode_ref->inode);

	if (ext4fs_is_directory(fn)) {
		if (lnkcnt > 1) {
			return 1;
		} else {
			return 0;
		}
	}

	// For regular files return real links count
	return lnkcnt;
}


bool ext4fs_is_directory(fs_node_t *fn)
{
	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	ext4_superblock_t *sb = enode->instance->filesystem->superblock;
	return ext4_inode_is_type(
			sb, enode->inode_ref->inode, EXT4_INODE_MODE_DIRECTORY);
}


bool ext4fs_is_file(fs_node_t *fn)
{
	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	ext4_superblock_t *sb = enode->instance->filesystem->superblock;
	return ext4_inode_is_type(
			sb, enode->inode_ref->inode, EXT4_INODE_MODE_FILE);
}


service_id_t ext4fs_service_get(fs_node_t *fn)
{
	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	return enode->instance->service_id;
}

/*
 * libfs operations.
 */
libfs_ops_t ext4fs_libfs_ops = {
	.root_get = ext4fs_root_get,
	.match = ext4fs_match,
	.node_get = ext4fs_node_get,
	.node_open = ext4fs_node_open,
	.node_put = ext4fs_node_put,
	.create = ext4fs_create_node,
	.destroy = ext4fs_destroy_node,
	.link = ext4fs_link,
	.unlink = ext4fs_unlink,
	.has_children = ext4fs_has_children,
	.index_get = ext4fs_index_get,
	.size_get = ext4fs_size_get,
	.lnkcnt_get = ext4fs_lnkcnt_get,
	.is_directory = ext4fs_is_directory,
	.is_file = ext4fs_is_file,
	.service_get = ext4fs_service_get
};


/*
 * VFS operations.
 */

static int ext4fs_mounted(service_id_t service_id, const char *opts,
   fs_index_t *index, aoff64_t *size, unsigned *lnkcnt)
{
	int rc;

	/* Allocate libext4 filesystem structure */
	ext4_filesystem_t *fs;
	fs = (ext4_filesystem_t *) malloc(sizeof(ext4_filesystem_t));
	if (fs == NULL) {
		return ENOMEM;
	}

	/* Allocate instance structure */
	ext4fs_instance_t *inst;
	inst = (ext4fs_instance_t *) malloc(sizeof(ext4fs_instance_t));
	if (inst == NULL) {
		free(fs);
		return ENOMEM;
	}

	/* Initialize the filesystem */
	rc = ext4_filesystem_init(fs, service_id);
	if (rc != EOK) {
		free(fs);
		free(inst);
		return rc;
	}

	/* Do some sanity checking */
	rc = ext4_filesystem_check_sanity(fs);
	if (rc != EOK) {
		ext4_filesystem_fini(fs, false);
		free(fs);
		free(inst);
		return rc;
	}

	/* Check flags */
	bool read_only;
	rc = ext4_filesystem_check_features(fs, &read_only);
	if (rc != EOK) {
		ext4_filesystem_fini(fs, false);
		free(fs);
		free(inst);
		return rc;
	}

	/* Initialize instance */
	link_initialize(&inst->link);
	inst->service_id = service_id;
	inst->filesystem = fs;
	inst->open_nodes_count = 0;

	/* Read root node */
	fs_node_t *root_node;
	rc = ext4fs_node_get_core(&root_node, inst, EXT4_INODE_ROOT_INDEX);
	if (rc != EOK) {
		ext4_filesystem_fini(fs, false);
		free(fs);
		free(inst);
		return rc;
	}

	/* Add instance to the list */
	fibril_mutex_lock(&instance_list_mutex);
	list_append(&inst->link, &instance_list);
	fibril_mutex_unlock(&instance_list_mutex);

	*index = EXT4_INODE_ROOT_INDEX;
	*size = 0;
	*lnkcnt = 1;

	ext4fs_node_put(root_node);

	return EOK;
}


static int ext4fs_unmounted(service_id_t service_id)
{
	int rc;

	ext4fs_instance_t *inst;
	rc = ext4fs_instance_get(service_id, &inst);
	if (rc != EOK) {
		return rc;
	}

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

	return ext4_filesystem_fini(inst->filesystem, true);
}


static int ext4fs_read(service_id_t service_id, fs_index_t index,
		aoff64_t pos, size_t *rbytes)
{
	int rc;

	/*
	 * Receive the read request.
	 */
	ipc_callid_t callid;
	size_t size;
	if (!async_data_read_receive(&callid, &size)) {
		async_answer_0(callid, EINVAL);
		return EINVAL;
	}

	ext4fs_instance_t *inst;
	rc = ext4fs_instance_get(service_id, &inst);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return rc;
	}

	ext4_inode_ref_t *inode_ref;
	rc = ext4_filesystem_get_inode_ref(inst->filesystem, index, &inode_ref);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return rc;
	}

	if (ext4_inode_is_type(inst->filesystem->superblock, inode_ref->inode,
			EXT4_INODE_MODE_FILE)) {
		rc = ext4fs_read_file(callid, pos, size, inst, inode_ref,
				rbytes);
	} else if (ext4_inode_is_type(inst->filesystem->superblock,
			inode_ref->inode, EXT4_INODE_MODE_DIRECTORY)) {
		rc = ext4fs_read_directory(callid, pos, size, inst, inode_ref,
				rbytes);
	} else {
		/* Other inode types not supported */
		async_answer_0(callid, ENOTSUP);
		rc = ENOTSUP;
	}

	ext4_filesystem_put_inode_ref(inode_ref);

	return rc;
}

bool ext4fs_is_dots(const uint8_t *name, size_t name_size)
{
	if (name_size == 1 && name[0] == '.') {
		return true;
	}

	if (name_size == 2 && name[0] == '.' && name[1] == '.') {
		return true;
	}

	return false;
}

int ext4fs_read_directory(ipc_callid_t callid, aoff64_t pos, size_t size,
    ext4fs_instance_t *inst, ext4_inode_ref_t *inode_ref, size_t *rbytes)
{
	int rc;

	ext4_directory_iterator_t it;
	rc = ext4_directory_iterator_init(&it, inst->filesystem, inode_ref, pos);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return rc;
	}

	/* Find next interesting directory entry.
	 * We want to skip . and .. entries
	 * as these are not used in HelenOS
	 */
	bool found = false;
	while (it.current != NULL) {

		if (it.current->inode == 0) {
			goto skip;
		}

		uint16_t name_size = ext4_directory_entry_ll_get_name_length(
		    inst->filesystem->superblock, it.current);

		/* skip . and .. */
		if (ext4fs_is_dots(it.current->name, name_size)) {
			goto skip;
		}

		/* The on-disk entry does not contain \0 at the end
		 * end of entry name, so we copy it to new buffer
		 * and add the \0 at the end
		 */
		uint8_t *buf = malloc(name_size+1);
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
		if (rc != EOK) {
			return rc;
		}
		next = it.current_offset;
	}

	rc = ext4_directory_iterator_fini(&it);
	if (rc != EOK) {
		return rc;
	}

	if (found) {
		*rbytes = next - pos;
		return EOK;
	} else {
		async_answer_0(callid, ENOENT);
		return ENOENT;
	}
}

int ext4fs_read_file(ipc_callid_t callid, aoff64_t pos, size_t size,
    ext4fs_instance_t *inst, ext4_inode_ref_t *inode_ref, size_t *rbytes)
{
	int rc;

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
	if (pos + bytes > file_size) {
		bytes = file_size - pos;
	}

	/* Get the real block number */
	uint32_t fs_block;
	rc = ext4_filesystem_get_inode_data_block_index(inst->filesystem,
		inode_ref->inode, file_block, &fs_block);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return rc;
	}

	/* Check for sparse file
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

		async_data_read_finalize(callid, buffer, bytes);
		*rbytes = bytes;

		free(buffer);

		return EOK;
	}

	/* Usual case - we need to read a block from device */
	block_t *block;
	rc = block_get(&block, inst->service_id, fs_block, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		async_answer_0(callid, rc);
		return rc;
	}

	assert(offset_in_block + bytes <= block_size);
	async_data_read_finalize(callid, block->data + offset_in_block, bytes);

	rc = block_put(block);
	if (rc != EOK) {
		return rc;
	}

	*rbytes = bytes;
	return EOK;
}

static int ext4fs_write(service_id_t service_id, fs_index_t index,
		aoff64_t pos, size_t *wbytes, aoff64_t *nsize)
{
	int rc;

	fs_node_t *fn;
	rc = ext4fs_node_get(&fn, service_id, index);
	if (rc != EOK) {
		EXT4FS_DBG("node get error");
		return rc;
	}

	ipc_callid_t callid;
	size_t len;
	if (!async_data_write_receive(&callid, &len)) {
		rc = EINVAL;
		ext4fs_node_put(fn);
		async_answer_0(callid, rc);
		EXT4FS_DBG("data write recv");
		return rc;
	}


	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	ext4_filesystem_t *fs = enode->instance->filesystem;

	uint32_t block_size = ext4_superblock_get_block_size(fs->superblock);

	// Prevent writing to more than one block
	uint32_t bytes = min(len, block_size - (pos % block_size));

	int flags = BLOCK_FLAGS_NONE;
	if (bytes == block_size) {
		flags = BLOCK_FLAGS_NOREAD;
	}

	uint32_t iblock =  pos / block_size;
	uint32_t fblock;

	ext4_inode_ref_t *inode_ref = enode->inode_ref;
	rc = ext4_filesystem_get_inode_data_block_index(fs, inode_ref->inode, iblock, &fblock);
	if (rc != EOK) {
		// TODO error
		ext4fs_node_put(fn);
		EXT4FS_DBG("error loading block addr");
		return rc;
	}

	if (fblock == 0) {
		rc =  ext4_balloc_alloc_block(fs, inode_ref, &fblock);
		if (rc != EOK) {
			ext4fs_node_put(fn);
			async_answer_0(callid, rc);
			return rc;
		}

		rc = ext4_filesystem_set_inode_data_block_index(fs, inode_ref, iblock, fblock);
		if (rc != EOK) {
			EXT4FS_DBG("ERROR: setting index failed");
		}
		inode_ref->dirty = true;

		flags = BLOCK_FLAGS_NOREAD;
	}

	block_t *write_block;
	rc = block_get(&write_block, service_id, fblock, flags);
	if (rc != EOK) {
		EXT4FS_DBG("error in loading block \%d", rc);
		ext4fs_node_put(fn);
		async_answer_0(callid, rc);
		return rc;
	}

	if (flags == BLOCK_FLAGS_NOREAD) {
		memset(write_block->data, 0, block_size);
	}

	rc = async_data_write_finalize(callid, write_block->data + (pos % block_size), bytes);
	if (rc != EOK) {
		// TODO error
		EXT4FS_DBG("error in write finalize \%d", rc);
	}

	write_block->dirty = true;

	rc = block_put(write_block);
	if (rc != EOK) {
		EXT4FS_DBG("error in writing block \%d", rc);
		ext4fs_node_put(fn);
		return rc;
	}

	uint32_t old_inode_size = ext4_inode_get_size(fs->superblock, inode_ref->inode);
	if (pos + bytes > old_inode_size) {
		ext4_inode_set_size(inode_ref->inode, pos + bytes);
		inode_ref->dirty = true;
	}

	*nsize = ext4_inode_get_size(fs->superblock, inode_ref->inode);
	*wbytes = bytes;

	return ext4fs_node_put(fn);
}


static int ext4fs_truncate(service_id_t service_id, fs_index_t index,
		aoff64_t new_size)
{
	int rc;

	fs_node_t *fn;
	rc = ext4fs_node_get(&fn, service_id, index);
	if (rc != EOK) {
		return rc;
	}

	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	ext4_inode_ref_t *inode_ref = enode->inode_ref;
	ext4_filesystem_t *fs = enode->instance->filesystem;

	rc = ext4_filesystem_truncate_inode(fs, inode_ref, new_size);
	ext4fs_node_put(fn);

	return rc;
}


static int ext4fs_close(service_id_t service_id, fs_index_t index)
{
	return EOK;
}


static int ext4fs_destroy(service_id_t service_id, fs_index_t index)
{
	int rc;

	fs_node_t *fn;
	rc = ext4fs_node_get(&fn, service_id, index);
	if (rc != EOK) {
		return rc;
	}

	/* Destroy the inode */
	return ext4fs_destroy_node(fn);
}


static int ext4fs_sync(service_id_t service_id, fs_index_t index)
{
	int rc;

	fs_node_t *fn;
	rc = ext4fs_node_get(&fn, service_id, index);
	if (rc != EOK) {
		return rc;
	}

	ext4fs_node_t *enode = EXT4FS_NODE(fn);
	enode->inode_ref->dirty = true;

	return ext4fs_node_put(fn);
}

vfs_out_ops_t ext4fs_ops = {
	.mounted = ext4fs_mounted,
	.unmounted = ext4fs_unmounted,
	.read = ext4fs_read,
	.write = ext4fs_write,
	.truncate = ext4fs_truncate,
	.close = ext4fs_close,
	.destroy = ext4fs_destroy,
	.sync = ext4fs_sync,
};

/**
 * @}
 */
