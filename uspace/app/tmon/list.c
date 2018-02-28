/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup tmon
 * @{
 */
/**
 * @file
 * USB transfer debugging.
 */

#include <stdio.h>
#include <devman.h>
#include <loc.h>
#include <usbdiag_iface.h>
#include "commands.h"

#define NAME "tmon"
#define MAX_PATH_LENGTH 1024

/** Print a single item of the device list.
 * @param[in] svc Service ID of the devman function.
 */
static void print_list_item(service_id_t svc)
{
	errno_t rc;
	devman_handle_t diag_handle = 0;

	if ((rc = devman_fun_sid_to_handle(svc, &diag_handle))) {
		printf(NAME ": Error resolving handle of device with SID %" PRIun ", skipping.\n", svc);
		return;
	}

	char path[MAX_PATH_LENGTH];
	if ((rc = devman_fun_get_path(diag_handle, path, sizeof(path)))) {
		printf(NAME ": Error resolving path of device with SID %" PRIun ", skipping.\n", svc);
		return;
	}

	printf("%s\n", path);
}

/** List command handler.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_list(int argc, char *argv[])
{
	category_id_t diag_cat;
	service_id_t *svcs;
	size_t count;
	int rc;

	if ((rc = loc_category_get_id(USBDIAG_CATEGORY, &diag_cat, 0))) {
		printf(NAME ": Error resolving category '%s'", USBDIAG_CATEGORY);
		return 1;
	}

	if ((rc = loc_category_get_svcs(diag_cat, &svcs, &count))) {
		printf(NAME ": Error getting list of diagnostic devices.\n");
		return 1;
	}

	for (unsigned i = 0; i < count; ++i) {
		print_list_item(svcs[i]);
	}

	free(svcs);
	return 0;
}

/** @}
 */
