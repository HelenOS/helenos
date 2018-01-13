/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file Volume service API
 */

#include <abi/ipc/interfaces.h>
#include <errno.h>
#include <ipc/services.h>
#include <ipc/vol.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>
#include <vol.h>

/** Create Volume service session.
 *
 * @param rvol Place to store pointer to volume service session.
 * @return     EOK on success, ENOMEM if out of memory, EIO if service
 *             cannot be contacted.
 */
errno_t vol_create(vol_t **rvol)
{
	vol_t *vol;
	service_id_t vol_svcid;
	errno_t rc;

	vol = calloc(1, sizeof(vol_t));
	if (vol == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = loc_service_get_id(SERVICE_NAME_VOLSRV, &vol_svcid, 0);
	if (rc != EOK) {
		rc = ENOENT;
		goto error;
	}

	vol->sess = loc_service_connect(vol_svcid, INTERFACE_VOL, 0);
	if (vol->sess == NULL) {
		rc = EIO;
		goto error;
	}

	*rvol = vol;
	return EOK;
error:
	free(vol);
	return rc;
}

/** Destroy volume service session.
 *
 * @param vol Volume service session
 */
void vol_destroy(vol_t *vol)
{
	if (vol == NULL)
		return;

	async_hangup(vol->sess);
	free(vol);
}

/** Get list of IDs into a buffer of fixed size.
 *
 * @param vol      Volume service
 * @param method   IPC method
 * @param arg1     First argument
 * @param id_buf   Buffer to store IDs
 * @param buf_size Buffer size
 * @param act_size Place to store actual size of complete data.
 *
 * @return EOK on success or an error code.
 */
static errno_t vol_get_ids_once(vol_t *vol, sysarg_t method, sysarg_t arg1,
    sysarg_t *id_buf, size_t buf_size, size_t *act_size)
{
	async_exch_t *exch = async_exchange_begin(vol->sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, method, arg1, &answer);
	errno_t rc = async_data_read_start(exch, id_buf, buf_size);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK) {
		return retval;
	}

	*act_size = IPC_GET_ARG1(answer);
	return EOK;
}

/** Get list of IDs.
 *
 * Returns an allocated array of service IDs.
 *
 * @param vol    Volume service
 * @param method IPC method
 * @param arg1   IPC argument 1
 * @param data   Place to store pointer to array of IDs
 * @param count  Place to store number of IDs
 * @return       EOK on success or an error code
 */
static errno_t vol_get_ids_internal(vol_t *vol, sysarg_t method, sysarg_t arg1,
    sysarg_t **data, size_t *count)
{
	*data = NULL;
	*count = 0;

	size_t act_size = 0;
	errno_t rc = vol_get_ids_once(vol, method, arg1, NULL, 0, &act_size);
	if (rc != EOK)
		return rc;

	size_t alloc_size = act_size;
	service_id_t *ids = malloc(alloc_size);
	if (ids == NULL)
		return ENOMEM;

	while (true) {
		rc = vol_get_ids_once(vol, method, arg1, ids, alloc_size,
		    &act_size);
		if (rc != EOK)
			return rc;

		if (act_size <= alloc_size)
			break;

		alloc_size = act_size;
		ids = realloc(ids, alloc_size);
		if (ids == NULL)
			return ENOMEM;
	}

	*count = act_size / sizeof(service_id_t);
	*data = ids;
	return EOK;
}

/** Get list of partitions as array of service IDs.
 *
 * @param vol Volume service
 * @param data Place to store pointer to array
 * @param count Place to store length of array (number of entries)
 *
 * @return EOK on success or an error code
 */
errno_t vol_get_parts(vol_t *vol, service_id_t **data, size_t *count)
{
	return vol_get_ids_internal(vol, VOL_GET_PARTS, 0, data, count);
}

/** Add partition.
 *
 * After a partition is created (e.g. as a result of deleting a label
 * the dummy partition is created), it can take some (unknown) time
 * until it is discovered.
 */
errno_t vol_part_add(vol_t *vol, service_id_t sid)
{
	async_exch_t *exch;
	errno_t retval;

	exch = async_exchange_begin(vol->sess);
	retval = async_req_1_0(exch, VOL_PART_ADD, sid);
	async_exchange_end(exch);

	if (retval != EOK)
		return retval;

	return EOK;
}

/** Get partition information. */
errno_t vol_part_info(vol_t *vol, service_id_t sid, vol_part_info_t *vinfo)
{
	async_exch_t *exch;
	errno_t retval;
	ipc_call_t answer;

	exch = async_exchange_begin(vol->sess);
	aid_t req = async_send_1(exch, VOL_PART_INFO, sid, &answer);
	errno_t rc = async_data_read_start(exch, vinfo, sizeof(vol_part_info_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return EIO;
	}

	async_wait_for(req, &retval);
	if (retval != EOK)
		return EIO;

	return EOK;
}

/** Erase partition (to the extent where we will consider it not containing
 * a file system. */
errno_t vol_part_empty(vol_t *vol, service_id_t sid)
{
	async_exch_t *exch;
	errno_t retval;

	exch = async_exchange_begin(vol->sess);
	retval = async_req_1_0(exch, VOL_PART_EMPTY, sid);
	async_exchange_end(exch);

	if (retval != EOK)
		return retval;

	return EOK;
}

/** Get volume label support. */
errno_t vol_part_get_lsupp(vol_t *vol, vol_fstype_t fstype,
    vol_label_supp_t *vlsupp)
{
	async_exch_t *exch;
	errno_t retval;
	ipc_call_t answer;

	exch = async_exchange_begin(vol->sess);
	aid_t req = async_send_1(exch, VOL_PART_LSUPP, fstype, &answer);
	errno_t rc = async_data_read_start(exch, vlsupp, sizeof(vol_label_supp_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return EIO;
	}

	async_wait_for(req, &retval);
	if (retval != EOK)
		return EIO;

	return EOK;
}

/** Create file system. */
errno_t vol_part_mkfs(vol_t *vol, service_id_t sid, vol_fstype_t fstype,
    const char *label)
{
	async_exch_t *exch;
	ipc_call_t answer;
	errno_t retval;

	exch = async_exchange_begin(vol->sess);
	aid_t req = async_send_2(exch, VOL_PART_MKFS, sid, fstype, &answer);
	retval = async_data_write_start(exch, label, str_size(label));
	async_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);

	if (retval != EOK)
		return retval;

	return EOK;
}

/** @}
 */
