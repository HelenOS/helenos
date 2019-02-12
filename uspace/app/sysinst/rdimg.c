/*
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup sysinst
 * @{
 */
/** @file RAM disk image manipulation
 */

#include <errno.h>
#include <fibril.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <task.h>
#include <vfs/vfs.h>
#include <vol.h>

#include "rdimg.h"

#define FILE_BD "/srv/bd/file_bd"
#define RD_SVC "bd/iird"
#define RD_LABEL "HelenOS-rd"

/** Find volume by volume label. */
static errno_t rd_img_part_by_label(vol_t *vol, const char *label,
    service_id_t *rid)
{
	vol_part_info_t vinfo;
	service_id_t *part_ids = NULL;
	size_t nparts;
	size_t i;
	errno_t rc;

	rc = vol_get_parts(vol, &part_ids, &nparts);
	if (rc != EOK) {
		printf("Error getting list of volumes.\n");
		goto out;
	}

	for (i = 0; i < nparts; i++) {
		rc = vol_part_info(vol, part_ids[i], &vinfo);
		if (rc != EOK) {
			printf("Error getting volume information.\n");
			rc = EIO;
			goto out;
		}

		if (str_cmp(vinfo.label, label) == 0) {
			*rid = part_ids[i];
			rc = EOK;
			goto out;
		}
	}

	rc = ENOENT;
out:
	free(part_ids);
	return rc;
}

/** Open RAM disk image.
 *
 * @param imgpath Image path
 * @param rpath Place to store pointer to newly allocated string, the path to
 *              the mounted RAM disk.
 * @param rimg Place to store pointer to newly allocated object.
 *
 * @return EOK on success or an error code
 */
errno_t rd_img_open(const char *imgpath, char **rpath, rd_img_t **rimg)
{
	rd_img_t *img = NULL;
	char *rdpath = NULL;
	errno_t rc;
	task_id_t id;
	task_wait_t wait;
	task_exit_t texit;
	vfs_stat_t stat;
	int retval;
	int cnt;

	printf("rd_img_open: begin\n");
	rdpath = str_dup("/vol/" RD_LABEL);
	if (rdpath == NULL) {
		rc = ENOMEM;
		goto error;
	}

	img = calloc(1, sizeof(rd_img_t));
	if (img == NULL) {
		rc = ENOMEM;
		goto error;
	}

	printf("rd_img_open: spawn file_bd\n");
	rc = task_spawnl(&id, &wait, FILE_BD, FILE_BD, imgpath, RD_SVC, NULL);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	printf("rd_img_open: wait for file_bd\n");
	rc = task_wait(&wait, &texit, &retval);
	if (rc != EOK || texit != TASK_EXIT_NORMAL) {
		rc = EIO;
		goto error;
	}

	/* Wait for the RAM disk to become available */
	printf("rd_img_open: wait for RAM disk to be mounted\n");
	cnt = 10;
	while (cnt > 0) {
		rc = vfs_stat_path(rdpath, &stat);
		if (rc == EOK)
			break;

		fibril_sleep(1);
		--cnt;
	}

	if (cnt == 0) {
		printf("rd_img_open: ran out of time\n");
		rc = EIO;
		goto error;
	}

	img->filebd_tid = id;
	*rimg = img;
	*rpath = rdpath;
	printf("rd_img_open: success\n");
	return EOK;
error:
	if (rdpath != NULL)
		free(rdpath);
	if (img != NULL)
		free(img);
	return rc;
}

/** Close RAM disk image.
 *
 */
errno_t rd_img_close(rd_img_t *img)
{
	errno_t rc;
	service_id_t rd_svcid;
	vol_t *vol = NULL;

	printf("rd_img_close: begin\n");

	rc = vol_create(&vol);
	if (rc != EOK) {
		printf("Error opening volume management service.\n");
		rc = EIO;
		goto error;
	}

	printf("rd_img_close: Find RAM disk volume.\n");
	rc = rd_img_part_by_label(vol, RD_LABEL, &rd_svcid);
	if (rc != EOK) {
		printf("Error getting RAM disk service ID.\n");
		rc = EIO;
		goto error;
	}

	printf("rd_img_close: eject RAM disk volume\n");
	rc = vol_part_eject(vol, rd_svcid);
	if (rc != EOK) {
		printf("Error ejecting RAM disk volume.\n");
		rc = EIO;
		goto error;
	}

	vol_destroy(vol);

	rc = task_kill(img->filebd_tid);
	if (rc != EOK) {
		printf("Error killing file_bd.\n");
		rc = EIO;
		goto error;
	}

	free(img);
	printf("rd_img_close: success\n");
	return EOK;
error:
	free(img);
	if (vol != NULL)
		vol_destroy(vol);
	return rc;
}

/** @}
 */
