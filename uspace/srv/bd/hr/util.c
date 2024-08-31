/*
 * Copyright (c) 2024 Miroslav Cimerman
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

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#include <block.h>
#include <errno.h>
#include <hr.h>
#include <io/log.h>
#include <loc.h>
#include <str_error.h>

#include "var.h"
#include "util.h"

extern loc_srv_t *hr_srv;

errno_t hr_init_devs(hr_volume_t *vol)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_init_devs()");

	errno_t rc;
	size_t i;

	for (i = 0; i < vol->dev_no; i++) {
		rc = block_init(vol->devs[i]);
		log_msg(LOG_DEFAULT, LVL_DEBUG,
		    "hr_init_devs(): initing (%" PRIun ")", vol->devs[i]);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR,
			    "hr_init_devs(): initing (%" PRIun ") failed, aborting",
			    vol->devs[i]);
			break;
		}
	}

	return rc;
}

void hr_fini_devs(hr_volume_t *vol)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "hr_fini_devs()");

	size_t i;

	for (i = 0; i < vol->dev_no; i++)
		block_fini(vol->devs[i]);
}

errno_t hr_register_volume(hr_volume_t *new_volume)
{
	errno_t rc;
	service_id_t new_id;
	category_id_t cat_id;

	rc = loc_service_register(hr_srv, new_volume->devname, &new_id);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "unable to register device \"%s\": %s\n",
		    new_volume->devname, str_error(rc));
		goto error;
	}

	rc = loc_category_get_id("raid", &cat_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "failed resolving category \"raid\": %s\n", str_error(rc));
		goto error;
	}

	rc = loc_service_add_to_cat(hr_srv, new_id, cat_id);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "failed adding \"%s\" to category \"raid\": %s\n",
		    new_volume->devname, str_error(rc));
		goto error;
	}

	new_volume->svc_id = new_id;

error:
	return rc;
}

/** @}
 */
