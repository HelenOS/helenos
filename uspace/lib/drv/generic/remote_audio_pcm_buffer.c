/*
 * Copyright (c) 2011 Jan Vesely
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

#include <async.h>
#include <ddf/log.h>
#include <errno.h>
#include <str.h>
#include <as.h>
#include <sys/mman.h>

#include "audio_pcm_buffer_iface.h"
#include "ddf/driver.h"

typedef enum {
	IPC_M_AUDIO_PCM_GET_INFO_STR,
	IPC_M_AUDIO_PCM_GET_BUFFER,
	IPC_M_AUDIO_PCM_RELEASE_BUFFER,
	IPC_M_AUDIO_PCM_START_PLAYBACK,
	IPC_M_AUDIO_PCM_STOP_PLAYBACK,
	IPC_M_AUDIO_PCM_START_RECORD,
	IPC_M_AUDIO_PCM_STOP_RECORD,
} audio_pcm_iface_funcs_t;

/*
 * CLIENT SIDE
 */
int audio_pcm_buffer_get_info_str(async_exch_t *exch, const char **name)
{
	if (!exch)
		return EINVAL;
	sysarg_t name_size;
	const int ret = async_req_1_1(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_GET_INFO_STR, &name_size);
	if (ret == EOK && name) {
		char *name_place = calloc(1, name_size);
		if (!name_place) {
			/* Make the other side fail
			 * as it waits for read request */
			async_data_read_start(exch, (void*)-1, 0);
			return ENOMEM;
		}
		const int ret =
		    async_data_read_start(exch, name_place, name_size);
		if (ret != EOK) {
			free(name_place);
			return ret;
		}
		*name = name_place;
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
int audio_pcm_buffer_get_buffer(async_exch_t *exch, void **buffer, size_t *size,
    unsigned *id, async_client_conn_t event_rec, void* arg)
{
	if (!exch || !buffer || !size || !id)
		return EINVAL;

	sysarg_t buffer_size = *size, buffer_id = 0;
	const int ret = async_req_2_2(exch,
	    DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE), IPC_M_AUDIO_PCM_GET_BUFFER,
	    (sysarg_t)buffer_size, &buffer_size, &buffer_id);
	if (ret == EOK) {
		void *dst = NULL;
		// FIXME Do we need to know the flags?
		int ret = async_share_in_start_0_0(exch, buffer_size, &dst);
		if (ret != EOK) {
			return ret;
		}
		ret = async_connect_to_me(exch, 0, 0, 0, event_rec, arg);
		if (ret != EOK) {
			return ret;
		}

		*buffer = dst;
		*size = buffer_size;
		*id = buffer_id;
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
int audio_pcm_buffer_release_buffer(async_exch_t *exch, unsigned id)
{
	if (!exch)
		return EINVAL;
	return async_req_2_0(exch, DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_RELEASE_BUFFER, id);
}
/*----------------------------------------------------------------------------*/
int audio_pcm_buffer_start_playback(async_exch_t *exch, unsigned id,
    unsigned parts, unsigned sample_rate, uint16_t sample_size,
    uint8_t channels, bool sign)
{
	if (!exch)
		return EINVAL;
	const sysarg_t packed =
	    (sample_size << 16) | (channels << 8) |
	    (parts & 0x7f << 1) | (sign ? 1 : 0);
	return async_req_4_0(exch, DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_START_PLAYBACK, id, sample_rate, packed);
}
/*----------------------------------------------------------------------------*/
int audio_pcm_buffer_stop_playback(async_exch_t *exch, unsigned id)
{
	if (!exch)
		return EINVAL;
	return async_req_2_0(exch, DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_STOP_PLAYBACK, id);
}
/*----------------------------------------------------------------------------*/
int audio_pcm_buffer_start_record(async_exch_t *exch, unsigned id,
    unsigned sample_rate, unsigned sample_size, unsigned channels, bool sign)
{
	if (!exch || sample_size > UINT16_MAX || channels > (UINT16_MAX >> 1))
		return EINVAL;
	sysarg_t packed = sample_size << 16 | channels << 1 | sign ? 1 : 0;
	return async_req_4_0(exch, DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_START_RECORD, id, sample_rate, packed);
}
/*----------------------------------------------------------------------------*/
int audio_pcm_buffer_stop_record(async_exch_t *exch, unsigned id)
{
	if (!exch)
		return EINVAL;
	return async_req_2_0(exch, DEV_IFACE_ID(AUDIO_PCM_BUFFER_IFACE),
	    IPC_M_AUDIO_PCM_STOP_RECORD, id);
}

/*
 * SERVER SIDE
 */
static void remote_audio_pcm_get_info_str(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_get_buffer(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_release_buffer(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_start_playback(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_stop_playback(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_start_record(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_pcm_stop_record(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote audio pcm buffer interface operations. */
static remote_iface_func_ptr_t remote_audio_pcm_iface_ops[] = {
	[IPC_M_AUDIO_PCM_GET_INFO_STR] = remote_audio_pcm_get_info_str,
	[IPC_M_AUDIO_PCM_GET_BUFFER] = remote_audio_pcm_get_buffer,
	[IPC_M_AUDIO_PCM_RELEASE_BUFFER] = remote_audio_pcm_release_buffer,
	[IPC_M_AUDIO_PCM_START_PLAYBACK] = remote_audio_pcm_start_playback,
	[IPC_M_AUDIO_PCM_STOP_PLAYBACK] = remote_audio_pcm_stop_playback,
	[IPC_M_AUDIO_PCM_START_RECORD] = remote_audio_pcm_start_record,
	[IPC_M_AUDIO_PCM_STOP_RECORD] = remote_audio_pcm_stop_record,
};

/** Remote audio mixer interface structure. */
remote_iface_t remote_audio_pcm_buffer_iface = {
	.method_count = sizeof(remote_audio_pcm_iface_ops) /
	    sizeof(remote_audio_pcm_iface_ops[0]),
	.methods = remote_audio_pcm_iface_ops
};
/*----------------------------------------------------------------------------*/
void remote_audio_pcm_get_info_str(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_buffer_iface_t *pcm_iface = iface;

	if (!pcm_iface->get_info_str) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	const char *name = NULL;
	const int ret = pcm_iface->get_info_str(fun, &name);
	const size_t name_size = name ? str_size(name) + 1 : 0;
	async_answer_1(callid, ret, name_size);
	/* Send the string. */
	if (ret == EOK && name_size > 0) {
		size_t size;
		ipc_callid_t name_id;
		if (!async_data_read_receive(&name_id, &size)) {
			async_answer_0(name_id, EPARTY);
			return;
		}
		if (size != name_size) {
			async_answer_0(name_id, ELIMIT);
			return;
		}
		async_data_read_finalize(name_id, name, name_size);
	}
}
/*----------------------------------------------------------------------------*/
void remote_audio_pcm_get_buffer(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_buffer_iface_t *pcm_iface = iface;

	if (!pcm_iface->get_buffer ||
	    !pcm_iface->release_buffer ||
	    !pcm_iface->set_event_session) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	void *buffer = NULL;
	size_t size = DEV_IPC_GET_ARG1(*call);
	unsigned id = 0;
	int ret = pcm_iface->get_buffer(fun, &buffer, &size, &id);
	async_answer_2(callid, ret, size, id);
	if (ret != EOK || size == 0)
		return;

	/* Share the buffer. */
	size_t share_size = 0;
	ipc_callid_t share_id = 0;
	ddf_msg(LVL_DEBUG2, "Calling share receive.");
	if (!async_share_in_receive(&share_id, &share_size)) {
		ddf_msg(LVL_DEBUG, "Failed to share pcm buffer.");
		if (pcm_iface->release_buffer)
			pcm_iface->release_buffer(fun, id);
		async_answer_0(share_id, EPARTY);
		return;
	}
	ddf_msg(LVL_DEBUG2, "Checking requested share size");
	if (share_size != size) {
		ddf_msg(LVL_DEBUG, "Incorrect pcm buffer size requested.");
		if (pcm_iface->release_buffer)
			pcm_iface->release_buffer(fun, id);
		async_answer_0(share_id, ELIMIT);
		return;
	}
	ddf_msg(LVL_DEBUG2, "Calling share finalize");
	ret = async_share_in_finalize(share_id, buffer, 0);
	if (ret != EOK) {
		ddf_msg(LVL_DEBUG, "Failed to share buffer");
		if (pcm_iface->release_buffer)
			pcm_iface->release_buffer(fun, id);
		return;

	}
	ddf_msg(LVL_DEBUG2, "Buffer shared with size %zu, creating callback.",
	    share_size);
	{
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		async_sess_t *sess =
		    async_callback_receive_start(EXCHANGE_ATOMIC, &call);
		if (sess == NULL) {
			ddf_msg(LVL_DEBUG, "Failed to create event callback");
			pcm_iface->release_buffer(fun, id);
			async_answer_0(callid, EAGAIN);
			return;
		}
		ret = pcm_iface->set_event_session(fun, id, sess);
		if (ret != EOK) {
			ddf_msg(LVL_DEBUG, "Failed to set event callback.");
			pcm_iface->release_buffer(fun, id);
		}
		async_answer_0(callid, ret);
	}
}
/*----------------------------------------------------------------------------*/
void remote_audio_pcm_release_buffer(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_buffer_iface_t *pcm_iface = iface;

	const unsigned id = DEV_IPC_GET_ARG1(*call);
	const int ret = pcm_iface->release_buffer ?
	    pcm_iface->release_buffer(fun, id) : ENOTSUP;
	async_answer_0(callid, ret);
}
/*----------------------------------------------------------------------------*/
void remote_audio_pcm_start_playback(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_buffer_iface_t *pcm_iface = iface;

	const unsigned id = DEV_IPC_GET_ARG1(*call);
	const unsigned rate = DEV_IPC_GET_ARG2(*call);
	const unsigned size = DEV_IPC_GET_ARG3(*call) >> 16;
	const unsigned channels = (DEV_IPC_GET_ARG3(*call) >> 8) & UINT8_MAX;
	const unsigned parts = (DEV_IPC_GET_ARG3(*call) >> 1) & 0x7f;
	const bool sign = (bool)(DEV_IPC_GET_ARG3(*call) & 1);

	const int ret = pcm_iface->start_playback
	    ? pcm_iface->start_playback(fun, id, parts, rate, size, channels, sign)
	    : ENOTSUP;
	async_answer_0(callid, ret);
}
/*----------------------------------------------------------------------------*/
void remote_audio_pcm_stop_playback(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_buffer_iface_t *pcm_iface = iface;

	const unsigned id = DEV_IPC_GET_ARG1(*call);
	const int ret = pcm_iface->stop_playback ?
	    pcm_iface->stop_playback(fun, id) : ENOTSUP;
	async_answer_0(callid, ret);
}
/*----------------------------------------------------------------------------*/
void remote_audio_pcm_start_record(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_buffer_iface_t *pcm_iface = iface;

	const unsigned id = DEV_IPC_GET_ARG1(*call);
	const unsigned rate = DEV_IPC_GET_ARG2(*call);
	const unsigned size = DEV_IPC_GET_ARG3(*call) >> 16;
	const unsigned channels = (DEV_IPC_GET_ARG3(*call) & UINT16_MAX) >> 1;
	const bool sign = (bool)(DEV_IPC_GET_ARG3(*call) & 1);

	const int ret = pcm_iface->start_record
	    ? pcm_iface->start_record(fun, id, rate, size, channels, sign)
	    : ENOTSUP;
	async_answer_0(callid, ret);
}
/*----------------------------------------------------------------------------*/
void remote_audio_pcm_stop_record(ddf_fun_t *fun, void *iface,
    ipc_callid_t callid, ipc_call_t *call)
{
	const audio_pcm_buffer_iface_t *pcm_iface = iface;

	const unsigned id = DEV_IPC_GET_ARG1(*call);
	const int ret = pcm_iface->stop_record ?
	    pcm_iface->stop_record(fun, id) : ENOTSUP;
	async_answer_0(callid, ret);
}

#if 0
/*----------------------------------------------------------------------------*/
void remote_audio_mixer_get_info(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	audio_mixer_iface_t *mixer_iface = iface;

	if (!mixer_iface->get_info) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	const char *name = NULL;
	unsigned items = 0;
	const int ret = mixer_iface->get_info(fun, &name, &items);
	const size_t name_size = name ? str_size(name) + 1 : 0;
	async_answer_2(callid, ret, name_size, items);
	/* Send the name. */
	if (ret == EOK && name_size > 0) {
		size_t size;
		ipc_callid_t name_id;
		if (!async_data_read_receive(&name_id, &size)) {
			async_answer_0(name_id, EPARTY);
			return;
		}
		if (size != name_size) {
			async_answer_0(name_id, ELIMIT);
			return;
		}
		async_data_read_finalize(name_id, name, name_size);
	}
}
/*----------------------------------------------------------------------------*/
void remote_audio_mixer_get_item_info(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	audio_mixer_iface_t *mixer_iface = iface;

	if (!mixer_iface->get_item_info) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	const unsigned item = DEV_IPC_GET_ARG1(*call);
	const char *name = NULL;
	unsigned channels = 0;
	const int ret = mixer_iface->get_item_info(fun, item, &name, &channels);
	const size_t name_size = name ? str_size(name) + 1 : 0;
	async_answer_2(callid, ret, name_size, channels);
	/* Send the name. */
	if (ret == EOK && name_size > 0) {
		size_t size;
		ipc_callid_t name_id;
		if (!async_data_read_receive(&name_id, &size)) {
			async_answer_0(name_id, EPARTY);
			return;
		}
		if (size != name_size) {
			async_answer_0(name_id, ELIMIT);
			return;
		}
		async_data_read_finalize(name_id, name, name_size);
	}
}
/*----------------------------------------------------------------------------*/
void remote_audio_mixer_get_channel_info(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	audio_mixer_iface_t *mixer_iface = iface;

	if (!mixer_iface->get_channel_info) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	const unsigned item = DEV_IPC_GET_ARG1(*call);
	const unsigned channel = DEV_IPC_GET_ARG2(*call);
	const char *name = NULL;
	unsigned levels = 0;
	const int ret =
	    mixer_iface->get_channel_info(fun, item, channel, &name, &levels);
	const size_t name_size = name ? str_size(name) + 1 : 0;
	async_answer_2(callid, ret, name_size, levels);
	/* Send the name. */
	if (ret == EOK && name_size > 0) {
		size_t size;
		ipc_callid_t name_id;
		if (!async_data_read_receive(&name_id, &size)) {
			async_answer_0(name_id, EPARTY);
			return;
		}
		if (size != name_size) {
			async_answer_0(name_id, ELIMIT);
			return;
		}
		async_data_read_finalize(name_id, name, name_size);
	}
}
/*----------------------------------------------------------------------------*/
void remote_audio_mixer_channel_mute_set(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	audio_mixer_iface_t *mixer_iface = iface;

	if (!mixer_iface->channel_mute_set) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	const unsigned item = DEV_IPC_GET_ARG1(*call);
	const unsigned channel = DEV_IPC_GET_ARG2(*call);
	const bool mute = DEV_IPC_GET_ARG3(*call);
	const int ret = mixer_iface->channel_mute_set(fun, item, channel, mute);
	async_answer_0(callid, ret);
}
/*----------------------------------------------------------------------------*/
void remote_audio_mixer_channel_mute_get(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	audio_mixer_iface_t *mixer_iface = iface;

	if (!mixer_iface->channel_mute_get) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	const unsigned item = DEV_IPC_GET_ARG1(*call);
	const unsigned channel = DEV_IPC_GET_ARG2(*call);
	bool mute = false;
	const int ret =
	    mixer_iface->channel_mute_get(fun, item, channel, &mute);
	async_answer_1(callid, ret, mute);
}
/*----------------------------------------------------------------------------*/
void remote_audio_mixer_channel_volume_set(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	audio_mixer_iface_t *mixer_iface = iface;

	if (!mixer_iface->channel_volume_set) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	const unsigned item = DEV_IPC_GET_ARG1(*call);
	const unsigned channel = DEV_IPC_GET_ARG2(*call);
	const unsigned level = DEV_IPC_GET_ARG3(*call);
	const int ret =
	    mixer_iface->channel_volume_set(fun, item, channel, level);
	async_answer_0(callid, ret);
}
/*----------------------------------------------------------------------------*/
void remote_audio_mixer_channel_volume_get(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	audio_mixer_iface_t *mixer_iface = iface;

	if (!mixer_iface->channel_volume_get) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	const unsigned item = DEV_IPC_GET_ARG1(*call);
	const unsigned channel = DEV_IPC_GET_ARG2(*call);
	unsigned current = 0, max = 0;
	const int ret =
	    mixer_iface->channel_volume_get(fun, item, channel, &current, &max);
	async_answer_2(callid, ret, current, max);
}
#endif

/**
 * @}
 */

