/*
 * Copyright (c) 2012 Jan Vesely
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
#include <bool.h>
#include <loc.h>
#include <pcm_sample_format.h>

enum {
        HOUND_REGISTER_PLAYBACK = IPC_FIRST_USER_METHOD,
        HOUND_REGISTER_RECORDING,
        HOUND_UNREGISTER_PLAYBACK,
        HOUND_UNREGISTER_RECORDING,
        HOUND_CONNECT,
        HOUND_DISCONNECT,
};

typedef void (*dev_change_callback_t)(void);
typedef int (*device_callback_t)(service_id_t, const char *);


int hound_server_register(const char *name, service_id_t *id);
void hound_server_unregister(service_id_t id);
int hound_server_set_device_change_callback(dev_change_callback_t cb);
int hound_server_devices_iterate(device_callback_t callback);
int hound_server_get_register_params(const char **name, async_sess_t **sess,
    unsigned *channels, unsigned *rate, pcm_sample_format_t *format);
int hound_server_get_unregister_params(const char **name);
int hound_server_get_connection_params(const char **source, const char **sink);

#endif
