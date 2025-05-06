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
 * @file
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

void hr_sess_destroy(hr_t *hr)
{
	if (hr == NULL)
		return;

	async_hangup(hr->sess);
	free(hr);
}

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

errno_t hr_auto_assemble(size_t *rassembled_cnt)
{
	hr_t *hr;
	errno_t rc;
	size_t assembled_cnt;

	rc = hr_sess_init(&hr);
	if (rc != EOK)
		return rc;

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
	hr_sess_destroy(hr);
	return rc;
}

static errno_t print_vol_info(size_t index, hr_vol_info_t *vol_info)
{
	errno_t rc;
	size_t i;
	char *devname;
	hr_extent_t *ext;

	printf("--- vol %zu ---\n", index);

	printf("svc_id: %" PRIun "\n", vol_info->svc_id);

	rc = loc_service_get_name(vol_info->svc_id, &devname);
	if (rc != EOK)
		return rc;
	printf("devname: %s\n", devname);

	printf("status: %s\n", hr_get_vol_status_msg(vol_info->status));

	printf("level: %d\n", vol_info->level);
	if (vol_info->level == HR_LVL_4 || vol_info->level == HR_LVL_5) {
		printf("layout: %s\n",
		    hr_get_layout_str(vol_info->layout));
	}
	if (vol_info->level == HR_LVL_0 || vol_info->level == HR_LVL_4) {
		if (vol_info->strip_size / 1024 < 1)
			printf("strip size in bytes: %" PRIu32 "\n",
			    vol_info->strip_size);
		else
			printf("strip size: %" PRIu32 "K\n",
			    vol_info->strip_size / 1024);
	}
	printf("size in bytes: %" PRIu64 "MiB\n",
	    vol_info->nblocks * vol_info->bsize / 1024 / 1024);
	printf("size in blocks: %" PRIu64 "\n", vol_info->nblocks);
	printf("block size: %zu\n", vol_info->bsize);

	if (vol_info->level == HR_LVL_4)
		printf("extents: [P] [status] [index] [devname]\n");
	else
		printf("extents: [status] [index] [devname]\n");
	for (i = 0; i < vol_info->extent_no; i++) {
		ext = &vol_info->extents[i];
		if (ext->status == HR_EXT_MISSING || ext->status == HR_EXT_NONE) {
			devname = (char *) "MISSING-devname";
		} else {
			rc = loc_service_get_name(ext->svc_id, &devname);
			if (rc != EOK) {
				printf("loc_service_get_name() failed, skipping...\n");
				continue;
			}
		}
		if (vol_info->level == HR_LVL_4) {
			if ((i == 0 && vol_info->layout == HR_RLQ_RAID4_0) ||
			    (i == vol_info->extent_no - 1 &&
			    vol_info->layout == HR_RLQ_RAID4_N))
				printf("          P   %s    %zu       %s\n", hr_get_ext_status_msg(ext->status), i, devname);
			else
				printf("              %s    %zu       %s\n", hr_get_ext_status_msg(ext->status), i, devname);
		} else {
			printf("          %s    %zu       %s\n", hr_get_ext_status_msg(ext->status), i, devname);
		}
	}

	if (vol_info->hotspare_no == 0)
		return EOK;

	printf("hotspares: [status] [index] [devname]\n");
	for (i = 0; i < vol_info->hotspare_no; i++) {
		ext = &vol_info->hotspares[i];
		if (ext->status == HR_EXT_MISSING) {
			devname = (char *) "MISSING-devname";
		} else {
			rc = loc_service_get_name(ext->svc_id, &devname);
			if (rc != EOK)
				return rc;
		}
		printf("            %s   %zu     %s\n",
		    hr_get_ext_status_msg(ext->status), i, devname);
	}

	return EOK;
}

errno_t hr_stop(const char *devname)
{
	hr_t *hr;
	errno_t rc;
	async_exch_t *exch;
	service_id_t svc_id;

	rc = loc_service_get_id(devname, &svc_id, 0);
	if (rc != EOK)
		return rc;

	rc = hr_sess_init(&hr);
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
	hr_sess_destroy(hr);
	return rc;
}

