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
/** @file Virtual Block Device client API
 */

#include <errno.h>
#include <ipc/services.h>
#include <ipc/vbd.h>
#include <loc.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>
#include <types/label.h>
#include <vbd.h>

int vbd_create(vbd_t **rvbd)
{
	vbd_t *vbd;
	service_id_t vbd_svcid;
	int rc;

	vbd = calloc(1, sizeof(vbd_t));
	if (vbd == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = loc_service_get_id(SERVICE_NAME_VBD, &vbd_svcid,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	vbd->sess = loc_service_connect(EXCHANGE_SERIALIZE, vbd_svcid,
	    IPC_FLAG_BLOCKING);
	if (vbd->sess == NULL) {
		rc = EIO;
		goto error;
	}

	*rvbd = vbd;
	return EOK;
error:
	free(vbd);
	return rc;
}

void vbd_destroy(vbd_t *vbd)
{
	if (vbd == NULL)
		return;

	async_hangup(vbd->sess);
	free(vbd);
}

int vbd_disk_add(vbd_t *vbd, service_id_t disk_sid)
{
	async_exch_t *exch;

	exch = async_exchange_begin(vbd->sess);
	sysarg_t rc = async_req_1_0(exch, VBD_DISK_ADD, disk_sid);
	async_exchange_end(exch);

	return (int)rc;
}

int vbd_disk_remove(vbd_t *vbd, service_id_t disk_sid)
{
	async_exch_t *exch;

	exch = async_exchange_begin(vbd->sess);
	sysarg_t rc = async_req_1_0(exch, VBD_DISK_REMOVE, disk_sid);
	async_exchange_end(exch);

	return (int)rc;
}

/** Get disk information. */
int vbd_disk_info(vbd_t *vbd, service_id_t sid, vbd_disk_info_t *vinfo)
{
	async_exch_t *exch;
	sysarg_t retval;
	ipc_call_t answer;

	exch = async_exchange_begin(vbd->sess);
	aid_t req = async_send_1(exch, VBD_DISK_INFO, sid, &answer);
	int rc = async_data_read_start(exch, vinfo, sizeof(vbd_disk_info_t));
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

int vbd_label_create(vbd_t *vbd, service_id_t sid, label_type_t ltype)
{
	async_exch_t *exch;
	int retval;

	exch = async_exchange_begin(vbd->sess);
	retval = async_req_2_0(exch, VBD_LABEL_CREATE, sid, ltype);
	async_exchange_end(exch);

	if (retval != EOK)
		return EIO;

	return EOK;
}

int vbd_label_delete(vbd_t *vbd, service_id_t sid)
{
	async_exch_t *exch;
	int retval;

	exch = async_exchange_begin(vbd->sess);
	retval = async_req_1_0(exch, VBD_LABEL_DELETE, sid);
	async_exchange_end(exch);

	if (retval != EOK)
		return EIO;

	return EOK;
}

/** Get list of IDs into a buffer of fixed size.
 *
 * @param vbd      Virtual Block Device
 * @param method   IPC method
 * @param arg1     First argument
 * @param id_buf   Buffer to store IDs
 * @param buf_size Buffer size
 * @param act_size Place to store actual size of complete data.
 *
 * @return EOK on success or negative error code.
 */
static int vbd_get_ids_once(vbd_t *vbd, sysarg_t method, sysarg_t arg1,
    sysarg_t *id_buf, size_t buf_size, size_t *act_size)
{
	async_exch_t *exch = async_exchange_begin(vbd->sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, method, arg1, &answer);
	int rc = async_data_read_start(exch, id_buf, buf_size);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	sysarg_t retval;
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
 * @param vbd    Virtual Block Device
 * @param method IPC method
 * @param arg1   IPC argument 1
 * @param data   Place to store pointer to array of IDs
 * @param count  Place to store number of IDs
 * @return       EOK on success or negative error code
 */
static int vbd_get_ids_internal(vbd_t *vbd, sysarg_t method, sysarg_t arg1,
    sysarg_t **data, size_t *count)
{
	*data = NULL;
	*count = 0;

	size_t act_size = 0;
	int rc = vbd_get_ids_once(vbd, method, arg1, NULL, 0, &act_size);
	if (rc != EOK)
		return rc;

	size_t alloc_size = act_size;
	service_id_t *ids = malloc(alloc_size);
	if (ids == NULL)
		return ENOMEM;

	while (true) {
		rc = vbd_get_ids_once(vbd, method, arg1, ids, alloc_size,
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

/** Get list of disks as array of service IDs.
 *
 * @param vbd   Virtual Block Device
 * @param data  Place to store pointer to array
 * @param count Place to store length of array (number of entries)
 *
 * @return EOK on success or negative error code
 */
int vbd_label_get_parts(vbd_t *vbd, service_id_t disk,
    service_id_t **data, size_t *count)
{
	return vbd_get_ids_internal(vbd, VBD_LABEL_GET_PARTS, disk,
	    data, count);
}

int vbd_part_get_info(vbd_t *vbd, vbd_part_id_t part, vbd_part_info_t *pinfo)
{
	async_exch_t *exch;
	sysarg_t index;
	sysarg_t b0_lo, b0_hi;
	sysarg_t nb_lo, nb_hi;
	int retval;

	exch = async_exchange_begin(vbd->sess);
	retval = async_req_1_5(exch, VBD_PART_GET_INFO, part, &index,
	    &b0_lo, &b0_hi, &nb_lo, &nb_hi);
	async_exchange_end(exch);

	if (retval != EOK)
		return EIO;

	pinfo->index = index;
	pinfo->block0 = MERGE_LOUP32(b0_lo, b0_hi);
	pinfo->nblocks = MERGE_LOUP32(nb_lo, nb_hi);
	return EOK;
}

int vbd_part_create(vbd_t *vbd, service_id_t disk, vbd_part_spec_t *pspec,
    vbd_part_id_t *rpart)
{
	async_exch_t *exch;
	sysarg_t retval;
	ipc_call_t answer;

	exch = async_exchange_begin(vbd->sess);
	aid_t req = async_send_1(exch, VBD_PART_CREATE, disk, &answer);
	int rc = async_data_write_start(exch, pspec, sizeof(vbd_part_spec_t));
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return EIO;
	}

	async_wait_for(req, &retval);
	if (retval != EOK)
		return EIO;

	*rpart = (vbd_part_id_t)IPC_GET_ARG1(answer);
	return EOK;

}

int vbd_part_delete(vbd_t *vbd, vbd_part_id_t part)
{
	async_exch_t *exch;
	int retval;

	exch = async_exchange_begin(vbd->sess);
	retval = async_req_1_0(exch, VBD_PART_DELETE, part);
	async_exchange_end(exch);

	if (retval != EOK)
		return EIO;

	return EOK;
}

void vbd_pspec_init(vbd_part_spec_t *pspec)
{
	memset(pspec, 0, sizeof(vbd_part_spec_t));
}

/** @}
 */
