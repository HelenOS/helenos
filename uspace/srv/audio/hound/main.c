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
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <hound/server.h>
#include <hound/protocol.h>
#include <task.h>

#include "hound.h"

#define NAMESPACE "audio"
#define NAME "hound"
#define CATEGORY "audio-pcm"

#include "log.h"

extern hound_server_iface_t hound_iface;

static hound_t hound;

static errno_t device_callback(service_id_t id, const char *name)
{
	return hound_add_device(&hound, id, name);
}

static void scan_for_devices(void *arg)
{
	hound_server_devices_iterate(device_callback);
}

int main(int argc, char **argv)
{
	printf("%s: HelenOS sound service\n", NAME);

	if (log_init(NAME) != EOK) {
		printf(NAME ": Failed to initialize logging.\n");
		return 1;
	}

	errno_t ret = hound_init(&hound);
	if (ret != EOK) {
		log_fatal("Failed to initialize hound structure: %s",
		    str_error(ret));
		return -ret;
	}

	hound_iface.server = &hound;
	hound_service_set_server_iface(&hound_iface);
	async_set_fallback_port_handler(hound_connection_handler, NULL);

	service_id_t id = 0;
	ret = hound_server_register(NAME, &id);
	if (ret != EOK) {
		log_fatal("Failed to register server: %s", str_error(ret));
		return -ret;
	}

	ret = hound_server_set_device_change_callback(scan_for_devices, NULL);
	if (ret != EOK) {
		log_fatal("Failed to register for device changes: %s",
		    str_error(ret));
		hound_server_unregister(id);
		return -ret;
	}
	log_info("Running with service id %" PRIun, id);

	scan_for_devices(NULL);
	task_retval(0);
	async_manager();
	return 0;
}

/**
 * @}
 */
