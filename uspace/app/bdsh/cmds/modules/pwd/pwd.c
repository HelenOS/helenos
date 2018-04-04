/*
 * Copyright (c) 2008 Tim Post
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
#include <mem.h>
#include <vfs/vfs.h>
#include <abi/errno.h>

#include "config.h"
#include "errors.h"
#include "entry.h"
#include "cmds.h"
#include "pwd.h"

static const char *cmdname = "pwd";

void help_cmd_pwd(unsigned int level)
{
	printf("`%s' prints your current working directory.\n", cmdname);
	return;
}

int cmd_pwd(char *argv[])
{
	char *buff;

	buff = (char *) malloc(PATH_MAX);
	if (NULL == buff) {
		cli_error(CL_ENOMEM, "%s:", cmdname);
		return CMD_FAILURE;
	}

	memset(buff, 0, PATH_MAX);

	if (vfs_cwd_get(buff, PATH_MAX) != EOK) {
		cli_error(CL_EFAIL,
		    "Unable to determine the current working directory");
		free(buff);
		return CMD_FAILURE;
	} else {
		printf("%s\n", buff);
		free(buff);
		return CMD_SUCCESS;
	}
}
