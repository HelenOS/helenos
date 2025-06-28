/*
 * Copyright (c) 2025 Miroslav Cimerman
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

/** @addtogroup libdevice
 * @{
 */
/**
 * @file HelenRAID client API
 */

#include <abi/ipc/interfaces.h>
#include <async.h>
#include <hr.h>
#include <ipc/hr.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>
#include <stdio.h>
#include <str.h>

/** Initialize server session.
 *
 * @param rhr	Place to store inited session
 *
 * @return EOK on success or an error code
 */
errno_t hr_sess_init(hr_t **rhr)
{
	errno_t rc;
	hr_t *hr = NULL;

	if (rhr == NULL)
		return EINVAL;

	hr = calloc(1, sizeof(hr_t));
	if (hr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	service_id_t hr_svcid;

	rc = loc_service_get_id(SERVICE_NAME_HR, &hr_svcid, 0);
	if (rc != EOK)
		goto error;

	hr->sess = loc_service_connect(hr_svcid, INTERFACE_HR, 0);
	if (hr->sess == NULL) {
		rc = EIO;
		goto error;
	}

	*rhr = hr;
	return EOK;
error:
	if (hr != NULL)
		free(hr);

	return rc;
}

/** Destroy server session.
 *
 * @param hr	Session to destroy
 */
void hr_sess_destroy(hr_t *hr)
{
	if (hr == NULL)
		return;

	async_hangup(hr->sess);
	free(hr);
}

/** Create volume.
 *
 * @param hr		Server session
 * @param hr_config	Config to create from
 *
 * @return EOK on success or an error code
 */
errno_t hr_create(hr_t *hr, hr_config_t *hr_config)
{
	errno_t rc, retval;
	async_exch_t *exch;
	aid_t req;

	exch = async_exchange_begin(hr->sess);
	if (exch == NULL)
		return EINVAL;

	req = async_send_0(exch, HR_CREATE, NULL);

	rc = async_data_write_start(exch, hr_config, sizeof(hr_config_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	async_exchange_end(exch);
	async_wait_for(req, &retval);
	return retval;
}

/** Assemble volumes.
 *
 * @param hr		 Server session
 * @param hr_config	 Config to assemble from
 * @param rassembled_cnt Place to store assembled count
 *
 * @return EOK on success or an error code
 */
errno_t hr_assemble(hr_t *hr, hr_config_t *hr_config, size_t *rassembled_cnt)
{
	errno_t rc;
	async_exch_t *exch;
	aid_t req;
	size_t assembled_cnt;

	exch = async_exchange_begin(hr->sess);
	if (exch == NULL)
		return EINVAL;

	req = async_send_0(exch, HR_ASSEMBLE, NULL);

	rc = async_data_write_start(exch, hr_config, sizeof(hr_config_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_read_start(exch, &assembled_cnt, sizeof(size_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	async_exchange_end(exch);
	async_wait_for(req, &rc);

	if (rassembled_cnt != NULL)
		*rassembled_cnt = assembled_cnt;

	return rc;
}

/** Automatically assemble volumes.
 *
 * @param hr		 Server session
 * @param rassembled_cnt Place to store assembled count
 *
 * @return EOK on success or an error code
 */
errno_t hr_auto_assemble(hr_t *hr, size_t *rassembled_cnt)
{
	errno_t rc;
	size_t assembled_cnt;

	async_exch_t *exch = async_exchange_begin(hr->sess);
	if (exch == NULL) {
		rc = EINVAL;
		goto error;
	}

	aid_t req = async_send_0(exch, HR_AUTO_ASSEMBLE, NULL);

	rc = async_data_read_start(exch, &assembled_cnt, sizeof(size_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	async_exchange_end(exch);
	async_wait_for(req, &rc);

	if (rassembled_cnt != NULL)
		*rassembled_cnt = assembled_cnt;
error:
	return rc;
}

/** Stop/deactivate volume.
 *
 * @param hr		Server session
 * @param devname	Volume name
 *
 * @return EOK on success or an error code
 */
errno_t hr_stop(hr_t *hr, const char *devname)
{
	errno_t rc;
	async_exch_t *exch;
	service_id_t svc_id;

	rc = loc_service_get_id(devname, &svc_id, 0);
	if (rc != EOK)
		return rc;

	exch = async_exchange_begin(hr->sess);
	if (exch == NULL) {
		rc = EINVAL;
		goto error;
	}

	rc = async_req_1_0(exch, HR_STOP, svc_id);
	async_exchange_end(exch);
error:
	return rc;
}

/** Stop/deactivate all volumes.
 *
 * @param hr		Server session
 *
 * @return EOK on success or an error code
 */
errno_t hr_stop_all(hr_t *hr)
{
	async_exch_t *exch;
	errno_t rc;

	exch = async_exchange_begin(hr->sess);
	if (exch == NULL) {
		rc = EINVAL;
		goto error;
	}

	rc = async_req_0_0(exch, HR_STOP_ALL);
	async_exchange_end(exch);
error:
	return rc;
}

/** Fail an extent in volume.
 *
 * @param hr		Server session
 * @param volume_name	Volume name
 * @param extent	Extent index to fail
 *
 * @return EOK on success or an error code
 */
errno_t hr_fail_extent(hr_t *hr, const char *volume_name, unsigned long extent)
{
	errno_t rc;
	async_exch_t *exch;
	service_id_t vol_svc_id;

	rc = loc_service_get_id(volume_name, &vol_svc_id, 0);
	if (rc != EOK)
		return rc;

	exch = async_exchange_begin(hr->sess);
	if (exch == NULL) {
		rc = EINVAL;
		goto error;
	}

	rc = async_req_2_0(exch, HR_FAIL_EXTENT, vol_svc_id, extent);
	async_exchange_end(exch);
error:
	return rc;
}

/** Add a hotspare to volume.
 *
 * @param hr		Server session
 * @param volume_name	Volume name
 * @param hotspare	Hotspare service name
 *
 * @return EOK on success or an error code
 */
errno_t hr_add_hotspare(hr_t *hr, const char *volume_name, const char *hotspare)
{
	errno_t rc;
	async_exch_t *exch;
	service_id_t vol_svc_id, hs_svc_id;

	rc = loc_service_get_id(volume_name, &vol_svc_id, 0);
	if (rc != EOK)
		return rc;

	rc = loc_service_get_id(hotspare, &hs_svc_id, 0);
	if (rc != EOK)
		return rc;

	exch = async_exchange_begin(hr->sess);
	if (exch == NULL) {
		rc = EINVAL;
		goto error;
	}

	rc = async_req_2_0(exch, HR_ADD_HOTSPARE, vol_svc_id, hs_svc_id);
	async_exchange_end(exch);
error:
	return rc;
}

/** Get state of volumes.
 *
 * @param hr		Server session
 * @param rpairs	Place to store pointer to (service id, vol state) pairs
 * @param rcnt		Place to store pair count
 *
 * @return EOK on success or an error code
 */
errno_t hr_get_vol_states(hr_t *hr, hr_pair_vol_state_t **rpairs, size_t *rcnt)
{
	errno_t rc, retval;
	async_exch_t *exch;
	aid_t req;
	size_t cnt, i;
	hr_pair_vol_state_t *pairs = NULL;

	exch = async_exchange_begin(hr->sess);
	if (exch == NULL) {
		rc = EINVAL;
		goto error;
	}

	req = async_send_0(exch, HR_GET_VOL_STATES, NULL);
	rc = async_data_read_start(exch, &cnt, sizeof(size_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	pairs = calloc(cnt, sizeof(*pairs));
	if (pairs == NULL) {
		async_exchange_end(exch);
		async_forget(req);
		return ENOMEM;
	}

	for (i = 0; i < cnt; i++) {
		rc = async_data_read_start(exch, &pairs[i], sizeof(*pairs));
		if (rc != EOK) {
			async_exchange_end(exch);
			async_forget(req);
			goto error;
		}
	}

	async_exchange_end(exch);
	async_wait_for(req, &retval);
	if (retval != EOK) {
		rc = retval;
		goto error;
	}

	if (rpairs != NULL)
		*rpairs = pairs;
	if (rcnt != NULL)
		*rcnt = cnt;
	return EOK;

error:
	if (pairs != NULL)
		free(pairs);
	return rc;
}

/** Get volume info.
 *
 * @param hr		Server session
 * @param svc_id	Service id of volume
 * @param rinfo		Place to store volume info
 *
 * @return EOK on success or an error code
 */
errno_t hr_get_vol_info(hr_t *hr, service_id_t svc_id, hr_vol_info_t *rinfo)
{
	errno_t rc, retval;
	async_exch_t *exch;
	aid_t req;

	exch = async_exchange_begin(hr->sess);
	if (exch == NULL) {
		rc = EINVAL;
		goto error;
	}

	req = async_send_0(exch, HR_GET_VOL_INFO, NULL);
	rc = async_data_write_start(exch, &svc_id, sizeof(svc_id));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_read_start(exch, rinfo, sizeof(*rinfo));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		goto error;
	}

	async_wait_for(req, &retval);
	if (retval != EOK) {
		rc = retval;
		goto error;
	}

error:
	return rc;
}

/** Get volume state string.
 *
 * @param state	State value
 *
 * @return State string
 */
const char *hr_get_vol_state_str(hr_vol_state_t state)
{
	switch (state) {
	case HR_VOL_NONE:
		return "NONE/UNKNOWN";
	case HR_VOL_OPTIMAL:
		return "OPTIMAL";
	case HR_VOL_FAULTY:
		return "FAULTY";
	case HR_VOL_DEGRADED:
		return "DEGRADED";
	case HR_VOL_REBUILD:
		return "REBUILD";
	default:
		return "Invalid state value";
	}
}

/** Get extent state string.
 *
 * @param state	State value
 *
 * @return State string
 */
const char *hr_get_ext_state_str(hr_ext_state_t state)
{
	switch (state) {
	case HR_EXT_NONE:
		return "NONE/UNKNOWN";
	case HR_EXT_INVALID:
		return "INVALID";
	case HR_EXT_ONLINE:
		return "ONLINE";
	case HR_EXT_MISSING:
		return "MISSING";
	case HR_EXT_FAILED:
		return "FAILED";
	case HR_EXT_REBUILD:
		return "REBUILD";
	case HR_EXT_HOTSPARE:
		return "HOTSPARE";
	default:
		return "Invalid state value";
	}
}

/** Get volume layout string.
 *
 * @param layout Layout value
 *
 * @return Layout string
 */
const char *hr_get_layout_str(hr_layout_t layout)
{
	switch (layout) {
	case HR_LAYOUT_NONE:
		return "RAID layout not set";
	case HR_LAYOUT_RAID4_0:
		return "RAID-4 Non-Rotating Parity 0";
	case HR_LAYOUT_RAID4_N:
		return "RAID-4 Non-Rotating Parity N";
	case HR_LAYOUT_RAID5_0R:
		return "RAID-5 Rotating Parity 0 with Data Restart";
	case HR_LAYOUT_RAID5_NR:
		return "RAID-5 Rotating Parity N with Data Restart";
	case HR_LAYOUT_RAID5_NC:
		return "RAID-5 Rotating Parity N with Data Continuation";
	default:
		return "Invalid RAID layout";
	}
}

/** Get volume level string.
 *
 * @param level Levelvalue
 *
 * @return Level string
 */
const char *hr_get_level_str(hr_level_t level)
{
	switch (level) {
	case HR_LVL_0:
		return "stripe (RAID 0)";
	case HR_LVL_1:
		return "mirror (RAID 1)";
	case HR_LVL_4:
		return "dedicated parity (RAID 4)";
	case HR_LVL_5:
		return "distributed parity (RAID 5)";
	default:
		return "Invalid RAID level";
	}
}

/** Get volume metadata type string.
 *
 * @param type Metadata type value
 *
 * @return Metadata type string
 */
const char *hr_get_metadata_type_str(hr_metadata_type_t type)
{
	switch (type) {
	case HR_METADATA_NATIVE:
		return "HelenRAID native";
	case HR_METADATA_GEOM_MIRROR:
		return "GEOM::MIRROR";
	case HR_METADATA_GEOM_STRIPE:
		return "GEOM::STRIPE";
	case HR_METADATA_SOFTRAID:
		return "OpenBSD softraid";
	default:
		return "Invalid metadata type value";
	}
}

/** @}
 */
