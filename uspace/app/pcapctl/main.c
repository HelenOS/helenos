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

#define LOGGER(msg, ...) \
     fprintf(stderr, \
         "[PCAP %s:%d]: " msg "\n", \
         __FILE__, __LINE__, \
         ##__VA_ARGS__\
     )

pcapctl_sess_t sess;

static void start_dumping(const char *name)
{
	pcapctl_dump_start(name, &sess);
}

static void stop_dumping(void)
{
	pcapctl_dump_stop(&sess);
}

static void usage(const char *progname)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  %s start <outfile>: Packets will be written to <outfile>\n", progname);
	fprintf(stderr, "  %s stop: Dumping stops\n", progname);

}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		usage(argv[0]);
		return 1;
	} else {
		errno_t rc = pcapctl_dump_init(&sess);
		if (rc != EOK) {
			fprintf(stderr, "Error initializing ...\n");
			return 1;
		}
		if (str_cmp(argv[1], "start") == 0) {
			if (argc != 3) {
				usage(argv[0]);
				return 1;
			}
			start_dumping(argv[2]);
		} else if (str_cmp(argv[1], "stop") == 0) {
			stop_dumping();
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
