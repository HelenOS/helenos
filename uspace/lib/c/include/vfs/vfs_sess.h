/*
 * SPDX-FileCopyrightText: 2007 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_VFS_SESS_H_
#define _LIBC_VFS_SESS_H_

#include <async.h>
#include <stdio.h>

extern async_sess_t *vfs_fd_session(int, iface_t);
extern async_sess_t *vfs_fsession(FILE *, iface_t);

#endif

/** @}
 */
