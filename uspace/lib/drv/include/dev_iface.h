/*
 * Copyright (c) 2010 Lenka Trochtova
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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#ifndef LIBDRV_DEV_IFACE_H_
#define LIBDRV_DEV_IFACE_H_

#include <ipc/common.h>
#include <ipc/dev_iface.h>

/*
 * Device interface
 */

struct ddf_fun;

/*
 * First two parameters: device and interface structure registered by the
 * devices driver.
 */
typedef void remote_iface_func_t(struct ddf_fun *, void *, cap_call_handle_t,
    ipc_call_t *);
typedef remote_iface_func_t *remote_iface_func_ptr_t;
typedef void remote_handler_t(struct ddf_fun *, cap_call_handle_t, ipc_call_t *);

typedef struct {
	const size_t method_count;
	const remote_iface_func_ptr_t *methods;
} remote_iface_t;

typedef struct {
	const remote_iface_t *ifaces[DEV_IFACE_COUNT];
} iface_dipatch_table_t;

extern const remote_iface_t *get_remote_iface(int);
extern remote_iface_func_ptr_t get_remote_method(const remote_iface_t *, sysarg_t);


extern bool is_valid_iface_idx(int);

#endif

/**
 * @}
 */
