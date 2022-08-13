/*
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */

#ifndef LIBEXT4_OPS_H_
#define LIBEXT4_OPS_H_

#include <libfs.h>
#include "ext4/fstypes.h"

extern vfs_out_ops_t ext4_ops;
extern libfs_ops_t ext4_libfs_ops;

extern errno_t ext4_global_init(void);
extern errno_t ext4_global_fini(void);

extern errno_t ext4_node_get_core(fs_node_t **, ext4_instance_t *, fs_index_t);
extern errno_t ext4_node_put(fs_node_t *);

#endif

/**
 * @}
 */
