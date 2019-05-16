/*
 * Copyright (c) 2019 Matthieu Riolo
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
#include <str.h>
#include <dirent.h>
#include <ipc/services.h>
#include <ipc/input.h>
#include <abi/ipc/interfaces.h>
#include <loc.h>

#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "layout.h"
#include "cmds.h"

static const char *cmdname = "layout";
static const char *path_layouts = "/lib/layouts/";


/* Dispays help for layout in various levels */
void help_cmd_layout(unsigned int level)
{
	printf("Changes, list or display the current keyboard layout.\n");

	if (level != HELP_SHORT) {
		printf(
			"Usage: %s\n"
			"\t%s list\tlists all layouts\n"
			"\t%s get\t displays currently set layout\n"
			"\t%s set <layout>\tchanges to the new layout\n",
			cmdname, cmdname, cmdname, cmdname);
	}

	return;
}

/* lists all available kb layouts */
static int cmd_layout_list(void)
{
	DIR *dir = opendir(path_layouts);
	if (!dir) {
		cli_error(CL_ENOENT, "%s: Error reading directory %s\n", cmdname, path_layouts);
		return CMD_FAILURE;
	}

	struct dirent *dp;
	int ERROR = CMD_SUCCESS;

	while ((dp = readdir(dir))) {
		char *suffixpos = str_chr(dp->d_name, '.');
		if (suffixpos == NULL || suffixpos == dp->d_name) {
			continue;
		}

		if (str_cmp(suffixpos, ".so") != 0) {
			continue;
		}

		char *name = str_ndup(dp->d_name, suffixpos - dp->d_name);
		if (name == NULL) {
			cli_error(CL_ENOMEM, "%s: Error while duplicating file name\n", cmdname);
			ERROR = CMD_FAILURE;
			goto error;
		}

		printf("%s\n", name);
		free(name);
	}

error:
	closedir(dir);
	return ERROR;
}

static int cmd_layout_get(void)
{
	return CMD_FAILURE;
}


static int cmd_layout_set(char *layout)
{
#ifdef CONFIG_RTLD
	service_id_t svcid;
	ipc_call_t call;
	errno_t rc = loc_service_get_id(SERVICE_NAME_HID_INPUT, &svcid, 0);
	if (rc != EOK) {
		cli_error(CL_ENOENT, "%s: Failing to find service `%s`\n", cmdname, SERVICE_NAME_HID_INPUT);
		return CMD_FAILURE;
	}

	async_sess_t *sess = loc_service_connect(svcid, INTERFACE_ANY, 0);
	if (sess == NULL) {
		cli_error(CL_ENOENT, "%s: Failing to connect to service `%s`\n", cmdname, SERVICE_NAME_HID_INPUT);
		return CMD_FAILURE;
	}

	async_exch_t *exch = async_exchange_begin(sess);

	aid_t mid = async_send_0(exch, INPUT_CHANGE_LAYOUT, &call);
	rc = async_data_write_start(exch, layout, str_size(layout));
	
	if (rc == EOK)
		async_wait_for(mid, &rc);
	
	async_exchange_end(exch);
	async_hangup(sess);

	return rc == EOK ? CMD_SUCCESS : CMD_FAILURE;
#else
	cli_error(CL_ENOTSUP, "%s: No support for RTLD\n", cmdname);
	return CMD_FAILURE;
#endif
}

/* Main entry point for layout, accepts an array of arguments */
int cmd_layout(char **argv)
{
	unsigned int argc = cli_count_args(argv);

	if (argc == 2 || argc == 3) {
		if (str_cmp(argv[1], "list") == 0) {
			return cmd_layout_list();
		} else if (str_cmp(argv[1], "get") == 0) {
			return cmd_layout_get();
		} else if (str_cmp(argv[1], "set") == 0 && argc == 3) {
			return cmd_layout_set(argv[2]);
		}	
	}

	help_cmd_layout(HELP_LONG);
	return CMD_FAILURE;
}

