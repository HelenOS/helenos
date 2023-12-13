/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup libipctest
 * @{
 */
/** @file
 */

#ifndef _LIBIPCTEST_H_
#define _LIBIPCTEST_H_

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
extern errno_t ipc_test_set_rw_buf_size(ipc_test_t *, size_t);
extern errno_t ipc_test_read(ipc_test_t *, void *, size_t);
extern errno_t ipc_test_write(ipc_test_t *, const void *, size_t);

#endif

/** @}
 */
