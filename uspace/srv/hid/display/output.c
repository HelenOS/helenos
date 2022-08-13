/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup display
 * @{
 */
/**
 * @file Display server output
 */

#include <assert.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/kbd_event.h>
#include <io/log.h>
#include <io/pos_event.h>
#include <loc.h>
#include <stdlib.h>
#include "ddev.h"
#include "output.h"

/** Check for new display devices.
 *
 * @param output Display server output
 */
static errno_t ds_output_check_new_devs(ds_output_t *output)
{
	category_id_t ddev_cid;
	service_id_t *svcs;
	size_t count, i;
	bool already_known;
	ds_ddev_t *nddev;
	errno_t rc;

	assert(fibril_mutex_is_locked(&output->lock));

	rc = loc_category_get_id("display-device", &ddev_cid,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Error looking up category 'display-device'.");
		return EIO;
	}

	/*
	 * Check for new dispay devices
	 */
	rc = loc_category_get_svcs(ddev_cid, &svcs, &count);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Error getting list of display devices.");
		return EIO;
	}

	for (i = 0; i < count; i++) {
		already_known = false;

		/* Determine whether we already know this device. */
		list_foreach(output->ddevs, loutdevs, ds_ddev_t, ddev) {
			if (ddev->svc_id == svcs[i]) {
				already_known = true;
				break;
			}
		}

		if (!already_known) {
			rc = ds_ddev_open(output->def_display, svcs[i], &nddev);
			if (rc != EOK) {
				log_msg(LOG_DEFAULT, LVL_ERROR,
				    "Error adding display device.");
				continue;
			}

			list_append(&nddev->loutdevs, &output->ddevs);

			log_msg(LOG_DEFAULT, LVL_NOTE,
			    "Added display device '%lu'",
			    (unsigned long) svcs[i]);
		}
	}

	free(svcs);

	return EOK;
}

/** Display device category change callback.
 *
 * @param arg Display server output (cast to void *)
 */
static void ds_ddev_change_cb(void *arg)
{
	ds_output_t *output = (ds_output_t *) arg;

	fibril_mutex_lock(&output->lock);
	(void) ds_output_check_new_devs(output);
	fibril_mutex_unlock(&output->lock);
}

/** Create display server output.
 *
 * @param routput Place to store pointer to display server output object.
 * @return EOK on success or an error code
 */
errno_t ds_output_create(ds_output_t **routput)
{
	ds_output_t *output;

	output = calloc(1, sizeof(ds_output_t));
	if (output == NULL)
		return ENOMEM;

	fibril_mutex_initialize(&output->lock);
	list_initialize(&output->ddevs);

	*routput = output;
	return EOK;
}

/** Start display device discovery.
 *
 * @param output Display server output
 * @return EOK on success or an error code
 */
errno_t ds_output_start_discovery(ds_output_t *output)
{
	errno_t rc;

	rc = loc_register_cat_change_cb(ds_ddev_change_cb, output);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed registering callback for device discovery.");
		return rc;
	}

	fibril_mutex_lock(&output->lock);
	rc = ds_output_check_new_devs(output);
	fibril_mutex_unlock(&output->lock);

	/* Fail if we did not open at least one device */
	if (list_empty(&output->ddevs)) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "No output device found.");
		return ENOENT;
	}

	return rc;
}

/** Destroy display server output.
 *
 * @param output Display server output
 */
void ds_output_destroy(ds_output_t *output)
{
	free(output);
}

/** @}
 */
