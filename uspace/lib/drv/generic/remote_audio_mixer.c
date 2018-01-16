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
#include <macros.h>

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

	/** Set new control item setting
	 * Answer:
	 * - ENOTSUP - call not supported
	 * - ENOENT - no such control item
	 * - EOK - call successful, info is valid
	 */
	IPC_M_AUDIO_MIXER_SET_ITEM_LEVEL,

	/** Get control item setting
	 * Answer:
	 * - ENOTSUP - call not supported
	 * - ENOENT - no such control item
	 * - EOK - call successful, info is valid
	 */
	IPC_M_AUDIO_MIXER_GET_ITEM_LEVEL,
} audio_mixer_iface_funcs_t;

/*
 * CLIENT SIDE
 */

/**
 * Query audio mixer for basic info (name and items count).
 * @param[in] exch IPC exchange connected to the device
 * @param[out] name Audio mixer string identifier
 * @param[out] items Number of items controlled by the mixer.
 * @return Error code.
 */
errno_t audio_mixer_get_info(async_exch_t *exch, char **name, unsigned *items)
{
	if (!exch)
		return EINVAL;
	sysarg_t name_size, itemc;
	const errno_t ret = async_req_1_2(exch, DEV_IFACE_ID(AUDIO_MIXER_IFACE),
	    IPC_M_AUDIO_MIXER_GET_INFO, &name_size, &itemc);
	if (ret == EOK && name) {
		char *name_place = calloc(1, name_size);
		if (!name_place) {
			/* Make the other side fail
			 * as it waits for read request */
			async_data_read_start(exch, (void*)-1, 0);
			return ENOMEM;
		}
		const errno_t ret =
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

/**
 * Query audio mixer for item specific info (name and channels count).
 * @param[in] exch IPC exchange connected to the device
 * @param[in] item The control item
 * @param[out] name Control item string identifier.
 * @param[out] channles Number of channels associated with this control item.
 * @return Error code.
 */
errno_t audio_mixer_get_item_info(async_exch_t *exch, unsigned item,
    char **name, unsigned *levels)
{
	if (!exch)
		return EINVAL;
	sysarg_t name_size, lvls;
	const errno_t ret = async_req_2_2(exch, DEV_IFACE_ID(AUDIO_MIXER_IFACE),
	    IPC_M_AUDIO_MIXER_GET_ITEM_INFO, item, &name_size, &lvls);
	if (ret == EOK && name) {
		char *name_place = calloc(1, name_size);
		if (!name_place) {
			/* Make the other side fail
			 * as it waits for read request */
			async_data_read_start(exch, (void*)-1, 0);
			return ENOMEM;
		}
		const errno_t ret =
		    async_data_read_start(exch, name_place, name_size);
		if (ret != EOK) {
			free(name_place);
			return ret;
		}
		*name = name_place;
	}
	if (ret == EOK && levels)
		*levels = lvls;
	return ret;
}

/**
 * Set control item to a new level.
 * @param[in] exch IPC exchange connected to the device.
 * @param[in] item The control item controlling the channel.
 * @param[in] level The new value.
 * @return Error code.
 */
errno_t audio_mixer_set_item_level(async_exch_t *exch, unsigned item,
    unsigned level)
{
	if (!exch)
		return EINVAL;
	return async_req_3_0(exch, DEV_IFACE_ID(AUDIO_MIXER_IFACE),
	    IPC_M_AUDIO_MIXER_SET_ITEM_LEVEL, item, level);
}

/**
 * Get current level of a control item.
 * @param[in] exch IPC exchange connected to the device.
 * @param[in] item The control item controlling the channel.
 * @param[in] channel The channel index.
 * @param[out] level Currently set value.
 * @return Error code.
 */
errno_t audio_mixer_get_item_level(async_exch_t *exch, unsigned item,
    unsigned *level)
{
	if (!exch)
		return EINVAL;
	sysarg_t current;
	const errno_t ret = async_req_2_1(exch, DEV_IFACE_ID(AUDIO_MIXER_IFACE),
	    IPC_M_AUDIO_MIXER_GET_ITEM_LEVEL, item, &current);
	if (ret == EOK && level)
		*level = current;
	return ret;
}

/*
 * SERVER SIDE
 */
static void remote_audio_mixer_get_info(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_mixer_get_item_info(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_mixer_get_item_level(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);
static void remote_audio_mixer_set_item_level(ddf_fun_t *, void *, ipc_callid_t, ipc_call_t *);

/** Remote audio mixer interface operations. */
static const remote_iface_func_ptr_t remote_audio_mixer_iface_ops[] = {
	[IPC_M_AUDIO_MIXER_GET_INFO] = remote_audio_mixer_get_info,
	[IPC_M_AUDIO_MIXER_GET_ITEM_INFO] = remote_audio_mixer_get_item_info,
	[IPC_M_AUDIO_MIXER_GET_ITEM_LEVEL] = remote_audio_mixer_get_item_level,
	[IPC_M_AUDIO_MIXER_SET_ITEM_LEVEL] = remote_audio_mixer_set_item_level,
};

/** Remote audio mixer interface structure. */
const remote_iface_t remote_audio_mixer_iface = {
	.method_count = ARRAY_SIZE(remote_audio_mixer_iface_ops),
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
	const errno_t ret = mixer_iface->get_info(fun, &name, &items);
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
	unsigned values = 0;
	const errno_t ret = mixer_iface->get_item_info(fun, item, &name, &values);
	const size_t name_size = name ? str_size(name) + 1 : 0;
	async_answer_2(callid, ret, name_size, values);
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

void remote_audio_mixer_set_item_level(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	audio_mixer_iface_t *mixer_iface = iface;

	if (!mixer_iface->set_item_level) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	const unsigned item = DEV_IPC_GET_ARG1(*call);
	const unsigned value = DEV_IPC_GET_ARG2(*call);
	const errno_t ret = mixer_iface->set_item_level(fun, item, value);
	async_answer_0(callid, ret);
}

void remote_audio_mixer_get_item_level(
    ddf_fun_t *fun, void *iface, ipc_callid_t callid, ipc_call_t *call)
{
	audio_mixer_iface_t *mixer_iface = iface;

	if (!mixer_iface->get_item_level) {
		async_answer_0(callid, ENOTSUP);
		return;
	}
	const unsigned item = DEV_IPC_GET_ARG1(*call);
	unsigned current = 0;
	const errno_t ret =
	    mixer_iface->get_item_level(fun, item, &current);
	async_answer_1(callid, ret, current);
}

/**
 * @}
 */
