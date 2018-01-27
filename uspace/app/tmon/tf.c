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
 * Testing framework.
 */

#include <stdio.h>
#include <devman.h>
#include <str_error.h>
#include <usbdiag_iface.h>
#include "resolve.h"
#include "tf.h"

#define NAME "tmon"
#define MAX_PATH_LENGTH 1024

/** Common command handler for all test commands.
 * @param[in] argc Number of arguments.
 * @param[in] argv Argument values. Must point to exactly `argc` strings.
 *
 * @return Exit code
 */
int tmon_test_main(int argc, char *argv[], const tmon_test_ops_t *ops)
{
	// Resolve device function.
	devman_handle_t fun = -1;

	if (argc >= 2 && *argv[1] != '-') {
		// Assume that the first argument is device path.
		if (tmon_resolve_named(argv[1], &fun))
			return 1;
	} else {
		// The first argument is either an option or not present.
		if (tmon_resolve_default(&fun))
			return 1;
	}

	int rc, ec;
	char path[MAX_PATH_LENGTH];
	if ((rc = devman_fun_get_path(fun, path, sizeof(path)))) {
		printf(NAME ": Error resolving path of device with handle "
		    "%" PRIun ". %s\n", fun, str_error(rc));
		return 1;
	}

	printf("Using device: %s\n", path);

	// Read test parameters from options.
	tmon_test_params_t *params = NULL;
	if ((rc = ops->read_params(argc, argv, &params))) {
		printf(NAME ": Reading test parameters failed. %s\n", str_error(rc));
		return 1;
	}

	// Run the test body.
	async_sess_t *sess = usbdiag_connect(fun);
	if (!sess) {
		printf(NAME ": Could not connect to the device.\n");
		return 1;
	}

	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch) {
		printf(NAME ": Could not start exchange with the device.\n");
		ec = 1;
		goto err_sess;
	}

	ec = ops->run(exch, params);
	async_exchange_end(exch);

err_sess:
	usbdiag_disconnect(sess);
	free(params);
	return ec;
}

/** @}
 */
