/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup net
 * @{
 */

/** @file
 *
 * Start the networking subsystem.
 * Perform networking self-test if executed
 * with the -s argument.
 *
 */

#define NAME  "netstart"

#include <async.h>
#include <stdio.h>
#include <task.h>
#include <str_error.h>
#include <ipc/ipc.h>
#include <ipc/services.h>

#include <net_err.h>
#include <net_modules.h>
#include <net_net_messages.h>

#include "self_test.h"

/** Start a module.
 *
 * @param[in] desc The module description
 * @param[in] path The module absolute path.
 *
 * @returns true on succesful spanwning
 * @returns false on failure
 *
 */
static bool spawn(const char *desc, const char *path)
{
	printf("%s: Spawning %s (%s)\n", NAME, desc, path);
	
	const char *argv[2];
	
	argv[0] = path;
	argv[1] = NULL;
	
	int err;
	if (task_spawn(path, argv, &err) == 0) {
		fprintf(stderr, "%s: Error spawning %s (%s)\n", NAME, path,
		    str_error(err));
		return false;
	}
	
	return true;
}

int main(int argc, char *argv[])
{
	ERROR_DECLARE;
	
	/* Run self-tests */
	if ((argc > 1) && (str_cmp(argv[1], "-s") == 0))
		ERROR_PROPAGATE(self_test());
	
	if (!spawn("networking service", "/srv/net"))
		return EINVAL;
	
	printf("%s: Initializing networking\n", NAME);
	
	int net_phone = connect_to_service(SERVICE_NETWORKING);
	if (ERROR_OCCURRED(ipc_call_sync_0_0(net_phone, NET_NET_STARTUP))) {
		fprintf(stderr, "%s: Startup error %d\n", NAME, ERROR_CODE);
		return ERROR_CODE;
	}
	
	return EOK;
}

/** @}
 */
