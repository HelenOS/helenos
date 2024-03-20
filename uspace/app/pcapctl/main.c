/*
 * Copyright (c) 2023 Nataliia Korop
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

/** @addtogroup pcapctl
 * @{
 */
/** @file pcapctl app
 */

#include <stdio.h>
#include <str.h>
#include <errno.h>

#include "pcapctl_dump.h"

#define NAME "pcapctl"
#define DEFAULT_DEV_NUM 0

//pcapctl_sess_t* sess = NULL;

static errno_t start_dumping(const char *svc_name, const char *name)
{
	pcapctl_sess_t *sess = NULL;
	errno_t rc = pcapctl_dump_open(svc_name, &sess);
	if (rc != EOK) {
		return 1;
	}

	pcapctl_dump_start(name, sess);
	pcapctl_dump_close(sess);
	return EOK;
}

/** Session might */
static errno_t stop_dumping(const char *svc_name)
{
	pcapctl_sess_t *sess = NULL;
	errno_t rc = pcapctl_dump_open(svc_name, &sess);
	if (rc != EOK) {
		return 1;
	}
	pcapctl_dump_stop(sess);
	pcapctl_dump_close(sess);
	return EOK;
}

static void list_devs(void)
{
	pcapctl_list();
}

static void usage(void)
{
	printf("Usage:\n"
	    NAME " list \n"
	    "\tList of devices\n"
	    NAME " start --device= | -d <device number from list> <outfile>\n"
	    "\tPackets dumped from device will be written to <outfile>\n"
	    NAME " stop --device= | -d <device>\n"
	    "\tDumping from <device> stops\n"
	    NAME " start <outfile>\n"
	    "\tPackets dumped from the 1st device from the list will be written to <outfile>\n"
	    NAME " --help | -h\n"
	    "\tShow this application help.\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		usage();
		return 1;
	} else {
		/** help */
		if (str_cmp(argv[1], "--help") == 0 || str_cmp(argv[1], "-h") == 0) {
			usage();
			return 0;
			/** list */
		} else if (str_cmp(argv[1], "list") == 0) {
			list_devs();
			return 0;
			/** start with/out devnum */
		} else if (str_cmp(argv[1], "start") == 0) {
			if (argc == 3) {
				start_dumping((char *)"0", argv[2]);
				return 0;
			} else if (argc == 4) {
				start_dumping(argv[2], argv[3]);
				return 0;
			} else {
				usage();
				return 1;
			}
			/** Stop with/out devnum */
		} else if (str_cmp(argv[1], "stop") == 0) {
			if (argc == 2) {
				stop_dumping((char *)"0");
				fprintf(stdout, "Dumping was stopped\n");
				return 0;
			} else if (argc == 3) {

				stop_dumping(argv[2]);
				fprintf(stdout, "Dumping was stopped\n");
			} else {
				usage();
				return 1;
			}
		} else {
			usage();
			return 1;
		}
	}
	return 0;
}

/** @}
 */
