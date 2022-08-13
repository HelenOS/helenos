/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_TYPES_VOL_H
#define LIBDEVICE_TYPES_VOL_H

#include <async.h>
#include <ipc/vfs.h>
#include <ipc/vol.h>
#include <stdbool.h>

typedef struct {
	sysarg_t id;
} volume_id_t;

typedef enum {
	/** Partition is empty */
	vpc_empty,
	/** Partition contains a recognized filesystem */
	vpc_fs,
	/** Partition contains unrecognized data */
	vpc_unknown
} vol_part_cnt_t;

/** File system type */
typedef enum {
	fs_exfat,
	fs_fat,
	fs_minix,
	fs_ext4,
	fs_cdfs
} vol_fstype_t;

#define VOL_FSTYPE_LIMIT (fs_ext4 + 1)
#define VOL_FSTYPE_DEFAULT fs_ext4

/** Volume service */
typedef struct vol {
	/** Volume service session */
	async_sess_t *sess;
} vol_t;

/** Partition information */
typedef struct {
	/** Partition content type */
	vol_part_cnt_t pcnt;
	/** Filesystem type */
	vol_fstype_t fstype;
	/** Volume label */
	char label[VOL_LABEL_MAXLEN + 1];
	/** Current mount point */
	char cur_mp[MAX_PATH_LEN + 1]; /* XXX too big */
	/** Current mount point is automatic */
	bool cur_mp_auto;
} vol_part_info_t;

/** Volume configuration information */
typedef struct {
	/** Volume identifier */
	volume_id_t id;
	/** Volume label */
	char label[VOL_LABEL_MAXLEN + 1];
	/** Mount path */
	char path[MAX_PATH_LEN + 1]; /* XXX too big */
} vol_info_t;

/** Volume label support */
typedef struct {
	/** Volume labels are supported */
	bool supported;
} vol_label_supp_t;

#endif

/** @}
 */
