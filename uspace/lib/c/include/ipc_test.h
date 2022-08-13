/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_TEST_H_
#define _LIBC_IPC_TEST_H_

#include <async.h>
#include <errno.h>

typedef struct {
	async_sess_t *sess;
} ipc_test_t;

extern errno_t ipc_test_create(ipc_test_t **);
extern void ipc_test_destroy(ipc_test_t *);
extern errno_t ipc_test_ping(ipc_test_t *);
extern errno_t ipc_test_get_ro_area_size(ipc_test_t *, size_t *);
extern errno_t ipc_test_get_rw_area_size(ipc_test_t *, size_t *);
extern errno_t ipc_test_share_in_ro(ipc_test_t *, size_t, const void **);
extern errno_t ipc_test_share_in_rw(ipc_test_t *, size_t, void **);

#endif

/** @}
 */
