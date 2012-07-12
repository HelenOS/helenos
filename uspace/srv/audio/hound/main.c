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

#include "log.h"

hound_t hound;

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


int main(int argc, char **argv)
{
	printf("%s: HelenOS sound service\n", NAME);

	int ret = hound_init(&hound);
	if (ret != EOK) {
		log_error("Failed to initialize hound structure: %s\n",
		    str_error(ret));
		return 1;
	}

	ret = loc_register_cat_change_cb(scan_for_devices);

	scan_for_devices();


	async_manager();
	return 0;
}

/**
 * @}
 */
