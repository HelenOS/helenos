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
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <hound/server.h>


#include "hound.h"

#define NAMESPACE "audio"
#define NAME "hound"
#define CATEGORY "audio-pcm"

#include "audio_client.h"
#include "log.h"

static hound_t hound;

static int device_callback(service_id_t id, const char *name)
{
	return hound_add_device(&hound, id, name);
}

static void scan_for_devices(void)
{
	hound_server_devices_iterate(device_callback);
}

static void client_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	async_answer_0(iid, EOK);

	LIST_INITIALIZE(local_playback);
	LIST_INITIALIZE(local_recording);
	audio_format_t format = {0};
	const char *name = NULL;
	async_sess_t *sess = NULL;

	while (1) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		switch (IPC_GET_IMETHOD(call)) {
		case HOUND_REGISTER_PLAYBACK: {
			hound_server_get_register_params(&name, &sess,
			    &format.channels, &format.sampling_rate,
			    &format.sample_format);
			audio_client_t *client =
			    audio_client_get_playback(name, &format, sess);
			free(name);
			if (!client) {
				log_error("Failed to create playback client");
				async_answer_0(callid, ENOMEM);
				break;
			}
			int ret = hound_add_source(&hound, &client->source);
			if (ret != EOK){
				audio_client_destroy(client);
				log_error("Failed to add audio source: %s",
				    str_error(ret));
				async_answer_0(callid, ret);
				break;
			}
			log_info("Added audio client %p '%s'",
			    client, client->name);
			async_answer_0(callid, EOK);
			list_append(&client->link, &local_playback);
			break;
		}
		case HOUND_REGISTER_RECORDING: {
			hound_server_get_register_params(&name, &sess,
			    &format.channels, &format.sampling_rate,
			    &format.sample_format);
			audio_client_t *client =
			    audio_client_get_recording(name, &format, sess);
			free(name);
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
			const char *name = NULL;
			hound_server_get_unregister_params(&name);
			int ret = ENOENT;
			list_foreach(local_playback, it) {
				audio_client_t *client =
				    audio_client_list_instance(it);
				if (str_cmp(client->name, name) == 0) {
					ret = hound_remove_source(&hound,
					    &client->source);
					if (ret == EOK) {
						list_remove(&client->link);
						audio_client_destroy(client);
					}
					break;
				}
			}
			free(name);
			async_answer_0(callid, ret);
			break;
		}
		case HOUND_UNREGISTER_RECORDING: {
			const char *name = NULL;
			hound_server_get_unregister_params(&name);
			int ret = ENOENT;
			list_foreach(local_recording, it) {
				audio_client_t *client =
				    audio_client_list_instance(it);
				if (str_cmp(client->name, name) == 0) {
					ret = hound_remove_sink(&hound,
					    &client->sink);
					if (ret == EOK) {
						list_remove(&client->link);
						audio_client_destroy(client);
					}
					break;
				}
			}
			free(name);
			async_answer_0(callid, ret);
			break;
		}
		case HOUND_CONNECT: {
			const char *source = NULL, *sink = NULL;
			hound_server_get_connection_params(&source, &sink);
			const int ret = hound_connect(&hound, source, sink);
			if (ret != EOK)
				log_error("Failed to connect '%s' to '%s': %s",
				    source, sink, str_error(ret));
			free(source);
			free(sink);
			async_answer_0(callid, ret);
			break;
		}
		case HOUND_DISCONNECT: {
			const char *source = NULL, *sink = NULL;
			hound_server_get_connection_params(&source, &sink);
			const int ret = hound_disconnect(&hound, source, sink);
			if (ret != EOK)
				log_error("Failed to disconnect '%s' from '%s'"
				    ": %s", source, sink, str_error(ret));
			free(source);
			free(sink);
			async_answer_0(callid, ret);
			break;
		}
		default:
			log_debug("Got unknown method %u",
			    IPC_GET_IMETHOD(call));
			async_answer_0(callid, ENOTSUP);
			break;
		case 0:
			while(!list_empty(&local_recording)) {
				audio_client_t *client =
				    audio_client_list_instance(
				        list_first(&local_recording));
				list_remove(&client->link);
				hound_remove_sink(&hound, &client->sink);
				audio_client_destroy(client);
			}
			while(!list_empty(&local_playback)) {
				audio_client_t *client =
				    audio_client_list_instance(
				        list_first(&local_playback));
				list_remove(&client->link);
				hound_remove_source(&hound, &client->source);
				audio_client_destroy(client);
			}
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

	service_id_t id = 0;
	ret = hound_server_register(NAME, &id);
	if (ret != EOK) {
		log_fatal("Failed to register server: %s", str_error(ret));
		return -ret;
	}

	ret = hound_server_set_device_change_callback(scan_for_devices);
	if (ret != EOK) {
		log_fatal("Failed to register for device changes: %s",
		    str_error(ret));
		hound_server_unregister(id);
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
