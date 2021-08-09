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

/** @addtogroup libdevice
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
#include <vfs/vfs.h>
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

	*act_size = ipc_get_arg1(&answer);
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
		service_id_t *temp = realloc(ids, alloc_size);
		if (temp == NULL) {
			free(ids);
			return ENOMEM;
		}
		ids = temp;
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
 *
 * @param vol Volume service
 * @param sid Service ID of the partition
 * @return EOK on success or an error code
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

/** Get partition information.
 *
 * @param vol Volume service
 * @param sid Service ID of the partition
 * @param vinfo Place to store partition information
 * @return EOK on success or an error code
 */
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

/** Unmount partition (and possibly eject the media).
 *
 * @param vol Volume service
 * @param sid Service ID of the partition
 * @return EOK on success or an error code
 */
errno_t vol_part_eject(vol_t *vol, service_id_t sid)
{
	async_exch_t *exch;
	errno_t retval;

	exch = async_exchange_begin(vol->sess);
	retval = async_req_1_0(exch, VOL_PART_EJECT, sid);
	async_exchange_end(exch);

	if (retval != EOK)
		return retval;

	return EOK;
}

/** Erase partition (to the extent where we will consider it not containing
 * a file system.
 *
 * @param vol Volume service
 * @param sid Service ID of the partition
 * @return EOK on success or an error code
 */
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

/** Insert volume.
 *
 * This will re-mount the volume if it has been ejected previously.
 *
 * @param vol Volume service
 * @param sid Service ID of the partition
 * @return EOK on success or an error code
 */
errno_t vol_part_insert(vol_t *vol, service_id_t sid)
{
	async_exch_t *exch;
	errno_t retval;

	exch = async_exchange_begin(vol->sess);
	retval = async_req_1_0(exch, VOL_PART_INSERT, sid);
	async_exchange_end(exch);

	if (retval != EOK)
		return retval;

	return EOK;
}

/** Insert volume by path.
 *
 * @param vol Volume service
 * @param path Filesystem path
 *
 * @return EOK on success or an error code
 */
errno_t vol_part_insert_by_path(vol_t *vol, const char *path)
{
	async_exch_t *exch;
	ipc_call_t answer;
	errno_t retval;

	exch = async_exchange_begin(vol->sess);
	aid_t req = async_send_0(exch, VOL_PART_INSERT_BY_PATH, &answer);

	retval = async_data_write_start(exch, path, str_size(path));
	if (retval != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return retval;
	}

	async_exchange_end(exch);
	async_wait_for(req, &retval);

	if (retval != EOK)
		return retval;

	return EOK;
}

/** Get volume label support.
 *
 * @param vol Volume service
 * @param fstype File system type
 * @param vlsupp Place to store volume label support information
 * @return EOK on success or an error code
 */
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

/** Create file system.
 *
 * @param vol Volume service
 * @param sid Partition service ID
 * @param fstype File system type
 * @param label Volume label
 * @param mountp Mount point
 *
 * @return EOK on success or an error code
 */
errno_t vol_part_mkfs(vol_t *vol, service_id_t sid, vol_fstype_t fstype,
    const char *label, const char *mountp)
{
	async_exch_t *exch;
	ipc_call_t answer;
	errno_t retval;

	exch = async_exchange_begin(vol->sess);
	aid_t req = async_send_2(exch, VOL_PART_MKFS, sid, fstype, &answer);

	retval = async_data_write_start(exch, label, str_size(label));
	if (retval != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return retval;
	}

	retval = async_data_write_start(exch, mountp, str_size(mountp));
	if (retval != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return retval;
	}

	async_exchange_end(exch);
	async_wait_for(req, &retval);

	if (retval != EOK)
		return retval;

	return EOK;
}

/** Set mount point for partition.
 *
 * @param vol Volume service
 * @param sid Partition service ID
 * @param mountp Mount point
 *
 * @return EOK on success or an error code
 */
