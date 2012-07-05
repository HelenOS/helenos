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
#include <errno.h>
#include <assert.h>
#include <str.h>

#include "audio_mixer_iface.h"
#include "ddf/driver.h"

typedef enum {
	/** Asks for basic mixer info: Mixer name and number of controllable
	 * items.
	 * Answer:
	 * - ENOTSUP - call not supported
	 * - EOK - call successful, info is valid
	 * Answer arguments:
	 * - Mixer name
	 * - Number of controllable items
	 */
	IPC_M_AUDIO_MIXER_GET_INFO,

	/** Asks for item mixer info: Item name and number of controllable
	 * channels.
	 * Answer:
	 * - ENOTSUP - call not supported
	 * - ENOENT - no such item
	 * - EOK - call successful, info is valid
	 * Answer arguments:
	 * - Item name
	 * - Number of controllable channels
	 */
	IPC_M_AUDIO_MIXER_GET_ITEM_INFO,

	/** Asks for channel name and number of volume levels.
	 * Answer:
	 * - ENOTSUP - call not supported
	 * - ENOENT - no such channel
	 * - EOK - call successful, info is valid
	 * Answer arguments:
	 * - Channel name
	 * - Volume levels
	 */
	IPC_M_AUDIO_MIXER_GET_CHANNEL_INFO,

	/** Set channel mute status
	 * Answer:
	 * - ENOTSUP - call not supported
	 * - ENOENT - no such channel
	 * - EOK - call successful, info is valid
	 */
	IPC_M_AUDIO_MIXER_CHANNEL_MUTE_SET,

	/** Get channel mute status
	 * Answer:
	 * - ENOTSUP - call not supported
	 * - ENOENT - no such channel
	 * - EOK - call successful, info is valid
	 * Answer arguments:
	 * - Channel mute status
	 */
	IPC_M_AUDIO_MIXER_CHANNEL_MUTE_GET,

	/** Set channel volume level
	 * Answer:
	 * - ENOTSUP - call not supported
	 * - ENOENT - no such channel
	 * - EOK - call successful, info is valid
	 */
	IPC_M_AUDIO_MIXER_CHANNEL_VOLUME_SET,

	/** Get channel volume level
	 * Answer:
	 * - ENOTSUP - call not supported
	 * - ENOENT - no such channel
	 * - EOK - call successful, info is valid
	 * Answer arguments:
	 * - Channel volume level
	 * - Channel maximum volume level
	 */
	IPC_M_AUDIO_MIXER_CHANNEL_VOLUME_GET,
} audio_mixer_iface_funcs_t;

/*
 * CLIENT SIDE
 */
int audio_mixer_get_info(async_exch_t *exch, const char **name, unsigned *items)
{
	if (!exch)
		return EINVAL;
	sysarg_t name_size, itemc;
	const int ret = async_req_1_2(exch, DEV_IFACE_ID(AUDIO_MIXER_IFACE),
	    IPC_M_AUDIO_MIXER_GET_INFO, &name_size, &itemc);
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
	if (ret == EOK && items)
		*items = itemc;
	return ret;
}

int audio_mixer_get_item_info(async_exch_t *exch, unsigned item,
    const char ** name, unsigned *channels)
{
	if (!exch)
		return EINVAL;
	sysarg_t name_size, chans;
	const int ret = async_req_2_2(exch, DEV_IFACE_ID(AUDIO_MIXER_IFACE),
	    IPC_M_AUDIO_MIXER_GET_ITEM_INFO, item, &name_size, &chans);
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
	if (ret == EOK && chans)
		*channels = chans;
	return ret;
}

int audio_mixer_get_channel_info(async_exch_t *exch, unsigned item,
    unsigned channel, const char **name, unsigned *volume_levels)
{
	if (!exch)
		return EINVAL;
	sysarg_t name_size, levels;
	const int ret = async_req_3_2(exch, DEV_IFACE_ID(AUDIO_MIXER_IFACE),
	    IPC_M_AUDIO_MIXER_GET_CHANNEL_INFO, item, channel,
	    &name_size, &levels);
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
	if (ret == EOK && volume_levels)
		*volume_levels = levels;
	return ret;
}

