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
#include <task.h>
#include <malloc.h>
#include <macros.h>
#include <string.h>
#include <devmap.h>
#include "init.h"

static void info_print(void)
{
	printf(NAME ": HelenOS init\n");
}

static bool mount_root(const char *fstype)
{
	int rc = -1;
	char *opts = "";
	const char *root_dev = "initrd";
	
	if (str_cmp(fstype, "tmpfs") == 0)
		opts = "restore";

	while (rc < 0) {
		rc = mount(fstype, "/", root_dev, opts, IPC_FLAG_BLOCKING);
		
		switch (rc) {
		case EOK:
			printf(NAME ": Root filesystem mounted, %s at %s\n",
			    fstype, root_dev);
			break;
		case EBUSY:
			printf(NAME ": Root filesystem already mounted\n");
			break;
		case ELIMIT:
			printf(NAME ": Unable to mount root filesystem\n");
			return false;
		case ENOENT:
			printf(NAME ": Unknown filesystem type (%s)\n", fstype);
			return false;
		}
	}
	
	return true;
}

static bool mount_devfs(void)
{
	int rc = -1;
	
	while (rc < 0) {
		rc = mount("devfs", "/dev", "null", "", IPC_FLAG_BLOCKING);
		
		switch (rc) {
		case EOK:
			printf(NAME ": Device filesystem mounted\n");
			break;
		case EBUSY:
			printf(NAME ": Device filesystem already mounted\n");
			break;
		case ELIMIT:
			printf(NAME ": Unable to mount device filesystem\n");
			return false;
		case ENOENT:
			printf(NAME ": Unknown filesystem type (devfs)\n");
			return false;
		}
	}
	
	return true;
}

static void spawn(char *fname)
{
	char *argv[2];
	
	printf(NAME ": Spawning %s\n", fname);
	
	argv[0] = fname;
	argv[1] = NULL;
	
	if (!task_spawn(fname, argv))
		printf(NAME ": Error spawning %s\n", fname);
}

static void getvc(char *dev, char *app)
{
	char *argv[4];
	char vc[MAX_DEVICE_NAME];
	int rc;
	
	snprintf(vc, MAX_DEVICE_NAME, "/dev/%s", dev);
	
	printf(NAME ": Spawning getvc on %s\n", vc);
	
	dev_handle_t handle;
	rc = devmap_device_get_handle(dev, &handle, IPC_FLAG_BLOCKING);
	
	if (rc == EOK) {
		argv[0] = "/app/getvc";
		argv[1] = vc;
		argv[2] = app;
		argv[3] = NULL;
		
		if (!task_spawn("/app/getvc", argv))
			printf(NAME ": Error spawning getvc on %s\n", vc);
	} else {
		printf(NAME ": Error waiting on %s\n", vc);
	}
}

void mount_data(void)
{
	int rc;

	printf("Trying to mount disk0 on /data... ");
	fflush(stdout);

	rc = mount("fat", "/data", "disk0", "wtcache", 0);
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
	
	spawn("/srv/fb");
	spawn("/srv/kbd");
	spawn("/srv/console");
	spawn("/srv/fhc");
	spawn("/srv/obio");
	spawn("/srv/ata_bd");
	spawn("/srv/gxe_bd");

	usleep(250000);
	mount_data();	

	getvc("vc0", "/app/bdsh");
	getvc("vc1", "/app/bdsh");
	getvc("vc2", "/app/bdsh");
	getvc("vc3", "/app/bdsh");
	getvc("vc4", "/app/bdsh");
	getvc("vc5", "/app/bdsh");
	getvc("vc6", "/app/klog");
	
	return 0;
}

/** @}
 */
