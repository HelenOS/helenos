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
/**
 * @file udf_ops.c
 * @brief Implementation of VFS operations for the UDF file system
 *        server.
 */

#include <libfs.h>
#include <block.h>
#include <ipc/services.h>
#include <ipc/loc.h>
#include <macros.h>
#include <async.h>
#include <errno.h>
#include <str.h>
#include <byteorder.h>
#include <adt/hash_table.h>
#include <adt/list.h>
#include <assert.h>
#include <fibril_synch.h>
#include <align.h>
#include <stdlib.h>
#include <inttypes.h>
#include <io/log.h>
#include "../../vfs/vfs.h"
#include "udf.h"
#include "udf_cksum.h"
#include "udf_volume.h"
#include "udf_idx.h"
#include "udf_file.h"
#include "udf_osta.h"

/** Mutex protecting the list of cached free nodes. */
static FIBRIL_MUTEX_INITIALIZE(ffn_mutex);

/** List of cached free nodes. */
static LIST_INITIALIZE(ffn_list);

static errno_t udf_node_get(fs_node_t **rfn, service_id_t service_id,
    fs_index_t index)
{
	udf_instance_t *instance;
	errno_t rc = fs_instance_get(service_id, (void **) &instance);
	if (rc != EOK)
		return rc;
	
	udf_node_t *node;
	rc = udf_idx_get(&node, instance, index);
	if (rc != EOK) {
		rc = udf_idx_add(&node, instance, index);
		if (rc != EOK)
			return rc;
		
		rc = udf_node_get_core(node);
		if (rc != EOK) {
			udf_idx_del(node);
			return rc;
		}
	}
	
	*rfn = FS_NODE(node);
	return EOK;
}

static errno_t udf_root_get(fs_node_t **rfn, service_id_t service_id)
{
	udf_instance_t *instance;
	errno_t rc = fs_instance_get(service_id, (void **) &instance);
	if (rc != EOK)
		return rc;
	
	return udf_node_get(rfn, service_id,
	    instance->volumes[DEFAULT_VOL].root_dir);
}

static service_id_t udf_service_get(fs_node_t *node)
{
	udf_node_t *udfn = UDF_NODE(node);
	if (udfn)
		return udfn->instance->service_id;
	
	return 0;
}

static errno_t udf_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	char *name = malloc(MAX_FILE_NAME_LEN + 1);
	if (name == NULL)
		return ENOMEM;
	
	block_t *block = NULL;
	udf_file_identifier_descriptor_t *fid = NULL;
	size_t pos = 0;
	
	while (udf_get_fid(&fid, &block, UDF_NODE(pfn), pos) == EOK) {
		udf_long_ad_t long_ad = fid->icb;
		
		udf_to_unix_name(name, MAX_FILE_NAME_LEN,
		    (char *) fid->implementation_use + FLE16(fid->lenght_iu),
		    fid->lenght_file_id, &UDF_NODE(pfn)->instance->charset);
		
		if (str_casecmp(name, component) == 0) {
			errno_t rc = udf_node_get(rfn, udf_service_get(pfn),
			    udf_long_ad_to_pos(UDF_NODE(pfn)->instance, &long_ad));
			
			if (block != NULL)
				block_put(block);
			
			free(name);
			return rc;
		}
		
		if (block != NULL) {
			errno_t rc = block_put(block);
			if (rc != EOK)
				return rc;
		}
		
		pos++;
	}
	
	free(name);
	return ENOENT;
}

static errno_t udf_node_open(fs_node_t *fn)
{
	return EOK;
}

static errno_t udf_node_put(fs_node_t *fn)
{
	udf_node_t *node = UDF_NODE(fn);
	if (!node)
		return EINVAL;
	
	fibril_mutex_lock(&node->lock);
	node->ref_cnt--;
	fibril_mutex_unlock(&node->lock);
	
	/* Delete node from hash table and memory */
	if (!node->ref_cnt)
		udf_idx_del(node);
	
	return EOK;
}

static errno_t udf_create_node(fs_node_t **rfn, service_id_t service_id, int flags)
{
	return ENOTSUP;
}

static errno_t udf_destroy_node(fs_node_t *fn)
{
	return ENOTSUP;
}

static errno_t udf_link(fs_node_t *pfn, fs_node_t *cfn, const char *name)
{
	return ENOTSUP;
}

static errno_t udf_unlink(fs_node_t *pfn, fs_node_t *cfn, const char *nm)
{
	return ENOTSUP;
}

static errno_t udf_has_children(bool *has_children, fs_node_t *fn)
{
	*has_children = true;
	return EOK;
}

static fs_index_t udf_index_get(fs_node_t *fn)
{
	udf_node_t *node = UDF_NODE(fn);
	if (node)
		return node->index;
	
	return 0;
}

static aoff64_t udf_size_get(fs_node_t *fn)
{
	udf_node_t *node = UDF_NODE(fn);
	if (node)
		return node->data_size;
	
	return 0;
}

