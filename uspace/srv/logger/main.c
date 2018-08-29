/*
 * Copyright (c) 2012 Vojtech Horky
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

/**
 * @addtogroup logger
 * @{
 */

/** @file
 */

#include <ipc/services.h>
#include <ipc/logger.h>
#include <io/log.h>
#include <io/logctl.h>
#include <ns.h>
#include <async.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <str_error.h>
#include "logger.h"

static void connection_handler_control(ipc_call_t *icall, void *arg)
{
	logger_connection_handler_control(icall);
}

static void connection_handler_writer(ipc_call_t *icall, void *arg)
{
	logger_connection_handler_writer(icall);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS Logging Service\n");

	parse_initial_settings();
	for (int i = 1; i < argc; i++) {
		parse_level_settings(argv[i]);
	}

	errno_t rc = service_register(SERVICE_LOGGER, INTERFACE_LOGGER_CONTROL,
	    connection_handler_control, NULL);
	if (rc != EOK) {
		printf("%s: Failed to register control port: %s.\n", NAME,
		    str_error(rc));
		return -1;
	}

	rc = service_register(SERVICE_LOGGER, INTERFACE_LOGGER_WRITER,
	    connection_handler_writer, NULL);
	if (rc != EOK) {
		printf("%s: Failed to register writer port: %s.\n", NAME,
		    str_error(rc));
		return -1;
	}

	printf("%s: Accepting connections\n", NAME);
	async_manager();

	/* Never reached */
	return 0;
}

/**
 * @}
 */
