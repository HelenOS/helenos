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
#include <string.h>
#include "config.h"
#include "util.h"
#include "errors.h"
#include "entry.h"
#include "bdd.h"
#include "cmds.h"

#include <libblock.h>
#include <devmap.h>
#include <errno.h>

#define BLOCK_SIZE	512
#define BPR		 16

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
	dev_handle_t handle;
	block_t *block;
	uint8_t *blk;
	size_t size, bytes, rows;
	int rc;
	bn_t boff;
	uint8_t b;

	/* Count the arguments */
	for (argc = 0; argv[argc] != NULL; argc ++);

	if (argc < 2 || argc > 4) {
		printf("%s - incorrect number of arguments.\n", cmdname);
		return CMD_FAILURE;
	}

	if (argc >= 3)
		boff = strtol(argv[2], NULL, 0);
	else
		boff = 0;

	if (argc >= 4)
		size = strtol(argv[3], NULL, 0);
	else
		size = 256;

	rc = devmap_device_get_handle(argv[1], &handle, 0);
	if (rc != EOK) {
		printf("Error: could not resolve device `%s'.\n", argv[1]);
		return CMD_FAILURE;
	}

	rc = block_init(handle, BLOCK_SIZE);
	if (rc != EOK)  {
		printf("Error: could not init libblock.\n");
		return CMD_FAILURE;
	}

	rc = block_cache_init(handle, BLOCK_SIZE, 2, CACHE_MODE_WB);
	if (rc != EOK) {
		printf("Error: could not init block cache.\n");
		return CMD_FAILURE;
	}

	while (size > 0) {
		block = block_get(handle, boff, 0);
		blk = (uint8_t *) block->data;

		bytes = (size < BLOCK_SIZE) ? size : BLOCK_SIZE;
		rows = (bytes + BPR - 1) / BPR;

		for (j = 0; j < rows; j++) {
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
			putchar('\n');
		}

		block_put(block);

		if (size > rows * BPR)
			size -= rows * BPR;
		else
			size = 0;

		boff += rows * BPR;
	}

	block_fini(handle);

	return CMD_SUCCESS;
}
