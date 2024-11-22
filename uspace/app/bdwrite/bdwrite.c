/*
 * Copyright (c) 2024 Miroslav Cimerman
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

/** @addtogroup bdwrite
 * @{
 */
/**
 * @file
 */

#include <block.h>
#include <errno.h>
#include <getopt.h>
#include <mem.h>
#include <stdlib.h>
#include <stdio.h>
#include <abi/ipc/ipc.h>

static void usage(void);

static const char usage_str[] =
    "Usage: bdwrite <dev> -o <offset in blocks> -c <block count>\n"
    "\n"
    "  Write cyclic blocks to block device.\n";

static struct option const long_options[] = {
	{ 0, 0, 0, 0 }
};

static void usage(void)
{
	printf("%s", usage_str);
}

int main(int argc, char **argv)
{
	errno_t rc;
	size_t bsize;
	int c;
	char *name = NULL;
	size_t blkcnt = 0, off = 0;
	service_id_t dev;

	if (argc != 6) {
		goto bad;
	}

	name = argv[1];

	c = 0;
	optreset = 1;
	optind = 0;

	while (c != -1) {
		c = getopt_long(argc, argv, "o:c:", long_options, NULL);
		switch (c) {
		case 'o':
			off = strtol(optarg, NULL, 10);
			break;
		case 'c':
			blkcnt = strtol(optarg, NULL, 10);
			break;
		}
	}

	rc = loc_service_get_id(name, &dev, 0);
	if (rc != EOK) {
		printf("bdwrite: error resolving device \"%s\"\n", name);
		return 1;
	}
	rc = block_init(dev);
	if (rc != EOK) {
		printf("bdwrite: error initializing block device \"%s\"\n", name);
		return 1;
	}

	rc = block_get_bsize(dev, &bsize);
	if (rc != EOK) {
		printf("bdwrite: error getting block size of \"%s\"\n", name);
		block_fini(dev);
		return 1;
	}

	uint64_t to_alloc = min(DATA_XFER_LIMIT, bsize * blkcnt);
	uint8_t *buf = malloc(to_alloc);
	if (buf == NULL) {
		rc = ENOMEM;
		goto end;
	}
	uint64_t left = blkcnt;
	while (left != 0) {
		uint64_t blks_to_write = min(to_alloc / bsize, left);
		uint8_t *ptr = buf;
		for (size_t i = 0; i < blks_to_write; i++) {
			/* memset(ptr, (i + 1) % 0x100, bsize); */
			memset(ptr, 'A' + (i % 26), bsize);
			ptr += bsize;
		}
		rc = block_write_direct(dev, off, blks_to_write, buf);
		if (rc != EOK) {
			printf("bdwrite: error writing to \"%s\"\n", name);
			goto end;
		}
		left -= blks_to_write;
		off += blks_to_write;
	}
end:
	free(buf);
	block_fini(dev);
	return rc;
bad:
	usage();
	return 0;
}

/** @}
 */
