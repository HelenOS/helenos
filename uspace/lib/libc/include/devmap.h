/*
 * Copyright (c) 2009 Jiri Svoboda
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

#ifndef LIBC_DEVMAP_H_
#define LIBC_DEVMAP_H_

#include <ipc/devmap.h>
#include <async.h>
#include <bool.h>

extern int devmap_get_phone(devmap_interface_t, unsigned int);
extern void devmap_hangup_phone(devmap_interface_t iface);

extern int devmap_driver_register(const char *, async_client_conn_t);
extern int devmap_device_register(const char *, dev_handle_t *);

extern int devmap_device_get_handle(const char *, dev_handle_t *, unsigned int);
extern int devmap_namespace_get_handle(const char *, dev_handle_t *, unsigned int);
extern devmap_handle_type_t devmap_handle_probe(dev_handle_t);

extern int devmap_device_connect(dev_handle_t, unsigned int);

extern int devmap_null_create(void);
extern void devmap_null_destroy(int);

extern size_t devmap_count_namespaces(void);
extern size_t devmap_count_devices(dev_handle_t);

extern size_t devmap_get_namespaces(dev_desc_t **);
extern size_t devmap_get_devices(dev_handle_t, dev_desc_t **);

#endif

/** @}
 */
