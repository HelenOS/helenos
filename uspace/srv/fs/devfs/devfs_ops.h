/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup fs
 * @{
 */

#ifndef DEVFS_DEVFS_OPS_H_
#define DEVFS_DEVFS_OPS_H_

#include <ipc/ipc.h>
#include <bool.h>

extern bool devfs_init(void);

extern void devfs_mounted(ipc_callid_t, ipc_call_t *);
extern void devfs_mount(ipc_callid_t, ipc_call_t *);
extern void devfs_lookup(ipc_callid_t, ipc_call_t *);
extern void devfs_open_node(ipc_callid_t, ipc_call_t *);
extern void devfs_stat(ipc_callid_t, ipc_call_t *);
extern void devfs_sync(ipc_callid_t, ipc_call_t *);
extern void devfs_read(ipc_callid_t, ipc_call_t *);
extern void devfs_write(ipc_callid_t, ipc_call_t *);
extern void devfs_truncate(ipc_callid_t, ipc_call_t *);
extern void devfs_close(ipc_callid_t, ipc_call_t *);
extern void devfs_destroy(ipc_callid_t, ipc_call_t *);

#endif

/**
 * @}
 */
