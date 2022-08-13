/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup libhound
 * @addtogroup audio
 * @{
 */
/** @file
 * @brief Audio PCM buffer interface.
 */

#ifndef LIBHOUND_SERVER_H_
#define LIBHOUND_SERVER_H_

#include <async.h>
#include <loc.h>

typedef void (*dev_change_callback_t)(void *);
typedef errno_t (*device_callback_t)(service_id_t, const char *);

errno_t hound_server_register(const char *name, service_id_t *id);
void hound_server_unregister(service_id_t id);
errno_t hound_server_set_device_change_callback(dev_change_callback_t cb,
    void *);
errno_t hound_server_devices_iterate(device_callback_t callback);

#endif