static unsigned int udf_lnkcnt_get(fs_node_t *fn)
{
	udf_node_t *node = UDF_NODE(fn);
	if (node)
		return node->link_cnt;
	
	return 0;
}

static bool udf_is_directory(fs_node_t *fn)
{
	udf_node_t *node = UDF_NODE(fn);
	if (node)
		return node->type == NODE_DIR;
	
	return false;
}

static bool udf_is_file(fs_node_t *fn)
{
	udf_node_t *node = UDF_NODE(fn);
	if (node)
		return node->type == NODE_FILE;
	
	return false;
}

static errno_t udf_size_block(service_id_t service_id, uint32_t *size)
{
	udf_instance_t *instance;
	errno_t rc = fs_instance_get(service_id, (void **) &instance);
	if (rc != EOK)
		return rc;

	if (NULL == instance)
		return ENOENT;
	
	*size = instance->volumes[DEFAULT_VOL].logical_block_size;
	
	return EOK;
}

static errno_t udf_total_block_count(service_id_t service_id, uint64_t *count)
{
	*count = 0;
	
	return EOK;
}

static errno_t udf_free_block_count(service_id_t service_id, uint64_t *count)
{
	*count = 0;
	
	return EOK;
}

libfs_ops_t udf_libfs_ops = {
	.root_get = udf_root_get,
	.match = udf_match,
	.node_get = udf_node_get,
	.node_open = udf_node_open,
	.node_put = udf_node_put,
	.create = udf_create_node,
	.destroy = udf_destroy_node,
	.link = udf_link,
	.unlink = udf_unlink,
	.has_children = udf_has_children,
	.index_get = udf_index_get,
	.size_get = udf_size_get,
	.lnkcnt_get = udf_lnkcnt_get,
	.is_directory = udf_is_directory,
	.is_file = udf_is_file,
	.service_get = udf_service_get,
	.size_block = udf_size_block,
	.total_block_count = udf_total_block_count,
	.free_block_count = udf_free_block_count
};

static errno_t udf_fsprobe(service_id_t service_id, vfs_fs_probe_info_t *info)
{
	return ENOTSUP;
}

static errno_t udf_mounted(service_id_t service_id, const char *opts,
    fs_index_t *index, aoff64_t *size)
{
	enum cache_mode cmode;
	
	/* Check for option enabling write through. */
	if (str_cmp(opts, "wtcache") == 0)
		cmode = CACHE_MODE_WT;
	else
		cmode = CACHE_MODE_WB;
	
	udf_instance_t *instance = malloc(sizeof(udf_instance_t));
	if (!instance)
		return ENOMEM;
	
	instance->sector_size = 0;
	
	/* Check for block size. Will be enhanced later */
	if (str_cmp(opts, "bs=512") == 0)
		instance->sector_size = 512;
	else if (str_cmp(opts, "bs=1024") == 0)
		instance->sector_size = 1024;
	else if (str_cmp(opts, "bs=2048") == 0)
		instance->sector_size = 2048;
	
	/* initialize block cache */
	errno_t rc = block_init(service_id, MAX_SIZE);
	if (rc != EOK)
		return rc;
	
	rc = fs_instance_create(service_id, instance);
	if (rc != EOK) {
		free(instance);
		block_fini(service_id);
		return rc;
	}
	
	instance->service_id = service_id;
	instance->open_nodes_count = 0;
	
	/* Check Volume Recognition Sequence */
	rc = udf_volume_recongnition(service_id);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "VRS failed");
		fs_instance_destroy(service_id);
		free(instance);
		block_fini(service_id);
		return rc;
	}
	
	/* Search for Anchor Volume Descriptor */
	udf_anchor_volume_descriptor_t avd;
	rc = udf_get_anchor_volume_descriptor(service_id, &avd);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Anchor read failed");
		fs_instance_destroy(service_id);
		free(instance);
		block_fini(service_id);
		return rc;
	}
	
	log_msg(LOG_DEFAULT, LVL_DEBUG,
	    "Volume: Anchor volume descriptor found. Sector size=%" PRIu32,
	    instance->sector_size);
	log_msg(LOG_DEFAULT, LVL_DEBUG,
	    "Anchor: main sequence [length=%" PRIu32 " (bytes), start=%"
	    PRIu32 " (sector)]", avd.main_extent.length,
	    avd.main_extent.location);
	log_msg(LOG_DEFAULT, LVL_DEBUG,
	    "Anchor: reserve sequence [length=%" PRIu32 " (bytes), start=%"
	    PRIu32 " (sector)]", avd.reserve_extent.length,
	    avd.reserve_extent.location);
	
	/* Initialize the block cache */
	rc = block_cache_init(service_id, instance->sector_size, 0, cmode);
	if (rc != EOK) {
		fs_instance_destroy(service_id);
		free(instance);
		block_fini(service_id);
		return rc;
	}
	
	/* Read Volume Descriptor Sequence */
	rc = udf_read_volume_descriptor_sequence(service_id, avd.main_extent);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Volume Descriptor Sequence read failed");
		fs_instance_destroy(service_id);
		free(instance);
		block_cache_fini(service_id);
		block_fini(service_id);
		return rc;
	}
	
	fs_node_t *rfn;
	rc = udf_node_get(&rfn, service_id, instance->volumes[DEFAULT_VOL].root_dir);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_NOTE, "Can't create root node");
		fs_instance_destroy(service_id);
		free(instance);
		block_cache_fini(service_id);
		block_fini(service_id);
		return rc;
	}
	
	udf_node_t *node = UDF_NODE(rfn);
	*index = instance->volumes[DEFAULT_VOL].root_dir;
	*size = node->data_size;
	
	return EOK;
}

