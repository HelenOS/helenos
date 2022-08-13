/*
 * SPDX-FileCopyrightText: 2017 Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