int audio_mixer_channel_mute_set(async_exch_t *exch, unsigned item,
    unsigned channel, bool mute_status)
{
	if (!exch)
		return EINVAL;
	return async_req_4_0(exch, DEV_IFACE_ID(AUDIO_MIXER_IFACE),
	    IPC_M_AUDIO_MIXER_CHANNEL_MUTE_SET, item, channel, mute_status);
}

int audio_mixer_channel_mute_get(async_exch_t *exch, unsigned item,
    unsigned channel, bool *mute_status)
{
	if (!exch)
		return EINVAL;
	sysarg_t mute;
	const int ret = async_req_3_1(exch, DEV_IFACE_ID(AUDIO_MIXER_IFACE),
	    IPC_M_AUDIO_MIXER_CHANNEL_MUTE_GET, item, channel, &mute);
	if (ret == EOK && mute_status)
		*mute_status = (bool)mute;
	return ret;
}

int audio_mixer_channel_volume_set(async_exch_t *exch, unsigned item,
    unsigned channel, unsigned volume)
{
	if (!exch)
		return EINVAL;
	return async_req_4_0(exch, DEV_IFACE_ID(AUDIO_MIXER_IFACE),
	    IPC_M_AUDIO_MIXER_CHANNEL_VOLUME_SET, item, channel, volume);
}

int audio_mixer_channel_volume_get(async_exch_t *exch, unsigned item,
    unsigned channel, unsigned *volume_current, unsigned *volume_max)
{
	if (!exch)
		return EINVAL;
	sysarg_t current, max;
	const int ret = async_req_3_2(exch, DEV_IFACE_ID(AUDIO_MIXER_IFACE),
	    IPC_M_AUDIO_MIXER_CHANNEL_VOLUME_GET, item, channel,
	    &current, &max);
	if (ret == EOK && volume_current)
		*volume_current = current;
	if (ret == EOK && volume_max)
		*volume_max = max;
	return ret;
}

/*
 * SERVER SIDE
 */
static void remote_audio_mixer_get_info(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_mixer_get_item_info(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_mixer_get_channel_info(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_mixer_channel_mute_set(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_mixer_channel_mute_get(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_mixer_channel_volume_set(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_mixer_channel_volume_get(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote audio mixer interface operations. */
static remote_iface_func_ptr_t remote_audio_mixer_iface_ops[] = {
	[IPC_M_AUDIO_MIXER_GET_INFO] = remote_audio_mixer_get_info,
	[IPC_M_AUDIO_MIXER_GET_ITEM_INFO] = remote_audio_mixer_get_item_info,
	[IPC_M_AUDIO_MIXER_GET_CHANNEL_INFO] = remote_audio_mixer_get_channel_info,
	[IPC_M_AUDIO_MIXER_CHANNEL_MUTE_SET] = remote_audio_mixer_channel_mute_set,
	[IPC_M_AUDIO_MIXER_CHANNEL_MUTE_GET] = remote_audio_mixer_channel_mute_get,
	[IPC_M_AUDIO_MIXER_CHANNEL_VOLUME_SET] = remote_audio_mixer_channel_volume_set,
	[IPC_M_AUDIO_MIXER_CHANNEL_VOLUME_GET] = remote_audio_mixer_channel_volume_get,
};

/** Remote audio mixer interface structure. */
remote_iface_t remote_audio_mixer_iface = {
	.method_count = sizeof(remote_audio_mixer_iface_ops) /
	    sizeof(remote_audio_mixer_iface_ops[0]),
	.methods = remote_audio_mixer_iface_ops
};

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

/**
 * @}
 */
