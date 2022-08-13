/*
 * SPDX-FileCopyrightText: 2011 Maurizio Lombardi
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_VFS_MTAB_H_
#define _LIBC_VFS_MTAB_H_

#include <ipc/vfs.h>
#include <adt/list.h>

typedef struct mtab_ent {
	link_t link;
	char mp[MAX_PATH_LEN];
	char fs_name[FS_NAME_MAXLEN];
	service_id_t service_id;
} mtab_ent_t;

extern errno_t vfs_get_mtab_list(list_t *);

#endif

/** @}
 */
