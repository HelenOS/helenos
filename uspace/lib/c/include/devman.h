/*
 * Copyright (c) 2009 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_DEVMAN_H_
#define LIBC_DEVMAN_H_

#include <ipc/devman.h>
#include <ipc/loc.h>
#include <async.h>
#include <stdbool.h>

extern async_exch_t *devman_exchange_begin_blocking(iface_t);
extern async_exch_t *devman_exchange_begin(iface_t);
extern void devman_exchange_end(async_exch_t *);

extern errno_t devman_driver_register(const char *);
extern errno_t devman_add_function(const char *, fun_type_t, match_id_list_t *,
    devman_handle_t, devman_handle_t *);
extern errno_t devman_remove_function(devman_handle_t);
extern errno_t devman_drv_fun_online(devman_handle_t);
extern errno_t devman_drv_fun_offline(devman_handle_t);

extern async_sess_t *devman_device_connect(devman_handle_t, unsigned int);
extern async_sess_t *devman_parent_device_connect(devman_handle_t,
    unsigned int);

extern errno_t devman_fun_get_handle(const char *, devman_handle_t *,
    unsigned int);
extern errno_t devman_fun_get_child(devman_handle_t, devman_handle_t *);
extern errno_t devman_dev_get_parent(devman_handle_t, devman_handle_t *);
extern errno_t devman_dev_get_functions(devman_handle_t, devman_handle_t **,
    size_t *);
extern errno_t devman_fun_get_match_id(devman_handle_t, size_t, char *, size_t,
    unsigned int *);
extern errno_t devman_fun_get_name(devman_handle_t, char *, size_t);
extern errno_t devman_fun_get_driver_name(devman_handle_t, char *, size_t);
extern errno_t devman_fun_get_path(devman_handle_t, char *, size_t);
extern errno_t devman_fun_online(devman_handle_t);
extern errno_t devman_fun_offline(devman_handle_t);

extern errno_t devman_add_device_to_category(devman_handle_t, const char *);
extern errno_t devman_fun_sid_to_handle(service_id_t, devman_handle_t *);
extern errno_t devman_get_drivers(devman_handle_t **, size_t *);
extern errno_t devman_driver_get_devices(devman_handle_t, devman_handle_t **,
    size_t *);
extern errno_t devman_driver_get_handle(const char *, devman_handle_t *);
extern errno_t devman_driver_get_match_id(devman_handle_t, size_t, char *, size_t,
    unsigned int *);
extern errno_t devman_driver_get_name(devman_handle_t, char *, size_t);
extern errno_t devman_driver_get_state(devman_handle_t, driver_state_t *);
extern errno_t devman_driver_load(devman_handle_t);
extern errno_t devman_driver_unload(devman_handle_t);

#endif

/** @}
 */
