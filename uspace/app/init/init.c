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
 * @brief	Init process for testing purposes.
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
#include <task.h>
#include <malloc.h>
#include "init.h"
#include "version.h"

static void console_wait(void)
{
	while (get_cons_phone() < 0)
		usleep(50000);	// FIXME
}

static bool mount_fs(const char *fstype)
{
	int rc = -1;
	
	while (rc < 0) {
		rc = mount(fstype, "/", "initrd");

		switch (rc) {
		case EOK:
			printf(NAME ": Root filesystem mounted\n");
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
		default:
			sleep(5);	// FIXME
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

	if (task_spawn(fname, argv) != 0) {
		/* Success */
		sleep(1);
	}
}

int main(int argc, char *argv[])
{
	info_print();
	sleep(5);	// FIXME
	
	if (!mount_fs("tmpfs") && !mount_fs("fat")) {
		printf(NAME ": Exiting\n");
		return -1;
	}
	
	// FIXME: spawn("/srv/pci");
	spawn("/srv/fb");
	spawn("/srv/kbd");
	spawn("/srv/console");
	spawn("/srv/fhc");
	spawn("/srv/obio");
	
	console_wait();
	version_print();
	
	spawn("/app/klog");
	spawn("/app/bdsh");
	
	return 0;
}

/** @}
 */
