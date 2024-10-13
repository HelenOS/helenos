/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup display
 * @{
 */
/**
 * @file Display server input device configuration
 */

#include <adt/list.h>
#include <errno.h>
#include <io/log.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include "display.h"
#include "idevcfg.h"
#include "seat.h"

/** Create input device configuration entry.
 *
 * @param display Parent display
 * @param svc_id Device service ID
 * @param seat Seat to which device is assigned
 * @param ridevcfg Place to store pointer to new input device configuration
 *                 entry
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t ds_idevcfg_create(ds_display_t *display, service_id_t svc_id,
    ds_seat_t *seat, ds_idevcfg_t **ridevcfg)
{
	ds_idevcfg_t *idevcfg;

	idevcfg = calloc(1, sizeof(ds_idevcfg_t));
	if (idevcfg == NULL)
		return ENOMEM;

	idevcfg->svc_id = svc_id;
	ds_seat_add_idevcfg(seat, idevcfg);

	ds_display_add_idevcfg(display, idevcfg);
	*ridevcfg = idevcfg;
	return EOK;
}

/** Destroy input device configuration entry.
 *
 * @param ddev Display device
 */
void ds_idevcfg_destroy(ds_idevcfg_t *idevcfg)
{
	ds_display_remove_idevcfg(idevcfg);
	ds_seat_remove_idevcfg(idevcfg);
	free(idevcfg);
}

/** Load input device configuration entry from SIF node.
 *
 * @param display Display
 * @param enode Entry node from which the entry should be loaded
 * @param ridevcfg Place to store pointer to the newly loaded input device
 *                 configuration entry
 *
 * @return EOK on success or an error code
 */
errno_t ds_idevcfg_load(ds_display_t *display, sif_node_t *enode,
    ds_idevcfg_t **ridevcfg)
{
	const char *svc_name;
	const char *sseat_id;
	char *endptr;
	service_id_t svc_id;
	unsigned long seat_id;
	ds_seat_t *seat;
	ds_idevcfg_t *idevcfg;
	errno_t rc;

	svc_name = sif_node_get_attr(enode, "svc-name");
	if (svc_name == NULL)
		return EIO;

	sseat_id = sif_node_get_attr(enode, "seat-id");
	if (sseat_id == NULL)
		return EIO;

	rc = loc_service_get_id(svc_name, &svc_id, 0);
	if (rc != EOK)
		return rc;

	seat_id = strtoul(sseat_id, &endptr, 10);
	if (*endptr != '\0')
		return EIO;

	seat = ds_display_find_seat(display, seat_id);
	if (seat == NULL)
		return EIO;

	rc = ds_idevcfg_create(display, svc_id, seat, &idevcfg);
	if (rc != EOK)
		return rc;

	(void)idevcfg;
	return EOK;
}

/** Save input device configuration entry to SIF node.
 *
 * @param idevcfg Input device configuration entry
 * @param enode Entry node to which the entry should be saved
 *
 * @return EOK on success or an error code
 */
errno_t ds_idevcfg_save(ds_idevcfg_t *idevcfg, sif_node_t *enode)
{
	char *svc_name;
	char *sseat_id;
	errno_t rc;
	int rv;

	rc = loc_service_get_name(idevcfg->svc_id, &svc_name);
	if (rc != EOK)
		return rc;

	rc = sif_node_set_attr(enode, "svc-name", svc_name);
	if (rc != EOK) {
		free(svc_name);
		return rc;
	}

	free(svc_name);

	rv = asprintf(&sseat_id, "%lu", (unsigned long)idevcfg->seat->id);
	if (rv < 0) {
		rc = ENOMEM;
		return rc;
	}

	rc = sif_node_set_attr(enode, "seat-id", sseat_id);
	if (rc != EOK) {
		free(sseat_id);
		return rc;
	}

	free(sseat_id);
	return EOK;
}

/** @}
 */
