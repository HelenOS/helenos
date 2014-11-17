/*
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

/** @addtogroup init Init
 * @brief Init process for user space environment configuration.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <vfs/vfs.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <task.h>
#include <malloc.h>
#include <macros.h>
#include <str.h>
#include <loc.h>
#include <str_error.h>
#include "init.h"

#define ROOT_DEVICE       "bd/initrd"
#define ROOT_MOUNT_POINT  "/"

#define LOCFS_FS_TYPE      "locfs"
#define LOCFS_MOUNT_POINT  "/loc"

#define TMPFS_FS_TYPE      "tmpfs"
#define TMPFS_MOUNT_POINT  "/tmp"

#define SRV_CONSOLE  "/srv/console"
#define APP_GETTERM  "/app/getterm"

#define SRV_COMPOSITOR  "/srv/compositor"

#define HID_INPUT              "hid/input"
#define HID_OUTPUT             "hid/output"
#define HID_COMPOSITOR_SERVER  ":0"

#define srv_start(path, ...) \
	srv_startl(path, path, ##__VA_ARGS__, NULL)

/** Print banner */
static void info_print(void)
{
	printf("%s: HelenOS init\n", NAME);
}

/** Report mount operation success */
static bool mount_report(const char *desc, const char *mntpt,
    const char *fstype, const char *dev, int rc)
{
	switch (rc) {
	case EOK:
		if (dev != NULL)
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

/** Mount root filesystem
 *
 * The operation blocks until the root filesystem
 * server is ready for mounting.
 *
 * @param[in] fstype Root filesystem type.
 *
 * @return True on success.
 * @return False on failure.
 *
 */
static bool mount_root(const char *fstype)
{
	const char *opts = "";
	
	if (str_cmp(fstype, "tmpfs") == 0)
		opts = "restore";
	
	int rc = mount(fstype, ROOT_MOUNT_POINT, ROOT_DEVICE, opts,
	    IPC_FLAG_BLOCKING, 0);
	return mount_report("Root filesystem", ROOT_MOUNT_POINT, fstype,
	    ROOT_DEVICE, rc);
}

/** Mount locfs filesystem
 *
 * The operation blocks until the locfs filesystem
 * server is ready for mounting.
 *
 * @return True on success.
 * @return False on failure.
 *
 */
static bool mount_locfs(void)
{
	int rc = mount(LOCFS_FS_TYPE, LOCFS_MOUNT_POINT, "", "",
	    IPC_FLAG_BLOCKING, 0);
	return mount_report("Location service filesystem", LOCFS_MOUNT_POINT,
	    LOCFS_FS_TYPE, NULL, rc);
}

static int srv_startl(const char *path, ...)
{
	struct stat s;
	if (stat(path, &s) == ENOENT) {
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
	int rc = task_spawn(&id, &wait, path, cnt, ap);
	va_end(ap);
	
	if (rc != EOK) {
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
	
	return retval;
}

static int console(const char *isvc, const char *osvc)
{
	/* Wait for the input service to be ready */
	service_id_t service_id;
	int rc = loc_service_get_id(isvc, &service_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Error waiting on %s (%s)\n", NAME, isvc,
		    str_error(rc));
		return rc;
	}
	
	/* Wait for the output service to be ready */
	rc = loc_service_get_id(osvc, &service_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Error waiting on %s (%s)\n", NAME, osvc,
		    str_error(rc));
		return rc;
	}
	
	return srv_start(SRV_CONSOLE, isvc, osvc);
}

static int compositor(const char *isvc, const char *name)
{
	/* Wait for the input service to be ready */
	service_id_t service_id;
	int rc = loc_service_get_id(isvc, &service_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Error waiting on %s (%s)\n", NAME, isvc,
		    str_error(rc));
		return rc;
	}
	
	return srv_start(SRV_COMPOSITOR, isvc, name);
}

static int gui_start(const char *app, const char *srv_name)
{
	char winreg[50];
	snprintf(winreg, sizeof(winreg), "%s%s%s", "comp", srv_name, "/winreg");
	
	printf("%s: Spawning %s %s\n", NAME, app, winreg);
	
	task_id_t id;
	task_wait_t wait;
	int rc = task_spawnl(&id, &wait, app, app, winreg, NULL);
	if (rc != EOK) {
		printf("%s: Error spawning %s %s (%s)\n", NAME, app,
		    winreg, str_error(rc));
		return -1;
	}
	
	task_exit_t texit;
	int retval;
	rc = task_wait(&wait, &texit, &retval);
	if ((rc != EOK) || (texit != TASK_EXIT_NORMAL)) {
		printf("%s: Error retrieving retval from %s (%s)\n", NAME,
		    app, str_error(rc));
		return -1;
	}
	
	return retval;
}

static void getterm(const char *svc, const char *app, bool msg)
{
	if (msg) {
		printf("%s: Spawning %s %s %s --msg --wait -- %s\n", NAME,
		    APP_GETTERM, svc, LOCFS_MOUNT_POINT, app);
		
		int rc = task_spawnl(NULL, NULL, APP_GETTERM, APP_GETTERM, svc,
		    LOCFS_MOUNT_POINT, "--msg", "--wait", "--", app, NULL);
		if (rc != EOK)
			printf("%s: Error spawning %s %s %s --msg --wait -- %s\n",
			    NAME, APP_GETTERM, svc, LOCFS_MOUNT_POINT, app);
	} else {
		printf("%s: Spawning %s %s %s --wait -- %s\n", NAME,
		    APP_GETTERM, svc, LOCFS_MOUNT_POINT, app);
		
		int rc = task_spawnl(NULL, NULL, APP_GETTERM, APP_GETTERM, svc,
		    LOCFS_MOUNT_POINT, "--wait", "--", app, NULL);
		if (rc != EOK)
			printf("%s: Error spawning %s %s %s --wait -- %s\n",
			    NAME, APP_GETTERM, svc, LOCFS_MOUNT_POINT, app);
	}
}

static bool mount_tmpfs(void)
{
	int rc = mount(TMPFS_FS_TYPE, TMPFS_MOUNT_POINT, "", "", 0, 0);
	return mount_report("Temporary filesystem", TMPFS_MOUNT_POINT,
	    TMPFS_FS_TYPE, NULL, rc);
}

int main(int argc, char *argv[])
{
	info_print();
	
	if (!mount_root(STRING(RDFMT))) {
		printf("%s: Exiting\n", NAME);
		return 1;
	}
	
	/* Make sure tmpfs is running. */
	if (str_cmp(STRING(RDFMT), "tmpfs") != 0)
		srv_start("/srv/tmpfs");
	
	srv_start("/srv/klog");
	srv_start("/srv/locfs");
	srv_start("/srv/taskmon");
	
	if (!mount_locfs()) {
		printf("%s: Exiting\n", NAME);
		return 2;
	}
	
	mount_tmpfs();
	
	srv_start("/srv/devman");
	srv_start("/srv/apic");
	srv_start("/srv/i8259");
	srv_start("/srv/icp-ic");
	srv_start("/srv/obio");
	srv_start("/srv/cuda_adb");
	srv_start("/srv/s3c24xx_uart");
	srv_start("/srv/s3c24xx_ts");
	
	srv_start("/srv/loopip");
	srv_start("/srv/ethip");
	srv_start("/srv/inetsrv");
	srv_start("/srv/tcp");
	srv_start("/srv/udp");
	srv_start("/srv/dnsrsrv");
	srv_start("/srv/dhcp");
	srv_start("/srv/nconfsrv");
	
	srv_start("/srv/clipboard");
	srv_start("/srv/remcons");
	
	srv_start("/srv/input", HID_INPUT);
	srv_start("/srv/output", HID_OUTPUT);
	srv_start("/srv/hound");
	
	int rc = compositor(HID_INPUT, HID_COMPOSITOR_SERVER);
	if (rc == EOK) {
		gui_start("/app/barber", HID_COMPOSITOR_SERVER);
		gui_start("/app/vlaunch", HID_COMPOSITOR_SERVER);
		gui_start("/app/vterm", HID_COMPOSITOR_SERVER);
	}
	
	rc = console(HID_INPUT, HID_OUTPUT);
	if (rc == EOK) {
		getterm("term/vc0", "/app/bdsh", true);
		getterm("term/vc1", "/app/bdsh", false);
		getterm("term/vc2", "/app/bdsh", false);
		getterm("term/vc3", "/app/bdsh", false);
		getterm("term/vc4", "/app/bdsh", false);
		getterm("term/vc5", "/app/bdsh", false);
	}
	
	return 0;
}

/** @}
 */
