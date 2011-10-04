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
#include <malloc.h>
#include <stdio.h>
#include <ipc/loc.h>
#include "ext4fs.h"
#include "../../vfs/vfs.h"

#define EXT4FS_NODE(node)	((node) ? (ext4fs_node_t *) (node)->data : NULL)
#define EXT4FS_DBG(format, ...) {if (true) printf("ext4fs: %s: " format "\n", __FUNCTION__, ##__VA_ARGS__);}

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
static FIBRIL_MUTEX_INITIALIZE(open_nodes_lock);


/**
 *	TODO doxy
 */
int ext4fs_global_init(void)
{
	// TODO
	return EOK;
}

/**
 * TODO doxy
 */
int ext4fs_global_fini(void)
{
	// TODO
	return EOK;
}


/*
 * EXT4 libfs operations.
 */

/**
 * TODO doxy
 */
int ext4fs_instance_get(service_id_t service_id, ext4fs_instance_t **inst)
{
	ext4fs_instance_t *tmp;

	fibril_mutex_lock(&instance_list_mutex);

	if (list_empty(&instance_list)) {
		fibril_mutex_unlock(&instance_list_mutex);
		return EINVAL;
	}

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

/**
 * TODO doxy
 */
int ext4fs_root_get(fs_node_t **rfn, service_id_t service_id)
{
	return ext4fs_node_get(rfn, service_id, EXT4_INODE_ROOT_INDEX);
}

/**
 * TODO doxy
 */
int ext4fs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	// TODO
	return EOK;
}

/**
 * TODO doxy
 */
int ext4fs_node_get(fs_node_t **rfn, service_id_t service_id, fs_index_t index)
{
	ext4fs_instance_t *inst = NULL;
	int rc;

	rc = ext4fs_instance_get(service_id, &inst);
	if (rc != EOK) {
		return rc;
	}

	return ext4fs_node_get_core(rfn, inst, index);
}

/**
 * TODO doxy
 */
int ext4fs_node_get_core(fs_node_t **rfn, ext4fs_instance_t *inst,
		fs_index_t index)
{
	// TODO
	return EOK;
}

/**
 * TODO doxy
 */
int ext4fs_node_put_core(ext4fs_node_t *enode) {
	// TODO
	return EOK;
}

/**
 * TODO doxy
 */
int ext4fs_node_open(fs_node_t *fn)
{
	// TODO stateless operation
	return EOK;
}

int ext4fs_node_put(fs_node_t *fn)
{
	EXT4FS_DBG("");
	int rc;
	ext4fs_node_t *enode = EXT4FS_NODE(fn);

	fibril_mutex_lock(&open_nodes_lock);

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
	// TODO
	return ENOTSUP;
}

int ext4fs_destroy_node(fs_node_t *fn)
{
	// TODO
	return ENOTSUP;
}

int ext4fs_link(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	// TODO
	return ENOTSUP;
}

int ext4fs_unlink(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	// TODO
	return ENOTSUP;
}

int ext4fs_has_children(bool *has_children, fs_node_t *fn)
{
	// TODO
	return EOK;
}


fs_index_t ext4fs_index_get(fs_node_t *fn)
{
	// TODO
	return 0;
}

aoff64_t ext4fs_size_get(fs_node_t *fn)
{
	// TODO
	return 0;
}

unsigned ext4fs_lnkcnt_get(fs_node_t *fn)
{
	// TODO
	return 0;
}

bool ext4fs_is_directory(fs_node_t *fn)
{
	// TODO
	return false;
}

bool ext4fs_is_file(fs_node_t *fn)
{
	// TODO
	return false;
}

service_id_t ext4fs_service_get(fs_node_t *fn)
{
	// TODO
	return 0;
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

/**
 * TODO doxy
 */
static int ext4fs_mounted(service_id_t service_id, const char *opts,
   fs_index_t *index, aoff64_t *size, unsigned *lnkcnt)
{

	EXT4FS_DBG("Mounting...");

	int rc;
	ext4_filesystem_t *fs;
	ext4fs_instance_t *inst;
	bool read_only;

	/* Allocate libext4 filesystem structure */
	fs = (ext4_filesystem_t *) malloc(sizeof(ext4_filesystem_t));
	if (fs == NULL) {
		return ENOMEM;
	}

	/* Allocate instance structure */
	inst = (ext4fs_instance_t *) malloc(sizeof(ext4fs_instance_t));
	if (inst == NULL) {
		free(fs);
		return ENOMEM;
	}

	EXT4FS_DBG("Basic structures allocated");

	/* Initialize the filesystem */
	rc = ext4_filesystem_init(fs, service_id);
	if (rc != EOK) {
		free(fs);
		free(inst);
		return rc;
	}

	EXT4FS_DBG("initialized");

	/* Do some sanity checking */
	rc = ext4_filesystem_check_sanity(fs);
	if (rc != EOK) {
		ext4_filesystem_fini(fs);
		free(fs);
		free(inst);
		return rc;
	}

	EXT4FS_DBG("Checked and clean");

	/* Check flags */
	rc = ext4_filesystem_check_features(fs, &read_only);
	if (rc != EOK) {
		ext4_filesystem_fini(fs);
		free(fs);
		free(inst);
		return rc;
	}

	EXT4FS_DBG("Features checked");

	/* Initialize instance */
	link_initialize(&inst->link);
	inst->service_id = service_id;
	inst->filesystem = fs;
	inst->open_nodes_count = 0;

	EXT4FS_DBG("Instance initialized");

	/* Read root node */
	fs_node_t *root_node;
	rc = ext4fs_root_get(&root_node, inst->service_id);
	if (rc != EOK) {
		ext4_filesystem_fini(fs);
		free(fs);
		free(inst);
		return rc;
	}
	ext4fs_node_t *enode = EXT4FS_NODE(root_node);

	EXT4FS_DBG("Root node found");

	/* Add instance to the list */
	fibril_mutex_lock(&instance_list_mutex);
	list_append(&inst->link, &instance_list);
	fibril_mutex_unlock(&instance_list_mutex);

	EXT4FS_DBG("Instance added");

	*index = EXT4_INODE_ROOT_INDEX;
	*size = 0;
	*lnkcnt = ext4_inode_get_usage_count(enode->inode_ref->inode);

	EXT4FS_DBG("Return values set");

	ext4fs_node_put(root_node);

	EXT4FS_DBG("Mounting finished");

	return EOK;
}

/**
 * TODO doxy
 */
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

	ext4_filesystem_fini(inst->filesystem);

	return EOK;
}

/**
 * TODO doxy
 */
static int
ext4fs_read(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *rbytes)
{
	// TODO
	return 0;
}

/**
 * TODO doxy
 */
static int
ext4fs_write(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *wbytes, aoff64_t *nsize)
{
	// TODO
	return ENOTSUP;
}

/**
 * TODO doxy
 */
static int
ext4fs_truncate(service_id_t service_id, fs_index_t index, aoff64_t size)
{
	// TODO
	return ENOTSUP;
}

/**
 * TODO doxy
 */
static int ext4fs_close(service_id_t service_id, fs_index_t index)
{
	// TODO
	return EOK;
}

/**
 * TODO doxy
 */
static int ext4fs_destroy(service_id_t service_id, fs_index_t index)
{
	//TODO
	return ENOTSUP;
}

/**
 * TODO doxy
 */
static int ext4fs_sync(service_id_t service_id, fs_index_t index)
{
	// TODO
	return ENOTSUP;
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
