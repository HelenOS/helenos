/*
 * Copyright (c) 2019 Matthieu Riolo
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

#include <abi/shutdown.h>
#include <errno.h>
#include <libc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>

const char *APP_NAME = "shutdown";

static void help()
{
	printf("%s [options]\n\n", APP_NAME);
	printf("Shutdowns the system by rebooting or halting the system\n");
	printf("Already launched shutdowns can be canceled!\n\n");

	printf("-h --help              prints this message\n");
	printf("-k --kernel-console    switches into kernel console before shutdown\n");
	printf("-c --cancel            cancels a previous started shutdown\n");
	printf("-H --halt              halts the system\n");
	printf("-r --reboot            reboots the system\n");
	printf("-d --delay <seconds>   delays the shutdown\n");
}

static int print_error(const char *msg)
{
	printf("%sUse `%s -h` for help\n", msg, APP_NAME);
	return 1;
}

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "kernel-console", no_argument, 0, 'k' },
	{ "cancel", no_argument, 0, 'c' },
	{ "halt", no_argument, 0, 'H' },
	{ "reboot", no_argument, 0, 'r' },
	{ "delay", required_argument, 0, 'd' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
	if (argc == 1) {
		goto print_help;
	}

	int c, option_index;
	int delay = 0;
	shutdown_mode_t mode = SHUTDOWN_UNDEFINED;
	bool kconsole = false;

	while ((c = getopt_long(argc, argv, "hkcHrd:", long_options, &option_index)) != -1) {
		switch (c) {
		case 'k':
			kconsole = true;
			break;
		case 'c':
			if (mode != SHUTDOWN_UNDEFINED)
				return print_error("The mode has already been set\n");
			mode = SHUTDOWN_CANCEL;
			break;
		case 'H':
			if (mode != SHUTDOWN_UNDEFINED)
				return print_error("The mode has already been set\n");
			mode = SHUTDOWN_HALT;
			break;
		case 'r':
			if (mode != SHUTDOWN_UNDEFINED)
				return print_error("The mode has already been set\n");
			mode = SHUTDOWN_REBOOT;
			break;
		case 'd':
			delay = atoi(optarg);
			if (delay <= 0)
				print_error("Delay must be bigger than zero\n");
			break;
		case '?':
			return print_error("");
		case 'h':
		default:
		print_help:
			help();
			return 0;
		}
	}

	if (mode == SHUTDOWN_UNDEFINED) {
		return print_error("You have to specific the mode\n");
	} else if (mode == SHUTDOWN_CANCEL && delay != 0) {
		return print_error("The mode cancel cannot be delayed\n");
	}

	if (optind < argc)
		return print_error("Too many arguments have been passed");

	errno_t rc = __SYSCALL3(SYS_SHUTDOWN, (sysarg_t) mode, (sysarg_t) delay, (sysarg_t) kconsole);

	if (rc != EOK)
		return print_error("There was an error while executing the systemcall");

	return 0;
}
