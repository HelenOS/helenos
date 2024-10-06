/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup lprint
 * @{
 */

/**
 * @file
 * @brief Shut the system down
 *
 */

#include <fibril_synch.h>
#include <nchoice.h>
#include <stdio.h>
#include <stdbool.h>
#include <str.h>
#include <system.h>
#include "shutdown.h"

#define NAME "shutdown"

static void syntax_print(void);

static sd_action_t action;

static void sd_shutdown_complete(void *);
static void sd_shutdown_failed(void *);

static system_cb_t sd_system_cb = {
	.shutdown_complete = sd_shutdown_complete,
	.shutdown_failed = sd_shutdown_failed
};

/** System shutdown complete.
 *
 * @param arg Argument (shutdown_t *)
 */
static void sd_shutdown_complete(void *arg)
{
	shutdown_t *shutdown = (shutdown_t *)arg;

	fibril_mutex_lock(&shutdown->lock);
	shutdown->stopped = true;
	shutdown->failed = false;
	fibril_condvar_broadcast(&shutdown->cv);
	fibril_mutex_unlock(&shutdown->lock);
}

/** System shutdown failed.
 *
 * @param arg Argument (not used)
 */
static void sd_shutdown_failed(void *arg)
{
	shutdown_t *shutdown = (shutdown_t *)arg;

	fibril_mutex_lock(&shutdown->lock);
	shutdown->stopped = true;
	shutdown->failed = true;
	fibril_condvar_broadcast(&shutdown->cv);
	fibril_mutex_unlock(&shutdown->lock);
}

/** Interactively choose the shutdown action to perform
 *
 * @return EOK on success or an error code
 */
static errno_t choose_action(void)
{
	errno_t rc;
	nchoice_t *nchoice = NULL;
	void *choice;

	rc = nchoice_create(&nchoice);
	if (rc != EOK) {
		printf(NAME ": Out of memory.\n");
		goto error;
	}

	rc = nchoice_set_prompt(nchoice, "Do you want to shut the system down? "
	    "Select action:");
	if (rc != EOK) {
		printf(NAME ": Out of memory.\n");
		goto error;
	}

	rc = nchoice_add(nchoice, "Power off", (void *)(uintptr_t)sd_poweroff,
	    0);
	if (rc != EOK) {
		printf(NAME ": Out of memory.\n");
		goto error;
	}

	rc = nchoice_add(nchoice, "Cancel", (void *)(uintptr_t)sd_cancel,
	    ncf_default);
	if (rc != EOK) {
		printf(NAME ": Out of memory.\n");
		goto error;
	}

	rc = nchoice_get(nchoice, &choice);
	if (rc != EOK) {
		if (rc != ENOENT)
			printf(NAME ": Error getting user choice.\n");
		goto error;
	}

	action = (sd_action_t)choice;
	nchoice_destroy(nchoice);
	return EOK;
error:
	if (nchoice != NULL)
		nchoice_destroy(nchoice);
	return rc;
}

int main(int argc, char **argv)
{
	errno_t rc;
	system_t *system = NULL;
	shutdown_t shutdown;

	--argc;
	++argv;

	while (*argv != NULL && *argv[0] == '-') {
		if (str_cmp(*argv, "-p") == 0) {
			--argc;
			++argv;
			action = sd_poweroff;
			continue;
		}

		printf(NAME ": Error, invalid option.\n");
		syntax_print();
		return 1;
	}

	if (argc >= 1) {
		printf(NAME ": Error, unexpected argument.\n");
		syntax_print();
		return 1;
	}

	if (action == 0)
		choose_action();

	if (action == sd_cancel)
		return 0;

	fibril_mutex_initialize(&shutdown.lock);
	fibril_condvar_initialize(&shutdown.cv);
	shutdown.stopped = false;
	shutdown.failed = false;

	rc = system_open(SYSTEM_DEFAULT, &sd_system_cb, &shutdown, &system);
	if (rc != EOK) {
		printf(NAME ": Failed opening system control service.\n");
		return 1;
	}

	rc = system_shutdown(system);
	if (rc != EOK) {
		system_close(system);
		printf(NAME ": Failed requesting system shutdown.\n");
		return 1;
	}

	fibril_mutex_lock(&shutdown.lock);
	printf("The system is shutting down...\n");
	while (!shutdown.stopped)
		fibril_condvar_wait(&shutdown.cv, &shutdown.lock);

	if (shutdown.failed) {
		printf("Shutdown failed.\n");
		system_close(system);
		return 1;
	}

	printf("Shutdown complete. It is now safe to remove power.\n");

	/* Sleep forever */
	while (true)
		fibril_condvar_wait(&shutdown.cv, &shutdown.lock);

	fibril_mutex_unlock(&shutdown.lock);

	system_close(system);
	return 0;
}

/** Print syntax help. */
static void syntax_print(void)
{
	printf("syntax:\n"
	    "\tshutdown [<options>]\n"
	    "options:\n"
	    "\t-p Power off\n");
}

/**
 * @}
 */
