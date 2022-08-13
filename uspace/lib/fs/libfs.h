/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