errno_t vol_part_set_mountp(vol_t *vol, service_id_t sid,
    const char *mountp)
{
	async_exch_t *exch;
	ipc_call_t answer;
	errno_t retval;

	exch = async_exchange_begin(vol->sess);
	aid_t req = async_send_1(exch, VOL_PART_SET_MOUNTP, sid,
	    &answer);

	retval = async_data_write_start(exch, mountp, str_size(mountp));
	if (retval != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return retval;
	}

	async_exchange_end(exch);
	async_wait_for(req, &retval);

	if (retval != EOK)
		return retval;

	return EOK;
}

/** Format file system type as string.
 *
 * @param fstype File system type
 * @param rstr Place to store pointer to newly allocated string
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t vol_fstype_format(vol_fstype_t fstype, char **rstr)
{
	const char *sfstype;
	char *s;

	sfstype = NULL;
	switch (fstype) {
	case fs_exfat:
		sfstype = "ExFAT";
		break;
	case fs_fat:
		sfstype = "FAT";
		break;
	case fs_minix:
		sfstype = "MINIX";
		break;
	case fs_ext4:
		sfstype = "Ext4";
		break;
	case fs_cdfs:
		sfstype = "ISO 9660";
		break;
	}

	s = str_dup(sfstype);
	if (s == NULL)
		return ENOMEM;

	*rstr = s;
	return EOK;
}

/** Format partition content / file system type as string.
 *
 * @param pcnt Partition content
 * @param fstype File system type
 * @param rstr Place to store pointer to newly allocated string
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t vol_pcnt_fs_format(vol_part_cnt_t pcnt, vol_fstype_t fstype,
    char **rstr)
{
	int rc;
	char *s = NULL;

	switch (pcnt) {
	case vpc_empty:
		s = str_dup("Empty");
		if (s == NULL)
			return ENOMEM;
		break;
	case vpc_fs:
		rc = vol_fstype_format(fstype, &s);
		if (rc != EOK)
			return ENOMEM;
		break;
	case vpc_unknown:
		s = str_dup("Unknown");
		if (s == NULL)
			return ENOMEM;
		break;
	}

	assert(s != NULL);
	*rstr = s;
	return EOK;
}

/** Validate mount point.
 *
 * Verify that mount point is valid. A valid mount point is
 * one of:
 *  - 'Auto'
 *  - 'None'
 *  - /path (string beginning with '/') to an existing directory
 *
 * @return EOK if mount point is in valid, EINVAL if the format is invalid,
 *         ENOENT if the directory does not exist
 */
errno_t vol_mountp_validate(const char *mountp)
{
	errno_t rc;
	vfs_stat_t stat;

	if (str_cmp(mountp, "Auto") == 0 || str_cmp(mountp, "auto") == 0)
		return EOK;

	if (str_casecmp(mountp, "None") == 0 || str_cmp(mountp, "none") == 0)
		return EOK;

	if (mountp[0] == '/') {
		rc = vfs_stat_path(mountp, &stat);
		if (rc != EOK || !stat.is_directory)
			return ENOENT;

		return EOK;
	}

	return EINVAL;
}

/** Get list of volumes as array of volume IDs.
 *
 * @param vol Volume service
 * @param data Place to store pointer to array
 * @param count Place to store length of array (number of entries)
 *
 * @return EOK on success or an error code
 */
errno_t vol_get_volumes(vol_t *vol, volume_id_t **data, size_t *count)
{
	return vol_get_ids_internal(vol, VOL_GET_VOLUMES, 0,
	    (sysarg_t **) data, count);
}

/** Get volume configuration information.
 *
 * @param vol Volume service
 * @param vid Volume ID
 * @param vinfo Place to store volume configuration information
 * @return EOK on success or an error code
 */
errno_t vol_info(vol_t *vol, volume_id_t vid, vol_info_t *vinfo)
{
	async_exch_t *exch;
	errno_t retval;
	ipc_call_t answer;

	exch = async_exchange_begin(vol->sess);
	aid_t req = async_send_1(exch, VOL_INFO, vid.id, &answer);

	errno_t rc = async_data_read_start(exch, vinfo, sizeof(vol_info_t));
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

/** @}
 */
