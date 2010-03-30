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
 *  @{
 */

/** @file
 *  Starts the networking subsystem.
 *  Performs self test if configured to.
 *  @see configuration.h
 */

#include <async.h>
#include <stdio.h>
#include <task.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include <net_err.h>
#include <net_modules.h>
#include <net_net_messages.h>
#include "self_test.h"

/** Networking startup module name.
 */
#define NAME	"Networking startup"

/** Starts the module.
 *  @param[in] fname The module absolute name.
 *  @returns The started module task identifier.
 *  @returns Other error codes as defined for the task_spawn() function.
 */
static task_id_t spawn(const char * fname){
	const char * argv[2];
	task_id_t res;

	argv[0] = fname;
	argv[1] = NULL;
	res = task_spawn(fname, argv);
	
	return res;
}

/** Module entry point.
 *  @param[in] argc The number of command line parameters.
 *  @param[in] argv The command line parameters.
 *  @returns EOK on success.
 *  @returns EINVAL if the net module cannot be started.
 *  @returns Other error codes as defined for the self_test() function.
 *  @returns Other error codes as defined for the NET_NET_STARTUP message.
 */
int main(int argc, char * argv[]){
	ERROR_DECLARE;

	int net_phone;

	// print the module label
	printf("Task %d - ", task_get_id());
	printf("%s\n", NAME);

	// run self tests
	ERROR_PROPAGATE(self_test());

	// start the networking service
	if(! spawn("/srv/net")){
		fprintf(stderr, "Could not spawn net\n");
		return EINVAL;
	}

	// start the networking
	net_phone = connect_to_service(SERVICE_NETWORKING);
	if(ERROR_OCCURRED(ipc_call_sync_0_0(net_phone, NET_NET_STARTUP))){
		printf("ERROR %d\n", ERROR_CODE);
		return ERROR_CODE;
	}else{
		printf("OK\n");
	}

	return EOK;
}

/** @}
 */
