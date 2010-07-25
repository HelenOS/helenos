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
#include <ipc/ipc.h>
#include <vfs/vfs.h>
#include <bool.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <task.h>
#include <malloc.h>
#include <macros.h>
#include <str.h>
#include <devmap.h>
#include <str_error.h>
#include "init.h"

#define ROOT_DEVICE       "bd/initrd"
#define ROOT_MOUNT_POINT  "/"

#define DEVFS_FS_TYPE      "devfs"
#define DEVFS_MOUNT_POINT  "/dev"

#define SCRATCH_FS_TYPE      "tmpfs"
#define SCRATCH_MOUNT_POINT  "/scratch"

#define DATA_FS_TYPE      "fat"
#define DATA_DEVICE       "bd/disk0"
#define DATA_MOUNT_POINT  "/data"

#define SRV_CONSOLE  "/srv/console"
#define APP_GETTERM  "/app/getterm"

static void info_print(void)
{
	printf("%s: HelenOS init\n", NAME);
}

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

static bool mount_root(const char *fstype)
{
	const char *opts = "";
	
	if (str_cmp(fstype, "tmpfs") == 0)
		opts = "restore";
	
	int rc = mount(fstype, ROOT_MOUNT_POINT, ROOT_DEVICE, opts,
	    IPC_FLAG_BLOCKING);
	return mount_report("Root filesystem", ROOT_MOUNT_POINT, fstype,
	    ROOT_DEVICE, rc);
}

static bool mount_devfs(void)
{
	int rc = mount(DEVFS_FS_TYPE, DEVFS_MOUNT_POINT, "", "",
	    IPC_FLAG_BLOCKING);
	return mount_report("Device filesystem", DEVFS_MOUNT_POINT, DEVFS_FS_TYPE,
	    NULL, rc);
}

static void spawn(const char *fname)
{
	const char *argv[2];
	struct stat s;
	
	if (stat(fname, &s) == ENOENT)
		return;
	
	printf("%s: Spawning %s\n", NAME, fname);
	
	argv[0] = fname;
	argv[1] = NULL;
	
	int err;
	if (!task_spawn(fname, argv, &err))
		printf("%s: Error spawning %s (%s)\n", NAME, fname,
		    str_error(err));
}

static void srv_start(const char *fname)
{
	const char *argv[2];
	task_id_t id;
	task_exit_t texit;
	int rc, retval;
	struct stat s;
	
	if (stat(fname, &s) == ENOENT)
		return;
	
	printf("%s: Starting %s\n", NAME, fname);
	
	argv[0] = fname;
	argv[1] = NULL;
	
	id = task_spawn(fname, argv, &retval);
	if (!id) {
		printf("%s: Error spawning %s (%s)\n", NAME, fname,
		    str_error(retval));
		return;
	}
	
	rc = task_wait(id, &texit, &retval);
	if (rc != EOK) {
		printf("%s: Error waiting for %s (%s(\n", NAME, fname,
		    str_error(retval));
		return;
	}
	
	if ((texit != TASK_EXIT_NORMAL) || (retval != 0)) {
		printf("%s: Server %s failed to start (%s)\n", NAME,
			fname, str_error(retval));
	}
}

static void console(const char *dev)
{
	const char *argv[3];
	char hid_in[DEVMAP_NAME_MAXLEN];
	int rc;
	
	snprintf(hid_in, DEVMAP_NAME_MAXLEN, "%s/%s", DEVFS_MOUNT_POINT, dev);
	
	printf("%s: Spawning %s %s\n", NAME, SRV_CONSOLE, hid_in);
	
	/* Wait for the input device to be ready */
	dev_handle_t handle;
	rc = devmap_device_get_handle(dev, &handle, IPC_FLAG_BLOCKING);
	
	if (rc == EOK) {
		argv[0] = SRV_CONSOLE;
		argv[1] = hid_in;
		argv[2] = NULL;
		
		if (!task_spawn(SRV_CONSOLE, argv, &rc))
			printf("%s: Error spawning %s %s (%s)\n", NAME, SRV_CONSOLE,
			    hid_in, str_error(rc));
	} else
		printf("%s: Error waiting on %s (%s)\n", NAME, hid_in,
		    str_error(rc));
}

static void getterm(const char *dev, const char *app)
{
	const char *argv[4];
	char term[DEVMAP_NAME_MAXLEN];
	int rc;
	
	snprintf(term, DEVMAP_NAME_MAXLEN, "%s/%s", DEVFS_MOUNT_POINT, dev);
	
	printf("%s: Spawning %s %s %s\n", NAME, APP_GETTERM, term, app);
	
	/* Wait for the terminal device to be ready */
	dev_handle_t handle;
	rc = devmap_device_get_handle(dev, &handle, IPC_FLAG_BLOCKING);
	
	if (rc == EOK) {
		argv[0] = APP_GETTERM;
		argv[1] = term;
		argv[2] = app;
		argv[3] = NULL;
		
		if (!task_spawn(APP_GETTERM, argv, &rc))
			printf("%s: Error spawning %s %s %s (%s)\n", NAME, APP_GETTERM,
			    term, app, str_error(rc));
	} else
		printf("%s: Error waiting on %s (%s)\n", NAME, term,
		    str_error(rc));
}

static bool mount_scratch(void)
{
	int rc = mount(SCRATCH_FS_TYPE, SCRATCH_MOUNT_POINT, "", "", 0);
	return mount_report("Scratch filesystem", SCRATCH_MOUNT_POINT,
	    SCRATCH_FS_TYPE, NULL, rc);
}

static bool mount_data(void)
{
	int rc = mount(DATA_FS_TYPE, DATA_MOUNT_POINT, DATA_DEVICE, "wtcache", 0);
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
	
	spawn("/srv/devfs");
	spawn("/srv/taskmon");
	
	if (!mount_devfs()) {
		printf("%s: Exiting\n", NAME);
		return -2;
	}
	
	mount_scratch();
	
	spawn("/srv/fhc");
	spawn("/srv/obio");
	srv_start("/srv/cuda_adb");
	srv_start("/srv/i8042");
	srv_start("/srv/adb_ms");
	srv_start("/srv/char_ms");
	
	spawn("/srv/fb");
	spawn("/srv/kbd");
	console("hid_in/kbd");
	
	spawn("/srv/clip");
	
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
	mount_data();
#else
	(void) mount_data;
#endif
	
	getterm("term/vc0", "/app/bdsh");
	getterm("term/vc1", "/app/bdsh");
	getterm("term/vc2", "/app/bdsh");
	getterm("term/vc3", "/app/bdsh");
	getterm("term/vc4", "/app/bdsh");
	getterm("term/vc5", "/app/bdsh");
	getterm("term/vc6", "/app/klog");
	
	return 0;
}

/** @}
 */
