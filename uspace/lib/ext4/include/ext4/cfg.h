/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */

#ifndef LIBEXT4_CFG_H_
#define LIBEXT4_CFG_H_

#include "types.h"

/** Versions available to choose from when creating a new file system. */
typedef enum {
	/** Ext2 original */
	extver_ext2_old,
	/** Ext2 dynamic revision */
	extver_ext2
} ext4_cfg_ver_t;

/** Default file system version */
#define ext4_def_fs_version extver_ext2

/** Configuration of a new ext4 file system */
typedef struct {
	/** File system version */
	ext4_cfg_ver_t version;
	/** Volume name encoded as UTF-8 string */
	const char *volume_name;
	/** Filesystem block size */
	size_t bsize;
} ext4_cfg_t;

#endif

/**
 * @}
 */
