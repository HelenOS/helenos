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
#include <errno.h>
#include <str_error.h>
#include <usb/diag/diag.h>
#include <usb/diag/iface.h>
#include "commands.h"

#define NAME "tmon"

static int resolve_default_fun(devman_handle_t *fun)
{
	category_id_t diag_cat;
	service_id_t *svcs;
	size_t count;
	int rc;

	if ((rc = loc_category_get_id(USB_DIAG_CATEGORY, &diag_cat, 0))) {
		printf(NAME ": Error resolving category '%s'", USB_DIAG_CATEGORY);
		return rc;
	}

	if ((rc = loc_category_get_svcs(diag_cat, &svcs, &count))) {
		printf(NAME ": Error getting list of diagnostic devices.\n");
		return rc;
	}

	// There must be exactly one diagnostic device for this to work.
	if (count != 1) {
		if (count) {
			printf(NAME ": Found %ld devices. Please specify which to use.\n", count);
		} else {
			printf(NAME ": No diagnostic devices found.\n");
		}
		return ENOENT;
	}

	if ((rc = devman_fun_sid_to_handle(svcs[0], fun))) {
		printf(NAME ": Error resolving handle of device with SID %ld.\n", svcs[0]);
		return rc;
	}

	return EOK;
}

static int resolve_named_fun(const char *dev_path, devman_handle_t *fun)
{
	int rc;
	service_id_t svc;
	if ((rc = loc_service_get_id(dev_path, &svc, IPC_FLAG_BLOCKING))) {
		printf(NAME ": Error resolving device. %s\n", str_error(rc));
		return rc;
	}

	if ((rc = devman_fun_sid_to_handle(svc, fun))) {
		printf(NAME ": Error resolving handle of device with SID %ld.\n", svc);
		return rc;
	}

	return EOK;
}

static int resolve_and_test(int argc, char *argv[], int (*test)(devman_handle_t)) {
	devman_handle_t fun = -1;

	if (argc == 0) {
		if (resolve_default_fun(&fun))
			return 1;
	} else if (argc == 1) {
		if (resolve_named_fun(argv[0], &fun))
			return 1;
	} else {
		printf(NAME ": Too many arguments provided.\n");
		return 1;
	}

	return test(fun);
}

static int bulk_worker(devman_handle_t fun) {
	async_sess_t *sess = usb_diag_connect(fun);
	async_exch_t *exch = async_exchange_begin(sess);

	// TODO: do some testing
	int y;
	int rc = usb_diag_test(exch, 4200, &y);

	if (rc) {
		printf(NAME ": %s\n", str_error(rc));
	} else {
		printf("The number is %d.\n", y);
	}

	async_exchange_end(exch);
	usb_diag_disconnect(sess);
	return 0;
}

int tmon_test_bulk(int argc, char *argv[])
{
	return resolve_and_test(argc, argv, bulk_worker);
}

/** @}
 */
