/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
