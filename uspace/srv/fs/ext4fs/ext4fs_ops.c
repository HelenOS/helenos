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
#include <libfs.h>
#include <ipc/loc.h>
#include "ext4fs.h"
#include "../../vfs/vfs.h"


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

/**
 *	TODO comment
 */
int ext4fs_global_init(void)
{
	// TODO
	return EOK;
}

int ext4fs_global_fini(void)
{
	// TODO
	return EOK;
}


/*
 * EXT4 libfs operations.
 */

int ext4fs_root_get(fs_node_t **rfn, service_id_t service_id)
{
	// TODO
	return 0;
}

int ext4fs_match(fs_node_t **rfn, fs_node_t *pfn, const char *component)
{
	// TODO
	return EOK;
}

int ext4fs_node_get(fs_node_t **rfn, service_id_t service_id, fs_index_t index)
{
	// TODO
	return 0;
}

int ext4fs_node_open(fs_node_t *fn)
{
	// TODO
	return EOK;
}

int ext4fs_node_put(fs_node_t *fn)
{
	// TODO
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

static int ext4fs_mounted(service_id_t service_id, const char *opts,
   fs_index_t *index, aoff64_t *size, unsigned *lnkcnt)
{
	// TODO
	return EOK;
}

static int ext4fs_unmounted(service_id_t service_id)
{
	// TODO
	return EOK;
}

static int
ext4fs_read(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *rbytes)
{
	// TODO
	return 0;
}

static int
ext4fs_write(service_id_t service_id, fs_index_t index, aoff64_t pos,
    size_t *wbytes, aoff64_t *nsize)
{
	// TODO
	return ENOTSUP;
}

static int
ext4fs_truncate(service_id_t service_id, fs_index_t index, aoff64_t size)
{
	// TODO
	return ENOTSUP;
}

static int ext4fs_close(service_id_t service_id, fs_index_t index)
{
	// TODO
	return EOK;
}

static int ext4fs_destroy(service_id_t service_id, fs_index_t index)
{
	//TODO
	return ENOTSUP;
}

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
