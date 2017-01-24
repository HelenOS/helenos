/*
 * Copyright (c) 2009 Jakub Jermar
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

/** @addtogroup libfs
 * @{
 */
/**
 * @file
 */

#ifndef LIBFS_LIBFS_H_
#define LIBFS_LIBFS_H_

#include <ipc/vfs.h>
#include <offset.h>
#include <async.h>
#include <loc.h>

typedef struct {
	errno_t (*fsprobe)(service_id_t, vfs_fs_probe_info_t *);
	errno_t (*mounted)(service_id_t, const char *, fs_index_t *, aoff64_t *);
	errno_t (*unmounted)(service_id_t);
	errno_t (*read)(service_id_t, fs_index_t, aoff64_t, size_t *);
	errno_t (*write)(service_id_t, fs_index_t, aoff64_t, size_t *,
	    aoff64_t *);
	errno_t (*truncate)(service_id_t, fs_index_t, aoff64_t);
	errno_t (*close)(service_id_t, fs_index_t);
	errno_t (*destroy)(service_id_t, fs_index_t);
	errno_t (*sync)(service_id_t, fs_index_t);
} vfs_out_ops_t;

typedef struct {
	void *data;         /**< Data of the file system implementation. */
} fs_node_t;

typedef struct {
	/*
	 * The first set of methods are functions that return an integer error
	 * code. If some additional return value is to be returned, the first
	 * argument holds the output argument.
	 */
	errno_t (*root_get)(fs_node_t **, service_id_t);
	errno_t (*match)(fs_node_t **, fs_node_t *, const char *);
	errno_t (*node_get)(fs_node_t **, service_id_t, fs_index_t);
	errno_t (*node_open)(fs_node_t *);
	errno_t (*node_put)(fs_node_t *);
	errno_t (*create)(fs_node_t **, service_id_t, int);
	errno_t (*destroy)(fs_node_t *);
	errno_t (*link)(fs_node_t *, fs_node_t *, const char *);
	errno_t (*unlink)(fs_node_t *, fs_node_t *, const char *);
	errno_t (*has_children)(bool *, fs_node_t *);
	/*
	 * The second set of methods are usually mere getters that do not
	 * return an integer error code.
	 */
	fs_index_t (*index_get)(fs_node_t *);
	aoff64_t (*size_get)(fs_node_t *);
	unsigned int (*lnkcnt_get)(fs_node_t *);
	bool (*is_directory)(fs_node_t *);
	bool (*is_file)(fs_node_t *);
	service_id_t (*service_get)(fs_node_t *);
	errno_t (*size_block)(service_id_t, uint32_t *);
	errno_t (*total_block_count)(service_id_t, uint64_t *);
	errno_t (*free_block_count)(service_id_t, uint64_t *);
} libfs_ops_t;

typedef struct {
	int fs_handle;           /**< File system handle. */
	uint8_t *plb_ro;         /**< Read-only PLB view. */
} fs_reg_t;

extern errno_t fs_register(async_sess_t *, vfs_info_t *, vfs_out_ops_t *,
    libfs_ops_t *);

extern void fs_node_initialize(fs_node_t *);

extern errno_t fs_instance_create(service_id_t, void *);
extern errno_t fs_instance_get(service_id_t, void **);
extern errno_t fs_instance_destroy(service_id_t);

#endif

/** @}
 */
