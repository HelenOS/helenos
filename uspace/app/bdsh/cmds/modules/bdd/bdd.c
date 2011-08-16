/*
 * Copyright (c) 2009 Jiri Svoboda
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

#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <sys/typefmt.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "bdd.h"
#include "cmds.h"

#include <libblock.h>
#include <loc.h>
#include <errno.h>
#include <assert.h>

enum {
	/* Number of bytes per row */
	BPR = 16
};

static const char *cmdname = "bdd";

/* Dispays help for bdd in various levels */
void help_cmd_bdd(unsigned int level)
{
	static char helpfmt[] =
	    "Usage:  %s <device> [<block_number> [<bytes>]]\n";
	if (level == HELP_SHORT) {
		printf("'%s' dump block device contents.\n", cmdname);
	} else {
		help_cmd_bdd(HELP_SHORT);
		printf(helpfmt, cmdname);
	}
	return;
}

/* Main entry point for bdd, accepts an array of arguments */
int cmd_bdd(char **argv)
{
	unsigned int argc;
	unsigned int i, j;
	service_id_t service_id;
	aoff64_t offset;
	uint8_t *blk;
	size_t size, bytes, rows;
	size_t block_size;
	int rc;
	aoff64_t ba;
	uint8_t b;

	/* Count the arguments */
	for (argc = 0; argv[argc] != NULL; argc ++);

	if (argc < 2 || argc > 4) {
		printf("%s - incorrect number of arguments.\n", cmdname);
		return CMD_FAILURE;
	}

	if (argc >= 3)
		ba = strtol(argv[2], NULL, 0);
	else
		ba = 0;

	if (argc >= 4)
		size = strtol(argv[3], NULL, 0);
	else
		size = 256;

	rc = loc_service_get_id(argv[1], &service_id, 0);
	if (rc != EOK) {
		printf("%s: Error resolving device `%s'.\n", cmdname, argv[1]);
		return CMD_FAILURE;
	}

	rc = block_init(EXCHANGE_SERIALIZE, service_id, 2048);
	if (rc != EOK)  {
		printf("%s: Error initializing libblock.\n", cmdname);
		return CMD_FAILURE;
	}

	rc = block_get_bsize(service_id, &block_size);
	if (rc != EOK) {
		printf("%s: Error determining device block size.\n", cmdname);
		return CMD_FAILURE;
	}

	blk = malloc(block_size);
	if (blk == NULL) {
		printf("%s: Error allocating memory.\n", cmdname);
		block_fini(service_id);
		return CMD_FAILURE;
	}

	offset = ba * block_size;

	while (size > 0) {
		rc = block_read_direct(service_id, ba, 1, blk);
		if (rc != EOK) {
			printf("%s: Error reading block %" PRIuOFF64 "\n", cmdname, ba);
			free(blk);
			block_fini(service_id);
			return CMD_FAILURE;
		}

		bytes = (size < block_size) ? size : block_size;
		rows = (bytes + BPR - 1) / BPR;

		for (j = 0; j < rows; j++) {
			printf("[%06" PRIxOFF64 "] ", offset);
			for (i = 0; i < BPR; i++) {
				if (j * BPR + i < bytes)
					printf("%02x ", blk[j * BPR + i]);
				else
					printf("   ");
			}
			putchar('\t');

			for (i = 0; i < BPR; i++) {
				if (j * BPR + i < bytes) {
					b = blk[j * BPR + i];
					if (b >= 32 && b < 127)
						putchar(b);
					else
						putchar(' ');
				} else {
					putchar(' ');
				}
			}
			offset += BPR;
			putchar('\n');
		}

		if (size > rows * BPR)
			size -= rows * BPR;
		else
			size = 0;

		/* Next block */
		ba += 1;
	}

	free(blk);
	block_fini(service_id);

	return CMD_SUCCESS;
}
