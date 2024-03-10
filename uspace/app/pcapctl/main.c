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

pcapctl_sess_t* sess;

static errno_t start_dumping(const char *svc_name, const char *name)
{
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
	errno_t rc = pcapctl_dump_open(svc_name, &sess);
	if (rc != EOK) {
		return 1;
	}

	pcapctl_dump_stop(sess);
	pcapctl_dump_close(sess);
	return EOK;
}

static void list_devs(void) {
	pcapctl_list();
}

static void usage(const char *progname)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s list: List of devices\n", progname);
	fprintf(stderr, "  %s start <device> <outfile>: Packets dumped from <device> will be written to <outfile>\n", progname);
	fprintf(stderr, "  %s stop <device>: Dumping from <device> stops\n", progname);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		usage(argv[0]);
		return 1;
	} else {
		if (str_cmp(argv[1], "--help") == 0 || str_cmp(argv[1], "-h") == 0) {
			usage(argv[0]);
			return 0;
		} else if (str_cmp(argv[1], "list") == 0) {
			list_devs();
			return 0;
		} else if (str_cmp(argv[1], "start") == 0) {
			if (argc != 4) {
				usage(argv[0]);
				return 1;
			}
			start_dumping(argv[2], argv[3]);
		} else if (str_cmp(argv[1], "stop") == 0) {
			if (argc != 3) {
				usage(argv[0]);
				return 1;
			}
			stop_dumping(argv[2]);
			fprintf(stdout, "Dumping was stopped\n");
			return EOK;
		} else {
			usage(argv[0]);
			return 1;
		}
	}
	return 0;
}

/** @}
 */
