/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup init
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdarg.h>
#include <vfs/vfs.h>
#include <stdbool.h>
#include <errno.h>
#include <task.h>
#include <stdlib.h>
#include <macros.h>
#include <str.h>
#include <loc.h>
#include <str_error.h>
#include <io/logctl.h>
#include <vfs/vfs.h>
#include "untar.h"
#include "init.h"

#define BANNER_LEFT   "######> "
#define BANNER_RIGHT  " <######"

#define ROOT_DEVICE       "bd/initrd"
#define ROOT_MOUNT_POINT  "/"

#define srv_start(path, ...) \
	srv_startl(path, path, ##__VA_ARGS__, NULL)

/** Print banner */
static void info_print(void)
{
	printf("%s: HelenOS init\n", NAME);
}

static void oom_check(errno_t rc, const char *path)
{
	if (rc == ENOMEM) {
		printf("%sOut-of-memory condition detected%s\n", BANNER_LEFT,
		    BANNER_RIGHT);
		printf("%sBailing out of the boot process after %s%s\n",
		    BANNER_LEFT, path, BANNER_RIGHT);
		printf("%sMore physical memory is required%s\n", BANNER_LEFT,
		    BANNER_RIGHT);
		exit(ENOMEM);
	}
}

/** Report mount operation success */
static bool mount_report(const char *desc, const char *mntpt,
    const char *fstype, const char *dev, errno_t rc)
{
	switch (rc) {
	case EOK:
		if ((dev != NULL) && (str_cmp(dev, "") != 0))
			printf("%s: %s mounted on %s (%s at %s)\n", NAME, desc, mntpt,
			    fstype, dev);
		else
			printf("%s: %s mounted on %s (%s)\n", NAME, desc, mntpt, fstype);
		break;
	case EBUSY:
		printf("%s: %s already mounted on %s\n", NAME, desc, mntpt);
		return false;
	case ELIMIT:
		printf("%s: %s limit exceeded\n", NAME, desc);
		return false;
	case ENOENT:
		printf("%s: %s unknown type (%s)\n", NAME, desc, fstype);
		return false;
	default:
		printf("%s: %s not mounted on %s (%s)\n", NAME, desc, mntpt,
		    str_error(rc));
		return false;
	}

	return true;
}

/** Mount root file system
 *
 * The operation blocks until the root file system
 * server is ready for mounting.
 *
 * @param[in] fstype Root file system type.
 *
 * @return True on success.
 * @return False on failure.
 *
 */
static bool mount_root(const char *fstype)
{
	const char *root_device = "";

	if (str_cmp(fstype, "tmpfs") != 0)
		root_device = ROOT_DEVICE;

	errno_t rc = vfs_mount_path(ROOT_MOUNT_POINT, fstype, root_device, "",
	    IPC_FLAG_BLOCKING, 0);
	if (rc == EOK)
		logctl_set_root();

	bool ret = mount_report("Root file system", ROOT_MOUNT_POINT, fstype,
	    root_device, rc);

	rc = vfs_cwd_set(ROOT_MOUNT_POINT);
	if (rc != EOK) {
		printf("%s: Unable to set current directory to %s (%s)\n",
		    NAME, ROOT_MOUNT_POINT, str_error(ret));
		return false;
	}

	if ((ret) && (str_cmp(fstype, "tmpfs") == 0)) {
		printf("%s: Extracting root file system archive\n", NAME);
		ret = bd_untar(ROOT_DEVICE);
	}

	return ret;
}

static errno_t srv_startl(const char *path, ...)
{
	vfs_stat_t s;
	if (vfs_stat_path(path, &s) != EOK) {
		printf("%s: Unable to stat %s\n", NAME, path);
		return ENOENT;
	}

	printf("%s: Starting %s\n", NAME, path);

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
		oom_check(rc, path);
		printf("%s: Error spawning %s (%s)\n", NAME, path,
		    str_error(rc));
		return rc;
	}

	if (!id) {
		printf("%s: Error spawning %s (invalid task id)\n", NAME,
		    path);
		return EINVAL;
	}

	task_exit_t texit;
	int retval;
	rc = task_wait(&wait, &texit, &retval);
	if (rc != EOK) {
		printf("%s: Error waiting for %s (%s)\n", NAME, path,
		    str_error(rc));
		return rc;
	}

	if (texit != TASK_EXIT_NORMAL) {
		printf("%s: Server %s failed to start (unexpectedly "
		    "terminated)\n", NAME, path);
		return EINVAL;
	}

	if (retval != 0)
		printf("%s: Server %s failed to start (exit code %d)\n", NAME,
		    path, retval);

	return retval == 0 ? EOK : EPARTY;
}

int main(int argc, char *argv[])
{
	info_print();

	if (!mount_root(STRING(RDFMT))) {
		printf("%s: Exiting\n", NAME);
		return 1;
	}

	/* System server takes over once root is mounted */
	srv_start("/srv/system");
	return 0;
}

/** @}
 */
