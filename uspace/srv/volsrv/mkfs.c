/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup volsrv
 * @{
 */
/**
 * @file Filesystem creation
 * @brief
 */

#include <errno.h>
#include <io/log.h>
#include <loc.h>
#include <stdarg.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <task.h>
#include <types/vol.h>

#include "mkfs.h"

static errno_t cmd_runl(const char *path, ...)
{
	va_list ap;
	const char *arg;
	int cnt = 0;

	va_start(ap, path);
	do {
		arg = va_arg(ap, const char *);
		cnt++;
	} while (arg != NULL);
	va_end(ap);

	va_start(ap, path);
	task_id_t id;
	task_wait_t wait;
	errno_t rc = task_spawn(&id, &wait, path, cnt, ap);
	va_end(ap);

	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error spawning %s (%s)",
		    path, str_error(rc));
		return rc;
	}

	if (!id) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error spawning %s "
		    "(invalid task ID)", path);
		return EINVAL;
	}

	task_exit_t texit;
	int retval;
	rc = task_wait(&wait, &texit, &retval);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error waiting for %s (%s)",
		    path, str_error(rc));
		return rc;
	}

	if (texit != TASK_EXIT_NORMAL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Command %s unexpectedly "
		    "terminated", path);
		return EINVAL;
	}

	if (retval != 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Command %s returned non-zero "
		    "exit code %d)", path, retval);
	}

	return retval == 0 ? EOK : EPARTY;
}

errno_t volsrv_part_mkfs(service_id_t sid, vol_fstype_t fstype, const char *label)
{
	const char *cmd;
	char *svc_name;
	errno_t rc;

	cmd = NULL;
	switch (fstype) {
	case fs_exfat:
		cmd = "/app/mkexfat";
		break;
	case fs_fat:
		cmd = "/app/mkfat";
		break;
	case fs_minix:
		cmd = "/app/mkmfs";
		break;
	case fs_ext4:
		cmd = "/app/mkext4";
		break;
	case fs_cdfs:
		cmd = NULL;
		break;
	}

	if (cmd == NULL)
		return ENOTSUP;

	rc = loc_service_get_name(sid, &svc_name);
	if (rc != EOK)
		return rc;

	if (str_size(label) > 0)
		rc = cmd_runl(cmd, cmd, "--label", label, svc_name, NULL);
	else
		rc = cmd_runl(cmd, cmd, svc_name, NULL);

	free(svc_name);
	return rc;
}

void volsrv_part_get_lsupp(vol_fstype_t fstype, vol_label_supp_t *vlsupp)
{
	vlsupp->supported = false;

	switch (fstype) {
	case fs_exfat:
	case fs_ext4:
	case fs_fat:
		vlsupp->supported = true;
		break;
	case fs_minix:
	case fs_cdfs:
		break;
	}
}

/** @}
 */
