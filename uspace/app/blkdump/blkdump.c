/*
 * Copyright (c) 2011 Martin Sucha
 * Copyright (c) 2013 Jiri Svoboda
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

/** @addtogroup fs
 * @{
 */

/**
 * @file	blockdump.c
 * @brief	Tool for dumping content of block devices
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <block.h>
#include <mem.h>
#include <loc.h>
#include <byteorder.h>
#include <scsi/mmc.h>
#include <offset.h>
#include <inttypes.h>
#include <errno.h>
#include <str.h>

#define NAME	"blkdump"

static void syntax_print(void);
static int print_blocks(aoff64_t block_offset, aoff64_t block_count, size_t block_size);
static int print_toc(void);
static void print_hex_row(uint8_t *data, size_t length, size_t bytes_per_row);

static bool relative = false;
static service_id_t service_id;

int main(int argc, char **argv)
{

	errno_t rc;
	char *dev_path;
	size_t block_size;
	char *endptr;
	aoff64_t block_offset = 0;
	aoff64_t block_count = 1;
	aoff64_t dev_nblocks;
	bool toc = false;

	if (argc < 2) {
		printf(NAME ": Error, argument missing.\n");
		syntax_print();
		return 1;
	}

	--argc; ++argv;

	if (str_cmp(*argv, "--toc") == 0) {
		--argc; ++argv;
		toc = true;
		goto devname;
	}

	if (str_cmp(*argv, "--relative") == 0) {
		--argc; ++argv;
		relative = true;
	}

	if (str_cmp(*argv, "--offset") == 0) {
		--argc; ++argv;
		if (*argv == NULL) {
			printf(NAME ": Error, argument missing (offset).\n");
			syntax_print();
			return 1;
		}

		block_offset = strtol(*argv, &endptr, 10);
		if (*endptr != '\0') {
			printf(NAME ": Error, invalid argument (offset).\n");
			syntax_print();
			return 1;
		}

		--argc; ++argv;
	}

	if (str_cmp(*argv, "--count") == 0) {
		--argc; ++argv;
		if (*argv == NULL) {
			printf(NAME ": Error, argument missing (count).\n");
			syntax_print();
			return 1;
		}

		block_count = strtol(*argv, &endptr, 10);
		if (*endptr != '\0') {
			printf(NAME ": Error, invalid argument (count).\n");
			syntax_print();
			return 1;
		}

		--argc; ++argv;
	}

devname:
	if (argc != 1) {
		printf(NAME ": Error, unexpected argument.\n");
		syntax_print();
		return 1;
	}

	dev_path = *argv;

	rc = loc_service_get_id(dev_path, &service_id, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n", dev_path);
		return 2;
	}

	rc = block_init(service_id, 2048);
	if (rc != EOK)  {
		printf(NAME ": Error initializing libblock.\n");
		return 2;
	}

	rc = block_get_bsize(service_id, &block_size);
	if (rc != EOK) {
		printf(NAME ": Error determining device block size.\n");
		return 2;
	}

	rc = block_get_nblocks(service_id, &dev_nblocks);
	if (rc != EOK) {
		printf(NAME ": Warning, failed to obtain block device size.\n");
	}

	printf("Device %s has %" PRIuOFF64 " blocks, %" PRIuOFF64 " bytes each\n", dev_path, dev_nblocks, (aoff64_t) block_size);

	int ret;
	if (toc)
		ret = print_toc();
	else
		ret = print_blocks(block_offset, block_count, block_size);

	block_fini(service_id);

	return ret;
}

static int print_blocks(aoff64_t block_offset, aoff64_t block_count, size_t block_size)
{
	uint8_t *data;
	aoff64_t current;
	aoff64_t limit;
	size_t data_offset;
	errno_t rc;

	data = malloc(block_size);
	if (data == NULL) {
		printf(NAME ": Error allocating data buffer of %" PRIuOFF64 " bytes", (aoff64_t) block_size);
		return 3;
	}

	limit = block_offset + block_count;
	for (current = block_offset; current < limit; current++) {
		rc = block_read_direct(service_id, current, 1, data);
		if (rc != EOK) {
			printf(NAME ": Error reading block at %" PRIuOFF64 " \n", current);
			free(data);
			return 3;
		}

		printf("---- Block %" PRIuOFF64 " (at %" PRIuOFF64 ") ----\n", current, current*block_size);

		for (data_offset = 0; data_offset < block_size; data_offset += 16) {
			if (relative) {
				printf("%8" PRIxOFF64 ": ", (aoff64_t) data_offset);
			}
			else {
				printf("%8" PRIxOFF64 ": ", current*block_size + data_offset);
			}
			print_hex_row(data+data_offset, block_size-data_offset, 16);
			printf("\n");
		}
		printf("\n");
	}

	free(data);
	return 0;
}

static int print_toc(void)
{
	scsi_toc_multisess_data_t toc;
	errno_t rc;

	rc = block_read_toc(service_id, 0, &toc, sizeof(toc));
	if (rc != EOK)
		return 1;

	printf("Multisession Information:\n");
	printf("\tFirst complete session: %" PRIu8 "\n", toc.first_sess);
	printf("\tLast complete session: %" PRIu8 "\n", toc.last_sess);
	printf("\tFirst track of last complete session:\n");
	printf("\t\tADR / Control: 0x%" PRIx8 "\n", toc.ftrack_lsess.adr_control);
	printf("\t\tTrack number: %" PRIu8 "\n", toc.ftrack_lsess.track_no);
	printf("\t\tStart block address: %" PRIu32 "\n", toc.ftrack_lsess.start_addr);

	return 0;
}

/**
 * Print a row of 16 bytes as commonly seen in hexadecimal dumps
 */
static void print_hex_row(uint8_t *data, size_t length, size_t bytes_per_row) {
	size_t pos;
	uint8_t b;

	if (length > bytes_per_row) {
		length = bytes_per_row;
	}

	/* Print hexadecimal values */
	for (pos = 0; pos < length; pos++) {
		if (pos == length/2) {
			printf(" ");
		}
		printf("%02hhX ", data[pos]);
	}

	/* Pad with spaces if we have less than 16 bytes */
	for (pos = length; pos < bytes_per_row; pos++) {
		if (pos == length/2) {
			printf(" ");
		}
		printf("   ");
	}

	/* Print printable characters */
	for (pos = 0; pos < length; pos++) {
		if (pos == length/2) {
			printf(" ");
		}
		b = data[pos];
		if (b >= 32 && b < 128) {
			putchar(b);
		}
		else {
			putchar('.');
		}
	}
}

static void syntax_print(void)
{
	printf("syntax: blkdump [--toc] [--relative] [--offset <num_blocks>] "
	    "[--count <num_blocks>] <device_name>\n");
}

/**
 * @}
 */
