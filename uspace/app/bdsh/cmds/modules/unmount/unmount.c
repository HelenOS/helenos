/*
 * Copyright (c) 2010 Jakub Jermar
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

#include <stdio.h>
#include <stdlib.h>
#include <vfs/vfs.h>
#include <errno.h>
#include <str_error.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "unmount.h"
#include "cmds.h"

static const char *cmdname = "unmount";

/* Dispays help for unmount in various levels */
void help_cmd_unmount(unsigned int level)
{
	if (level == HELP_SHORT) {
		printf("'%s' unmount a file system.\n", cmdname);
	} else {
		help_cmd_unmount(HELP_SHORT);
		printf("Usage: %s <mp>\n", cmdname);
	}
	return;
}

/* Main entry point for unmount, accepts an array of arguments */
int cmd_unmount(char **argv)
{
	unsigned int argc;
	errno_t rc;

	argc = cli_count_args(argv);

	if (argc != 2) {
		printf("%s: invalid number of arguments.\n",
		    cmdname);
		return CMD_FAILURE;
	}

	rc = vfs_unmount_path(argv[1]);
	if (rc != EOK) {
		printf("Unable to unmount %s: %s\n", argv[1], str_error(rc));
		return CMD_FAILURE;
	}

	return CMD_SUCCESS;
}

