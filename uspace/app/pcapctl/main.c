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
#include <arg_parse.h>
#include <getopt.h>
#include <vfs/vfs.h>

#include "pcapdump_client.h"

#define NAME "pcapctl"
#define DEFAULT_DEV_NUM 0
#define DECIMAL_SYSTEM 10

#define DEFAULT_FILE_OPS 0
#define SHORT_FILE_OPS 1
#define APPEND_FILE_OPS 2
#define USB_FILE_OPS 3

static errno_t start_dumping(int *dev_number, const char *name, int *ops_index)
{
	pcapctl_sess_t *sess = NULL;
	errno_t rc = pcapctl_dump_open(dev_number, &sess);
	if (rc != EOK) {
		return 1;
	}

	rc = pcapctl_is_valid_ops_number(ops_index, sess);
	if (rc != EOK) {
		printf("Wrong number of ops: %d.\n", *ops_index);
		pcapctl_dump_close(sess);
		return rc;
	}

	rc = pcapctl_dump_start(name, ops_index, sess);
	if (rc != EOK) {
		if (rc == EBUSY) {
			printf("Dumping for device %d is in process, stop to start dumping to file %s.\n", *dev_number, name);
		}
		printf("Starting the dumping was not successful.\n");
	}
	pcapctl_dump_close(sess);
	return EOK;
}

static errno_t stop_dumping(int *dev_number)
{
	pcapctl_sess_t *sess = NULL;
	errno_t rc = pcapctl_dump_open(dev_number, &sess);
	if (rc != EOK) {
		return 1;
	}
	rc = pcapctl_dump_stop(sess);
	if (rc != EOK) {
		printf("Stoping the dumping was not successful.\n");
	}
	pcapctl_dump_close(sess);
	return EOK;
}

static void list_devs(void)
{
	pcapctl_list();
}

/**
 * Array of supported commandline options
 */
static const struct option opts[] = {
	{ "append", required_argument, 0, 'A' }, // file as argument and ops 0 if not exist and 2 if exists
	{ "new", required_argument, 0, 'N' }, // file name as argument
	{ "truncated", required_argument, 0, 'T' }, // truncated ops
	{ "usb", required_argument, 0, 'U' }, //??
	{ "device", required_argument, 0, 'd' },
	{ "list", no_argument, 0, 'l' },
	{ "help", no_argument, 0, 'h' },
	{ "outfile", required_argument, 0, 'o' },
	{ "start", no_argument, 0, 'r' },
	{ "stop", no_argument, 0, 't' },
	{ "ops", required_argument, 0, 'p' },
	{ "force", no_argument, 0, 'f' },
	{ 0, 0, 0, 0 }
};

static bool file_exists(const char *path)
{
	vfs_stat_t stats;
	if (vfs_stat_path(path, &stats) != EOK) {
		return false;
	} else {
		return true;
	}
}

static void usage(void)
{
	printf("Usage:\n"
	    NAME " --list | -l \n"
	    "\tList of devices\n"
	    NAME " --start | -r --device= | -d <device number from list> --outfile= | -o <outfile> --ops= | p <ops index>\n"
	    "\tPackets dumped from device will be written to <outfile>\n"
	    NAME " --stop | -t --device= | -d <device number from list>\n"
	    "\tDumping from <device> stops\n"
	    NAME " --start | -r --outfile= | -o <outfile>\n"
	    "\tPackets dumped from the 0. device from the list will be written to <outfile>\n"
	    NAME " --help | -h\n"
	    "\tShow this application help.\n"
	    NAME " --force | -f"
	    "\tTo open existing file and write to it.\n");
}

int main(int argc, char *argv[])
{
	bool start = false;
	bool stop = false;
	int dev_number = DEFAULT_DEV_NUM;
	int ops_number = DEFAULT_FILE_OPS;
	bool forced = false;
	const char *output_file_name = "";
	int idx = 0;
	int ret = 0;
	if (argc == 1) {
		usage();
		return 0;
	}
	while (ret != -1) {
		ret = getopt_long(argc, argv, "A:N:T:U:d:lho:rtp:f", opts, &idx);
		switch (ret) {
		case 'd':
			char *rest;
			long dev_result = strtol(optarg, &rest, DECIMAL_SYSTEM);
			dev_number = (int)dev_result;
			errno_t rc = pcapctl_is_valid_device(&dev_number);
			if (rc != EOK) {
				printf("Device with index %d not found\n", dev_number);
				return 1;
			}
			break;
		case 'A':
			output_file_name = optarg;
			if (file_exists(output_file_name)) {
				ops_number = APPEND_FILE_OPS;
			}
			break;
		case 'N':
			output_file_name = optarg;
			break;
		case 'T':
			output_file_name = optarg;
			ops_number = SHORT_FILE_OPS;
			break;
		case 'U':
			output_file_name = optarg;
			ops_number = USB_FILE_OPS;
			break;
		case 'l':
			list_devs();
			return 0;
		case 'h':
			usage();
			return 0;
		case 'o':
			output_file_name = optarg;
			break;
		case 'r':
			start = true;
			break;
		case 't':
			stop = true;
			break;
		case 'p':
			char *ops_inval;
			long ops_result = strtol(optarg, &ops_inval, DECIMAL_SYSTEM);
			ops_number = (int)ops_result;
			break;
		case 'f':
			forced = true;
			break;
		}
	}

	if (!str_cmp(output_file_name, "") && start) {
		printf("Dumping destination was not specified. Specify with --outfile | -o\n");
		return 1;
	}

	printf("%s: HelenOS Packet Dumping utility: device - %d, ops - %d.\n", NAME, dev_number, ops_number);

	if (start) {

		if (file_exists(output_file_name) && !forced && ops_number != 2) {
			printf("File %s already exists. If you want to overwrite to it, then use flag --force.\n", output_file_name);
			return 0;
		}

		/* start with dev number and name */
		start_dumping(&dev_number, output_file_name, &ops_number);
	} else if (stop) {
		/* stop with dev number */
		stop_dumping(&dev_number);
	} else {
		usage();
		return 1;
	}
	return 0;
}

/** @}
 */
