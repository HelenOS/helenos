/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
