/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */

#ifndef LIBEXT4_IALLOC_H_
#define LIBEXT4_IALLOC_H_

#include "ext4/types.h"

extern errno_t ext4_ialloc_free_inode(ext4_filesystem_t *, uint32_t, bool);
extern errno_t ext4_ialloc_alloc_inode(ext4_filesystem_t *, uint32_t *, bool);
extern errno_t ext4_ialloc_alloc_this_inode(ext4_filesystem_t *, uint32_t,
    bool);

#endif

/**
 * @}
 */
