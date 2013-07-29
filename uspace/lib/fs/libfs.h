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
#include <stdint.h>
#include <async.h>
#include <loc.h>

typedef struct {
	int (* mounted)(service_id_t, const char *, fs_index_t *, aoff64_t *,
	    unsigned *);
	int (* unmounted)(service_id_t);
	int (* read)(service_id_t, fs_index_t, aoff64_t, size_t *);
	int (* write)(service_id_t, fs_index_t, aoff64_t, size_t *,
	    aoff64_t *);
	int (* truncate)(service_id_t, fs_index_t, aoff64_t);
	int (* close)(service_id_t, fs_index_t);
	int (* destroy)(service_id_t, fs_index_t);
	int (* sync)(service_id_t, fs_index_t);
} vfs_out_ops_t;

typedef struct {
	bool mp_active;
	async_sess_t *sess;
	fs_handle_t fs_handle;
	service_id_t service_id;
} mp_data_t;

typedef struct {
	mp_data_t mp_data;  /**< Mount point info. */
	void *data;         /**< Data of the file system implementation. */
} fs_node_t;

typedef struct {
	/*
	 * The first set of methods are functions that return an integer error
	 * code. If some additional return value is to be returned, the first
	 * argument holds the output argument.
	 */
	int (* root_get)(fs_node_t **, service_id_t);
	int (* match)(fs_node_t **, fs_node_t *, const char *);
	int (* node_get)(fs_node_t **, service_id_t, fs_index_t);
	int (* node_open)(fs_node_t *);
	int (* node_put)(fs_node_t *);
	int (* create)(fs_node_t **, service_id_t, int);
	int (* destroy)(fs_node_t *);
	int (* link)(fs_node_t *, fs_node_t *, const char *);
	int (* unlink)(fs_node_t *, fs_node_t *, const char *);
	int (* has_children)(bool *, fs_node_t *);
	/*
	 * The second set of methods are usually mere getters that do not
	 * return an integer error code.
	 */
	fs_index_t (* index_get)(fs_node_t *);
	aoff64_t (* size_get)(fs_node_t *);
	unsigned int (* lnkcnt_get)(fs_node_t *);
	bool (* is_directory)(fs_node_t *);
	bool (* is_file)(fs_node_t *);
	service_id_t (* service_get)(fs_node_t *);
	int (* size_block)(service_id_t, uint32_t *);
	int (* total_block_count)(service_id_t, uint64_t *);
	int (* free_block_count)(service_id_t, uint64_t *);
} libfs_ops_t;

typedef struct {
	int fs_handle;           /**< File system handle. */
	uint8_t *plb_ro;         /**< Read-only PLB view. */
} fs_reg_t;

extern int fs_register(async_sess_t *, vfs_info_t *, vfs_out_ops_t *,
    libfs_ops_t *);

extern void fs_node_initialize(fs_node_t *);

extern int fs_instance_create(service_id_t, void *);
extern int fs_instance_get(service_id_t, void **);
extern int fs_instance_destroy(service_id_t);

#endif

/** @}
 */
