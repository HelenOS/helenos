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

	hr_t *hr = calloc(1, sizeof(hr_t));
	if (hr == NULL) {
		rc = ENOMEM;
		goto error;
	}

	service_id_t hr_svcid;

	rc = loc_service_get_id(SERVICE_NAME_HR, &hr_svcid, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	hr->sess = loc_service_connect(hr_svcid, INTERFACE_HR,
	    IPC_FLAG_BLOCKING);
	if (hr->sess == NULL) {
		rc = EIO;
		goto error;
	}

	*rhr = hr;
	return EOK;
error:
	if (hr != NULL)
		free(hr);
	*rhr = NULL;
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
	if (retval != EOK)
		return retval;

	return EOK;
}

errno_t hr_assemble(hr_t *hr, hr_config_t *hr_config)
{
	errno_t rc, retval;
	async_exch_t *exch;
	aid_t req;

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

	async_exchange_end(exch);
	async_wait_for(req, &retval);
	if (retval != EOK)
		return retval;

	return EOK;
}

static errno_t print_vol_info(size_t index, hr_vol_info_t *vol_info)
{
	errno_t rc;
	size_t i;
	char *devname;

	printf("--- vol %zu ---\n", index);

	printf("svc_id: %lu\n", vol_info->svc_id);

	rc = loc_service_get_name(vol_info->svc_id, &devname);
	if (rc != EOK)
		return rc;
	printf("devname: %s\n", devname);

	printf("level: %d\n", vol_info->level);
	if (vol_info->level == hr_l_0) {
		if (vol_info->strip_size / 1024 < 1)
			printf("strip size in bytes: %u\n",
			    vol_info->strip_size);
		else
			printf("strip size: %uK\n",
			    vol_info->strip_size / 1024);
	}
	printf("size in bytes: %luMiB\n",
	    vol_info->nblocks * vol_info->bsize / 1024 / 1024);
	printf("size in blocks: %lu\n", vol_info->nblocks);
	printf("block size: %zu\n", vol_info->bsize);

	printf("extents: [index] [devname]\n");
	for (i = 0; i < vol_info->extent_no; i++) {
		rc = loc_service_get_name(vol_info->extents[i], &devname);
		if (rc != EOK)
			return rc;
		printf("          %zu       %s\n", i, devname);
	}
	return EOK;
}

errno_t hr_print_status(void)
{
	hr_t *hr;
	errno_t rc, retval;
	async_exch_t *exch;
	aid_t req;
	size_t size, i;
	hr_vol_info_t *vols;

	rc = hr_sess_init(&hr);
	if (rc != EOK)
		return rc;

	exch = async_exchange_begin(hr->sess);
	if (exch == NULL)
		return EINVAL;

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

	for (i = 0; i < size; i++) {
		rc = print_vol_info(i, &vols[i]);
		if (rc != EOK)
			goto error;
	}

error:
	if (vols != NULL)
		free(vols);
	return rc;
}

/** @}
 */
