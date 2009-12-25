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
#include <string.h>
#include <devmap.h>
#include "init.h"

#define DEVFS_MOUNT_POINT  "/dev"

#define SRV_CONSOLE  "/srv/console"
#define APP_GETTERM  "/app/getterm"

static void info_print(void)
{
	printf(NAME ": HelenOS init\n");
}

static bool mount_root(const char *fstype)
{
	char *opts = "";
	const char *root_dev = "bd/initrd";
	
	if (str_cmp(fstype, "tmpfs") == 0)
		opts = "restore";
	
	int rc = mount(fstype, "/", root_dev, opts, IPC_FLAG_BLOCKING);
	
	switch (rc) {
	case EOK:
		printf(NAME ": Root filesystem mounted, %s at %s\n",
		    fstype, root_dev);
		break;
	case EBUSY:
		printf(NAME ": Root filesystem already mounted\n");
		return false;
	case ELIMIT:
		printf(NAME ": Unable to mount root filesystem\n");
		return false;
	case ENOENT:
		printf(NAME ": Unknown filesystem type (%s)\n", fstype);
		return false;
	default:
		printf(NAME ": Error mounting root filesystem (%d)\n", rc);
		return false;
	}
	
	return true;
}

static bool mount_devfs(void)
{
	char null[MAX_DEVICE_NAME];
	int null_id = devmap_null_create();
	
	if (null_id == -1) {
		printf(NAME ": Unable to create null device\n");
		return false;
	}
	
	snprintf(null, MAX_DEVICE_NAME, "null/%d", null_id);
	int rc = mount("devfs", DEVFS_MOUNT_POINT, null, "", IPC_FLAG_BLOCKING);
	
	switch (rc) {
	case EOK:
		printf(NAME ": Device filesystem mounted\n");
		break;
	case EBUSY:
		printf(NAME ": Device filesystem already mounted\n");
		devmap_null_destroy(null_id);
		return false;
	case ELIMIT:
		printf(NAME ": Unable to mount device filesystem\n");
		devmap_null_destroy(null_id);
		return false;
	case ENOENT:
		printf(NAME ": Unknown filesystem type (devfs)\n");
		devmap_null_destroy(null_id);
		return false;
	default:
		printf(NAME ": Error mounting device filesystem (%d)\n", rc);
		devmap_null_destroy(null_id);
		return false;
	}
	
	return true;
}

static void spawn(char *fname)
{
	char *argv[2];
	struct stat s;
	
	if (stat(fname, &s) == ENOENT)
		return;
	
	printf(NAME ": Spawning %s\n", fname);
	
	argv[0] = fname;
	argv[1] = NULL;
	
	if (!task_spawn(fname, argv))
		printf(NAME ": Error spawning %s\n", fname);
}

static void srv_start(char *fname)
{
	char *argv[2];
	task_id_t id;
	task_exit_t texit;
	int rc, retval;
	struct stat s;
	
	if (stat(fname, &s) == ENOENT)
		return;
	
	printf(NAME ": Starting %s\n", fname);
	
	argv[0] = fname;
	argv[1] = NULL;
	
	id = task_spawn(fname, argv);
	if (!id) {
		printf(NAME ": Error spawning %s\n", fname);
		return;
	}

	rc = task_wait(id, &texit, &retval);
	if (rc != EOK) {
		printf(NAME ": Error waiting for %s\n", fname);
		return;
	}

	if ((texit != TASK_EXIT_NORMAL) || (retval != 0)) {
		printf(NAME ": Server %s failed to start (returned %d)\n",
			fname, retval);
	}
}

static void console(char *dev)
{
	char *argv[3];
	char hid_in[MAX_DEVICE_NAME];
	int rc;
	
	snprintf(hid_in, MAX_DEVICE_NAME, "%s/%s", DEVFS_MOUNT_POINT, dev);
	
	printf(NAME ": Spawning %s with %s\n", SRV_CONSOLE, hid_in);
	
	/* Wait for the input device to be ready */
	dev_handle_t handle;
	rc = devmap_device_get_handle(dev, &handle, IPC_FLAG_BLOCKING);
	
	if (rc == EOK) {
		argv[0] = SRV_CONSOLE;
		argv[1] = hid_in;
		argv[2] = NULL;
		
		if (!task_spawn(SRV_CONSOLE, argv))
			printf(NAME ": Error spawning %s with %s\n", SRV_CONSOLE, hid_in);
	} else
		printf(NAME ": Error waiting on %s\n", hid_in);
}

static void getterm(char *dev, char *app)
{
	char *argv[4];
	char term[MAX_DEVICE_NAME];
	int rc;
	
	snprintf(term, MAX_DEVICE_NAME, "%s/%s", DEVFS_MOUNT_POINT, dev);
	
	printf(NAME ": Spawning %s with %s %s\n", APP_GETTERM, term, app);
	
	/* Wait for the terminal device to be ready */
	dev_handle_t handle;
	rc = devmap_device_get_handle(dev, &handle, IPC_FLAG_BLOCKING);
	
	if (rc == EOK) {
		argv[0] = APP_GETTERM;
		argv[1] = term;
		argv[2] = app;
		argv[3] = NULL;
		
		if (!task_spawn(APP_GETTERM, argv))
			printf(NAME ": Error spawning %s with %s %s\n", APP_GETTERM,
			    term, app);
	} else
		printf(NAME ": Error waiting on %s\n", term);
}

static void mount_data(void)
{
	int rc;

	printf("Trying to mount bd/disk0 on /data... ");
	fflush(stdout);

	rc = mount("fat", "/data", "bd/disk0", "wtcache", 0);
	if (rc == EOK)
		printf("OK\n");
	else
		printf("Failed\n");
}

int main(int argc, char *argv[])
{
	info_print();
	
	if (!mount_root(STRING(RDFMT))) {
		printf(NAME ": Exiting\n");
		return -1;
	}
	
	spawn("/srv/devfs");
	
	if (!mount_devfs()) {
		printf(NAME ": Exiting\n");
		return -2;
	}
	
	spawn("/srv/fhc");
	spawn("/srv/obio");

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
