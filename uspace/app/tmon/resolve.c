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
 * USB diagnostic device resolving.
 */

#include <loc.h>
#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <usbdiag_iface.h>
#include "resolve.h"

#define NAME "tmon"

int tmon_resolve_default(devman_handle_t *fun)
{
	category_id_t diag_cat;
	service_id_t *svcs;
	size_t count;
	int rc;

	if ((rc = loc_category_get_id(USBDIAG_CATEGORY, &diag_cat, 0))) {
		printf(NAME ": Error resolving category '%s'", USBDIAG_CATEGORY);
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

int tmon_resolve_named(const char *dev_path, devman_handle_t *fun)
{
	int rc;
	if ((rc = devman_fun_get_handle(dev_path, fun, 0))) {
		printf(NAME ": Error resolving handle of device - %s.\n", str_error(rc));
		return rc;
	}

	return EOK;
}

/** @}
 */
