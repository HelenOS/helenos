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

/**
 * @addtogroup audio
 * @brief HelenOS sound server.
 * @{
 */
/** @file
 */

#include <async.h>
#include <bool.h>
#include <errno.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>

#include "hound.h"

#define NAMESPACE "audio"
#define NAME "hound"
#define CATEGORY "audio-pcm"

#include "audio_client.h"
#include "log.h"
#include "protocol.h"

static hound_t hound;

static inline audio_format_t read_format(const ipc_call_t *call)
{
	audio_format_t format = {
		.channels = IPC_GET_ARG1(*call),
		.sampling_rate = IPC_GET_ARG2(*call),
		.sample_format = IPC_GET_ARG3(*call),
	};
	return format;
}
static inline const char *get_name()
{
	size_t size = 0;
	ipc_callid_t callid;
	async_data_read_receive(&callid, &size);
	char *buffer = malloc(size);
	if (buffer) {
		async_data_read_finalize(callid, buffer, size);
		buffer[size - 1] = 0;
		log_verbose("Got name from client: %s", buffer);
	}
	return buffer;
}
static inline async_sess_t *get_session()
{
	ipc_call_t call;
	ipc_callid_t callid = async_get_call(&call);
	async_sess_t *s = async_callback_receive_start(EXCHANGE_ATOMIC, &call);
	async_answer_0(callid, s ? EOK : ENOMEM);
	log_verbose("Received callback session");
	return s;
}


static void scan_for_devices(void)
{
	static bool cat_resolved = false;
	static category_id_t cat;

	if (!cat_resolved) {
		log_verbose("Resolving category \"%s\".", CATEGORY);
		const int ret = loc_category_get_id(CATEGORY, &cat,
		    IPC_FLAG_BLOCKING);
		if (ret != EOK) {
			log_error("Failed to get category: %s", str_error(ret));
			return;
		}
		cat_resolved = true;
	}

	log_verbose("Getting available services in category.");

	service_id_t *svcs = NULL;
	size_t count = 0;
	const int ret = loc_category_get_svcs(cat, &svcs, &count);
	if (ret != EOK) {
		log_error("Failed to get audio devices: %s", str_error(ret));
		return;
	}

	for (unsigned i = 0; i < count; ++i) {
		char *name = NULL;
		int ret = loc_service_get_name(svcs[i], &name);
		if (ret != EOK) {
			log_error("Failed to get dev name: %s", str_error(ret));
			continue;
		}
		ret = hound_add_device(&hound, svcs[i], name);
		if (ret != EOK && ret != EEXISTS) {
			log_error("Failed to add audio device \"%s\": %s",
			    name, str_error(ret));
		}
		free(name);
	}

	free(svcs);
}

static void client_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	async_answer_0(iid, EOK);

	LIST_INITIALIZE(local_playback);
	LIST_INITIALIZE(local_recording);

	while (1) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		log_debug("Got method %u", IPC_GET_IMETHOD(call));
		switch (IPC_GET_IMETHOD(call)) {
		case HOUND_REGISTER_PLAYBACK: {
			const audio_format_t format = read_format(&call);
			const char *name = get_name();
			async_sess_t *sess = get_session();
			audio_client_t * client =
			    audio_client_get_playback(name, &format, sess);
			if (!client) {
				log_error("Failed to create playback client");
				async_answer_0(callid, ENOMEM);
			}
			int ret = hound_add_source(&hound, &client->source);
			if (ret != EOK){
				audio_client_destroy(client);
				log_error("Failed to add audio source: %s",
				    str_error(ret));
				async_answer_0(callid, ret);
				break;
			}
			async_answer_0(callid, EOK);
			list_append(&client->link, &local_playback);
			break;
		}
		case HOUND_REGISTER_RECORDING: {
			const audio_format_t format = read_format(&call);
			const char *name = get_name();
			async_sess_t *sess = get_session();
			audio_client_t * client =
			    audio_client_get_recording(name, &format, sess);
			if (!client) {
				log_error("Failed to create recording client");
				async_answer_0(callid, ENOMEM);
				break;
			}
			int ret = hound_add_sink(&hound, &client->sink);
			if (ret != EOK){
				audio_client_destroy(client);
				log_error("Failed to add audio sink: %s",
				    str_error(ret));
				async_answer_0(callid, ret);
				break;
			}
			async_answer_0(callid, EOK);
			list_append(&client->link, &local_recording);
			break;
		}
		case HOUND_UNREGISTER_PLAYBACK: {
			//const char *name = get_name();
			//TODO unregister in hound
			//TODO remove from local
			break;
		}
		case HOUND_UNREGISTER_RECORDING: {
			//TODO Get Name
			//TODO unregister in hound
			//TODO remove from local
			break;
		}
		case HOUND_CONNECT: {
			//TODO Get Name
			//TODO Get Name
			//TODO connect in hound
			break;
		}
		case HOUND_DISCONNECT: {
			//TODO Get Name
			//TODO Get Name
			//TODO disconnect in hound
			break;
		}
		default:
			async_answer_0(callid, ENOTSUP);
			break;
		case 0:
			return;
		}
	}
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS sound service\n", NAME);

	int ret = hound_init(&hound);
	if (ret != EOK) {
		log_fatal("Failed to initialize hound structure: %s",
		    str_error(ret));
		return -ret;
	}

	async_set_client_connection(client_connection);
	ret = loc_server_register(NAME);
	if (ret != EOK) {
		log_fatal("Failed to register sound server: %s",
		    str_error(ret));
		return -ret;
	}

	char fqdn[LOC_NAME_MAXLEN + 1];
	snprintf(fqdn, LOC_NAME_MAXLEN, "%s/%s", NAMESPACE, NAME);
	service_id_t id = 0;
	ret = loc_service_register(fqdn, &id);
	if (ret != EOK) {
		log_fatal("Failed to register sound service: %s",
		    str_error(ret));
		return -ret;
	}

	ret = loc_register_cat_change_cb(scan_for_devices);
	if (ret != EOK) {
		log_fatal("Failed to register for category changes: %s",
		    str_error(ret));
		loc_service_unregister(id);
		return -ret;
	}
	log_info("Running with service id %u", id);

	scan_for_devices();

	async_manager();
	return 0;
}

/**
 * @}
 */
