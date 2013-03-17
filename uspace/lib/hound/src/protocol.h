/*
 * Copyright (c) 2013 Jan Vesely
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

#ifndef LIBHOUND_PROTOCOL_H_
#define LIBHOUND_PROTOCOL_H_

#include <async.h>
#include <pcm/format.h>

const char *HOUND_SERVICE;

typedef async_sess_t hound_sess_t;
typedef int hound_context_id_t;

hound_sess_t *hound_service_connect(const char *service);
void hound_service_disconnect(hound_sess_t *sess);

hound_context_id_t hound_service_register_context(hound_sess_t *sess,
    const char *name);
int hound_service_unregister_context(hound_sess_t *sess, hound_context_id_t id);

int hound_service_stream_enter(async_exch_t *exch, hound_context_id_t id,
    int flags, pcm_format_t format, size_t bsize);
int hound_service_stream_drain(async_exch_t *exch);
int hound_service_stream_exit(async_exch_t *exch);

int hound_service_stream_write(async_exch_t *exch, const void *data, size_t size);
int hound_service_stream_read(async_exch_t *exch, void *data, size_t size);


#endif
/** @}
 */