errno_t hr_stop_all(void)
{
	hr_t *hr;
	async_exch_t *exch;
	errno_t rc;

	rc = hr_sess_init(&hr);
	if (rc != EOK)
		return rc;

	exch = async_exchange_begin(hr->sess);
	if (exch == NULL) {
		rc = EINVAL;
		goto error;
	}

	rc = async_req_0_0(exch, HR_STOP_ALL);
	async_exchange_end(exch);
error:
	hr_sess_destroy(hr);
	return rc;
}

errno_t hr_fail_extent(const char *volume_name, unsigned long extent)
{
	hr_t *hr;
	errno_t rc;
	async_exch_t *exch;
	service_id_t vol_svc_id;

	rc = loc_service_get_id(volume_name, &vol_svc_id, 0);
	if (rc != EOK)
		return rc;

	rc = hr_sess_init(&hr);
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
	hr_sess_destroy(hr);
	return rc;
}

errno_t hr_add_hotspare(const char *volume_name, const char *hotspare)
{
	hr_t *hr;
	errno_t rc;
	async_exch_t *exch;
	service_id_t vol_svc_id, hs_svc_id;

	rc = loc_service_get_id(volume_name, &vol_svc_id, 0);
	if (rc != EOK)
		return rc;

	rc = loc_service_get_id(hotspare, &hs_svc_id, 0);
	if (rc != EOK)
		return rc;

	rc = hr_sess_init(&hr);
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
	hr_sess_destroy(hr);
	return rc;
}

errno_t hr_print_status(void)
{
	hr_t *hr;
	errno_t rc, retval;
	async_exch_t *exch;
	aid_t req;
	size_t size, i;
	hr_vol_info_t *vols = NULL;

	rc = hr_sess_init(&hr);
	if (rc != EOK)
		return rc;

	exch = async_exchange_begin(hr->sess);
	if (exch == NULL) {
		rc = EINVAL;
		goto error;
	}

	req = async_send_0(exch, HR_STATUS, NULL);
	rc = async_data_read_start(exch, &size, sizeof(size_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	vols = calloc(size, sizeof(hr_vol_info_t));
	if (vols == NULL) {
		async_exchange_end(exch);
		async_forget(req);
		return ENOMEM;
	}

	for (i = 0; i < size; i++) {
		rc = async_data_read_start(exch, &vols[i],
		    sizeof(hr_vol_info_t));
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

	if (size == 0) {
		printf("no active arrays\n");
		goto error;
	}

	for (i = 0; i < size; i++) {
		rc = print_vol_info(i, &vols[i]);
		if (rc != EOK)
			goto error;
	}

error:
	hr_sess_destroy(hr);
	if (vols != NULL)
		free(vols);
	return rc;
}

const char *hr_get_vol_status_msg(hr_vol_status_t status)
{
	switch (status) {
	case HR_VOL_NONE:
		return "NONE/UNKNOWN";
	case HR_VOL_ONLINE:
		return "ONLINE";
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

const char *hr_get_ext_status_msg(hr_ext_status_t status)
{
	switch (status) {
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

const char *hr_get_layout_str(hr_layout_t layout)
{
	switch (layout) {
	case HR_RLQ_NONE:
		return "RAID layout not set";
	case HR_RLQ_RAID4_0:
		return "RAID-4 Non-Rotating Parity 0";
	case HR_RLQ_RAID4_N:
		return "RAID-4 Non-Rotating Parity N";
	case HR_RLQ_RAID5_0R:
		return "RAID-5 Rotating Parity 0 with Data Restart";
	case HR_RLQ_RAID5_NR:
		return "RAID-5 Rotating Parity N with Data Restart";
	case HR_RLQ_RAID5_NC:
		return "RAID-5 Rotating Parity N with Data Continuation";
	default:
		return "Invalid RAID layout";
	}
}

const char *hr_get_metadata_type_str(hr_metadata_type_t type)
{
	switch (type) {
	case HR_METADATA_NATIVE:
		return "Native HelenRAID metadata";
	case HR_METADATA_GEOM_MIRROR:
		return "GEOM::MIRROR";
	case HR_METADATA_GEOM_STRIPE:
		return "GEOM::STRIPE";
	default:
		return "Invalid metadata type value";
	}
}

/** @}
 */
