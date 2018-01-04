/*
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_IO_CHARDEV_SRV_H_
#define LIBC_IO_CHARDEV_SRV_H_

#include <adt/list.h>
#include <async.h>
#include <fibril_synch.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct chardev_ops chardev_ops_t;

/** Service setup (per sevice) */
typedef struct {
	chardev_ops_t *ops;
	void *sarg;
} chardev_srvs_t;

/** Server structure (per client session) */
typedef struct {
	chardev_srvs_t *srvs;
	void *carg;
} chardev_srv_t;

struct chardev_ops {
	errno_t (*open)(chardev_srvs_t *, chardev_srv_t *);
	errno_t (*close)(chardev_srv_t *);
	errno_t (*read)(chardev_srv_t *, void *, size_t, size_t *);
	errno_t (*write)(chardev_srv_t *, const void *, size_t, size_t *);
	void (*def_handler)(chardev_srv_t *, ipc_callid_t, ipc_call_t *);
};

extern void chardev_srvs_init(chardev_srvs_t *);

extern errno_t chardev_conn(ipc_callid_t, ipc_call_t *, chardev_srvs_t *);

#endif

/** @}
 */
