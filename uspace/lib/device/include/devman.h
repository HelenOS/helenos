/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_DEVMAN_H
#define LIBDEVICE_DEVMAN_H

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
