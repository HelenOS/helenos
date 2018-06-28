/*
 * Copyright (c) 2017 Jiri Svoboda
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

/** @addtogroup vol
 * @{
 */
/** @file Volume administration (interface to volsrv).
 */

#include <errno.h>
#include <io/table.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <vfs/vfs.h>
#include <vol.h>

#define NAME "vol"

static char *volspec;

typedef enum {
	vcmd_eject,
	vcmd_help,
	vcmd_list,
} vol_cmd_t;

static int vol_cmd_list(void)
{
	vol_t *vol = NULL;
	vol_part_info_t vinfo;
	service_id_t *part_ids = NULL;
	char *svc_name;
	char *sfstype;
	size_t nparts;
	size_t i;
	table_t *table = NULL;
	int rc;

	rc = vol_create(&vol);
	if (rc != EOK) {
		printf("Error contacting volume service.\n");
		goto out;
	}

	rc = vol_get_parts(vol, &part_ids, &nparts);
	if (rc != EOK) {
		printf("Error getting list of volumes.\n");
		goto out;
	}

	rc = table_create(&table);
	if (rc != EOK) {
		printf("Out of memory.\n");
		goto out;
	}

	table_header_row(table);
	table_printf(table, "Volume Name\t" "Resource\t" "Content\t" "Auto\t"
	    "Mounted at\n");

	for (i = 0; i < nparts; i++) {
		rc = vol_part_info(vol, part_ids[i], &vinfo);
		if (rc != EOK) {
			printf("Error getting volume information.\n");
			return EIO;
		}

		rc = loc_service_get_name(part_ids[i], &svc_name);
		if (rc != EOK) {
			printf("Error getting service name.\n");
			return EIO;
		}

		rc = vol_pcnt_fs_format(vinfo.pcnt, vinfo.fstype, &sfstype);
		if (rc != EOK) {
			printf("Out of memory.\n");
			free(svc_name);
			return ENOMEM;
		}

		table_printf(table, "%s\t" "%s\t" "%s\t" "%d\t" "%s\n",
		    vinfo.label, svc_name, sfstype, vinfo.cur_mp_auto,
		    vinfo.cur_mp);

		free(svc_name);
		free(sfstype);
		svc_name = NULL;
	}

	rc = table_print_out(table, stdout);
	if (rc != EOK)
		printf("Error printing table.\n");
out:
	table_destroy(table);
	vol_destroy(vol);
	free(part_ids);

	return rc;
}

static void print_syntax(void)
{
	printf("Syntax:\n");
	printf("  %s                List volumes\n", NAME);
	printf("  %s -h             Print help\n", NAME);
	printf("  %s eject <volume> Eject volume\n", NAME);
}

int main(int argc, char *argv[])
{
	char *cmd;
	vol_cmd_t vcmd;
	int i;
	int rc;

	if (argc < 2) {
		vcmd = vcmd_list;
		i = 1;
	} else {
		cmd = argv[1];
		i = 2;
		if (str_cmp(cmd, "-h") == 0) {
			vcmd = vcmd_help;
		} else if (str_cmp(cmd, "eject") == 0) {
			vcmd = vcmd_eject;
			if (argc <= i) {
				printf("Parameter missing.\n");
				goto syntax_error;
			}
			volspec = argv[i++];
		} else {
			printf("Invalid sub-command '%s'.\n", cmd);
			goto syntax_error;
		}
	}

	if (argc > i) {
		printf("Unexpected argument '%s'.\n", argv[i]);
		goto syntax_error;
	}

	switch (vcmd) {
	case vcmd_eject:
		rc = EOK;
		break;
	case vcmd_help:
		print_syntax();
		rc = EOK;
		break;
	case vcmd_list:
		rc = vol_cmd_list();
		break;
	}

	if (rc != EOK)
		return 1;

	return 0;

syntax_error:
	printf("Use %s -h to get help.\n", NAME);
	return 1;
}

/** @}
 */