static errno_t udf_unmounted(service_id_t service_id)
{
	fs_node_t *fn;
	errno_t rc = udf_root_get(&fn, service_id);
	if (rc != EOK)
		return rc;
	
	udf_node_t *nodep = UDF_NODE(fn);
	udf_instance_t *instance = nodep->instance;
	
	/*
	 * We expect exactly two references on the root node.
	 * One for the udf_root_get() above and one created in
	 * udf_mounted().
	 */
	if (nodep->ref_cnt != 2) {
		udf_node_put(fn);
		return EBUSY;
	}
	
	/*
	 * Put the root node twice.
	 */
	udf_node_put(fn);
	udf_node_put(fn);
	
	fs_instance_destroy(service_id);
	free(instance);
	block_cache_fini(service_id);
	block_fini(service_id);
	
	return EOK;
}

static errno_t udf_read(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *rbytes)
{
	udf_instance_t *instance;
	errno_t rc = fs_instance_get(service_id, (void **) &instance);
	if (rc != EOK)
		return rc;
	
	fs_node_t *rfn;
	rc = udf_node_get(&rfn, service_id, index);
	if (rc != EOK)
		return rc;
	
	udf_node_t *node = UDF_NODE(rfn);
	
	ipc_callid_t callid;
	size_t len = 0;
	if (!async_data_read_receive(&callid, &len)) {
		async_answer_0(callid, EINVAL);
		udf_node_put(rfn);
		return EINVAL;
	}
	
	if (node->type == NODE_FILE) {
		if (pos >= node->data_size) {
			*rbytes = 0;
			async_data_read_finalize(callid, NULL, 0);
			udf_node_put(rfn);
			return EOK;
		}
		
		size_t read_len = 0;
		if (node->data == NULL)
			rc = udf_read_file(&read_len, callid, node, pos, len);
		else {
			/* File in allocation descriptors area */
			read_len = (len < node->data_size) ? len : node->data_size;
			async_data_read_finalize(callid, node->data + pos, read_len);
			rc = EOK;
		}
		
		*rbytes = read_len;
		(void) udf_node_put(rfn);
		return rc;
	} else {
		block_t *block = NULL;
		udf_file_identifier_descriptor_t *fid = NULL;
		if (udf_get_fid(&fid, &block, node, pos) == EOK) {
			char *name = malloc(MAX_FILE_NAME_LEN + 1);
			
			// FIXME: Check for NULL return value
			
			udf_to_unix_name(name, MAX_FILE_NAME_LEN,
			    (char *) fid->implementation_use + FLE16(fid->lenght_iu),
			    fid->lenght_file_id, &node->instance->charset);
			
			async_data_read_finalize(callid, name, str_size(name) + 1);
			*rbytes = 1;
			free(name);
			udf_node_put(rfn);
			
			if (block != NULL)
				return block_put(block);
			
			return EOK;
		} else {
			*rbytes = 0;
			udf_node_put(rfn);
			async_answer_0(callid, ENOENT);
			return ENOENT;
		}
	}
}

static errno_t udf_close(service_id_t service_id, fs_index_t index)
{
	return EOK;
}

static errno_t udf_sync(service_id_t service_id, fs_index_t index)
{
	return ENOTSUP;
}

static errno_t udf_write(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *wbytes, aoff64_t *nsize)
{
	return ENOTSUP;
}

static errno_t udf_truncate(service_id_t service_id, fs_index_t index,
    aoff64_t size)
{
	return ENOTSUP;
}

static errno_t udf_destroy(service_id_t service_id, fs_index_t index)
{
	return ENOTSUP;
}

vfs_out_ops_t udf_ops = {
	.fsprobe = udf_fsprobe,
	.mounted = udf_mounted,
	.unmounted = udf_unmounted,
	.read = udf_read,
	.write = udf_write,
	.truncate = udf_truncate,
	.close = udf_close,
	.destroy = udf_destroy,
	.sync = udf_sync
};

/**
 * @}
 */
