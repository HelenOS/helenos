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

enum {
	HOUND_SINK_APPS = 0x1,
	HOUND_SINK_DEVS = 0x2,
	HOUND_SOURCE_APPS = 0x4,
	HOUND_SOURCE_DEVS = 0x8,
	HOUND_CONNECTED = 0x10,

	HOUND_STREAM_DRAIN_ON_EXIT = 0x1,
	HOUND_STREAM_IGNORE_UNDERFLOW = 0x2,
	HOUND_STREAM_IGNORE_OVERFLOW = 0x4,
};

typedef async_sess_t hound_sess_t;
typedef int hound_context_id_t;

static inline int hound_context_id_err(hound_context_id_t id)
{
	return id > 0 ? EOK : (id == 0 ? ENOENT : id);
}

hound_sess_t *hound_service_connect(const char *service);
void hound_service_disconnect(hound_sess_t *sess);

hound_context_id_t hound_service_register_context(hound_sess_t *sess,
    const char *name, bool record);
int hound_service_unregister_context(hound_sess_t *sess, hound_context_id_t id);

int hound_service_get_list(hound_sess_t *sess, const char ***ids, size_t *count,
    int flags, const char *connection);
static inline int hound_service_get_list_all(hound_sess_t *sess,
    const char ***ids, size_t *count, int flags)
{
	return hound_service_get_list(sess, ids, count, flags, NULL);
}

int hound_service_connect_source_sink(hound_sess_t *sess, const char *source,
    const char *sink);
int hound_service_disconnect_source_sink(hound_sess_t *sess, const char *source,
    const char *sink);

int hound_service_stream_enter(async_exch_t *exch, hound_context_id_t id,
    int flags, pcm_format_t format, size_t bsize);
int hound_service_stream_drain(async_exch_t *exch);
int hound_service_stream_exit(async_exch_t *exch);

int hound_service_stream_write(async_exch_t *exch, const void *data, size_t size);
int hound_service_stream_read(async_exch_t *exch, void *data, size_t size);

/* Server */
typedef struct hound_server_iface {
	int (*add_context)(void *, hound_context_id_t *, const char *, bool);
	int (*rem_context)(void *, hound_context_id_t);
	bool (*is_record_context)(void *, hound_context_id_t);
	int (*connect)(void *, const char *, const char *);
	int (*disconnect)(void *, const char *, const char *);
	int (*add_stream)(void *, hound_context_id_t, int, pcm_format_t, size_t,
	    void **);
	int (*rem_stream)(void *, void *);
	int (*stream_data_write)(void *, const void *, size_t);
	int (*stream_data_read)(void *, void *, size_t);
	void *server;
} hound_server_iface_t;

void hound_service_set_server_iface(hound_server_iface_t *iface);

void hound_connection_handler(ipc_callid_t iid, ipc_call_t *icall, void *arg);

#endif
/** @}
 */
