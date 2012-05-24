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
#include <vfs/vfs.h>
#include <bool.h>
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

#define DATA_FS_TYPE      "fat"
#define DATA_DEVICE       "bd/ata1disk0"
#define DATA_MOUNT_POINT  "/data"

#define SRV_CONSOLE  "/srv/console"
#define APP_GETTERM  "/app/getterm"

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

static void spawn(const char *fname)
{
	int rc;
	struct stat s;
	
	if (stat(fname, &s) == ENOENT)
		return;
	
	printf("%s: Spawning %s\n", NAME, fname);
	rc = task_spawnl(NULL, fname, fname, NULL);
	if (rc != EOK) {
		printf("%s: Error spawning %s (%s)\n", NAME, fname,
		    str_error(rc));
	}
}

static void srv_start(const char *fname)
{
	task_id_t id;
	task_exit_t texit;
	int rc, retval;
	struct stat s;
	
	if (stat(fname, &s) == ENOENT)
		return;
	
	printf("%s: Starting %s\n", NAME, fname);
	rc = task_spawnl(&id, fname, fname, NULL);
	if (!id) {
		printf("%s: Error spawning %s (%s)\n", NAME, fname,
		    str_error(rc));
		return;
	}
	
	rc = task_wait(id, &texit, &retval);
	if (rc != EOK) {
		printf("%s: Error waiting for %s (%s)\n", NAME, fname,
		    str_error(rc));
		return;
	}
	
	if (texit != TASK_EXIT_NORMAL) {
		printf("%s: Server %s failed to start (unexpectedly "
		    "terminated)\n", NAME, fname);
		return;
	}

	if (retval != 0) {
		printf("%s: Server %s failed to start (exit code %d)\n", NAME,
			fname, retval);
	}
}

static void console(const char *isvc, const char *fbsvc)
{
	printf("%s: Spawning %s %s %s\n", NAME, SRV_CONSOLE, isvc, fbsvc);
	
	/* Wait for the input service to be ready */
	service_id_t service_id;
	int rc = loc_service_get_id(isvc, &service_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Error waiting on %s (%s)\n", NAME, isvc,
		    str_error(rc));
		return;
	}
	
	/* Wait for the framebuffer service to be ready */
	rc = loc_service_get_id(fbsvc, &service_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Error waiting on %s (%s)\n", NAME, fbsvc,
		    str_error(rc));
		return;
	}
	
	rc = task_spawnl(NULL, SRV_CONSOLE, SRV_CONSOLE, isvc, fbsvc, NULL);
	if (rc != EOK) {
		printf("%s: Error spawning %s %s %s (%s)\n", NAME, SRV_CONSOLE,
		    isvc, fbsvc, str_error(rc));
	}
}

static void getterm(const char *svc, const char *app, bool wmsg)
{
	char term[LOC_NAME_MAXLEN];
	int rc;
	
	snprintf(term, LOC_NAME_MAXLEN, "%s/%s", LOCFS_MOUNT_POINT, svc);
	
	printf("%s: Spawning %s %s %s\n", NAME, APP_GETTERM, term, app);
	
	/* Wait for the terminal service to be ready */
	service_id_t service_id;
	rc = loc_service_get_id(svc, &service_id, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		printf("%s: Error waiting on %s (%s)\n", NAME, term,
		    str_error(rc));
		return;
	}
	
	if (wmsg) {
		rc = task_spawnl(NULL, APP_GETTERM, APP_GETTERM, "-w", term,
		    app, NULL);
		if (rc != EOK) {
			printf("%s: Error spawning %s -w %s %s (%s)\n", NAME,
			    APP_GETTERM, term, app, str_error(rc));
		}
	} else {
		rc = task_spawnl(NULL, APP_GETTERM, APP_GETTERM, term, app,
		    NULL);
		if (rc != EOK) {
			printf("%s: Error spawning %s %s %s (%s)\n", NAME,
			    APP_GETTERM, term, app, str_error(rc));
		}
	}
}

static bool mount_tmpfs(void)
{
	int rc = mount(TMPFS_FS_TYPE, TMPFS_MOUNT_POINT, "", "", 0, 0);
	return mount_report("Temporary filesystem", TMPFS_MOUNT_POINT,
	    TMPFS_FS_TYPE, NULL, rc);
}

static bool mount_data(void)
{
	int rc = mount(DATA_FS_TYPE, DATA_MOUNT_POINT, DATA_DEVICE, "wtcache", 0, 0);
	return mount_report("Data filesystem", DATA_MOUNT_POINT, DATA_FS_TYPE,
	    DATA_DEVICE, rc);
}

int main(int argc, char *argv[])
{
	info_print();
	
	if (!mount_root(STRING(RDFMT))) {
		printf("%s: Exiting\n", NAME);
		return -1;
	}
	
	/* Make sure tmpfs is running. */
	if (str_cmp(STRING(RDFMT), "tmpfs") != 0) {
		spawn("/srv/tmpfs");
	}
	
	spawn("/srv/locfs");
	spawn("/srv/taskmon");
	
	if (!mount_locfs()) {
		printf("%s: Exiting\n", NAME);
		return -2;
	}
	
	mount_tmpfs();
	
	spawn("/srv/devman");
	spawn("/srv/apic");
	spawn("/srv/i8259");
	spawn("/srv/obio");
	srv_start("/srv/cuda_adb");
	srv_start("/srv/s3c24xx_uart");
	srv_start("/srv/s3c24xx_ts");
	
	spawn("/srv/loopip");
	spawn("/srv/ethip");
	spawn("/srv/inetsrv");
	spawn("/srv/tcp");
	spawn("/srv/udp");
	
	spawn("/srv/fb");
	spawn("/srv/input");
	console("hid/input", "hid/fb0");
	
	spawn("/srv/clipboard");
	spawn("/srv/remcons");
	
	/*
	 * Start these synchronously so that mount_data() can be
	 * non-blocking.
	 */
#ifdef CONFIG_START_BD
	srv_start("/srv/ata_bd");
	srv_start("/srv/gxe_bd");
#else
	(void) srv_start;
#endif
	
#ifdef CONFIG_MOUNT_DATA
	/* Make sure fat is running. */
	if (str_cmp(STRING(RDFMT), "fat") != 0) {
		srv_start("/srv/fat");
	}
	mount_data();
#else
	(void) mount_data;
#endif
	
	getterm("term/vc0", "/app/bdsh", true);
	getterm("term/vc1", "/app/bdsh", false);
	getterm("term/vc2", "/app/bdsh", false);
	getterm("term/vc3", "/app/bdsh", false);
	getterm("term/vc4", "/app/bdsh", false);
	getterm("term/vc5", "/app/bdsh", false);
	getterm("term/vc6", "/app/klog", false);
	
	return 0;
}

/** @}
 */
