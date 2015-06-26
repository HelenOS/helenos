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
	sysarg_t ltype;
	int retval;

	exch = async_exchange_begin(vbd->sess);
	retval = async_req_1_1(exch, VBD_DISK_INFO, sid, &ltype);
	async_exchange_end(exch);

	if (retval != EOK)
		return EIO;

	vinfo->ltype = (label_type_t)ltype;
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

/** @}
 */
