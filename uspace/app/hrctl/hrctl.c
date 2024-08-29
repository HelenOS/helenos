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

/** @addtogroup hrctl
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <getopt.h>
#include <hr.h>
#include <stdlib.h>
#include <stdio.h>
#include <str.h>
#include <str_error.h>

static void usage(void);

static const char usage_str[] =
    "Usage: hrctl [OPTION]... -n <dev_no> <devices>...\n"
    "\n"
    "Options:\n"
    "  -h, --help           display this help and exit\n"
    "  -s, --status         display status of active arrays\n"
    "  -a, --assemble=NAME  assemble an existing array\n"
    "  -c, --create=NAME    create new array\n"
    "  -n                   non-zero number of devices\n"
    "  -l, --level=LEVEL    set the RAID level,\n"
    "                       valid values: 0, 1, 5, linear\n"
    "  -0                   striping\n"
    "  -1                   mirroring\n"
    "  -5                   distributed parity\n"
    "  -L                   linear concatenation\n"
    "\n"
    "Example usage:\n"
    "  hrctl --create /hr0 -0 -n 2 devices/\\hw\\0 devices/\\hw\\1\n"
    "    - creates new mirroring RAID device named /hr0 consisting\n"
    "      of 2 drives\n"
    "  hrctl --assemble /hr0 -n 2 devices/\\hw\\0 devices/\\hw\\1\n"
    "    - assembles RAID device named /hr0 consisting of 2 drives,\n"
    "      that were previously in an array\n";

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "status", no_argument, 0, 's' },
	{ "assemble", required_argument, 0, 'a' },
	{ "create", required_argument, 0, 'c' },
	{ "level", required_argument, 0, 'l' },
	{ 0, 0, 0, 0 }
};

static void usage(void)
{
	printf("%s", usage_str);
}

static errno_t get_dev_svc_ids(int argc, char **argv, int optind, int dev_no,
    service_id_t **rdevs)
{
	errno_t rc;
	int i;
	size_t k;
	service_id_t *devs;

	devs = calloc(dev_no, sizeof(service_id_t));
	if (devs == NULL)
		return ENOMEM;

	for (i = optind, k = 0; i < argc; i++, k++) {
		rc = loc_service_get_id(argv[i], &devs[k], 0);
		if (rc != EOK) {
			printf("hrctl: error resolving device \"%s\"\n", argv[i]);
			free(devs);
			return EINVAL;
		}
	}

	*rdevs = devs;

	return EOK;
}

int main(int argc, char **argv)
{
	errno_t rc;
	int retval, c, dev_no;
	bool create, assemble;
	char *name = NULL;
	service_id_t *devs = NULL;
	hr_t *hr;
	hr_config_t hr_config;
	hr_level_t level;

	retval = 0;
	level = hr_l_empty;
	dev_no = 0;
	create = assemble = false;

	if (argc < 2) {
		goto bad;
	}

	c = 0;
	optreset = 1;
	optind = 0;

	while (c != -1) {
		c = getopt_long(argc, argv, "hsc:a:l:015Ln:",
		    long_options, NULL);
		switch (c) {
		case 'h':
			usage();
			return 0;
		case 's':
			printf("hrctl: status not implemented yet\n");
			return 0;
		case 'a':
			name = optarg;
			assemble = true;
			break;
		case 'c':
			name = optarg;
			create = true;
			break;
		case 'l':
			if (level != hr_l_empty)
				goto bad;
			if (str_cmp(optarg, "linear") == 0)
				level = hr_l_linear;
			else
				level = strtol(optarg, NULL, 10);
			break;
		case '0':
			if (level != hr_l_empty)
				goto bad;
			level = hr_l_0;
			break;
		case '1':
			if (level != hr_l_empty)
				goto bad;
			level = hr_l_1;
			break;
		case '5':
			if (level != hr_l_empty)
				goto bad;
			level = hr_l_5;
			break;
		case 'L':
			if (level != hr_l_empty)
				goto bad;
			level = hr_l_linear;
			break;
		case 'n':
			dev_no = strtol(optarg, NULL, 10);
			if (dev_no + optind != argc)
				goto bad;
			rc = get_dev_svc_ids(argc, argv, optind, dev_no, &devs);
			if (rc != EOK)
				return 1;
			break;
		}
	}

	if ((create && assemble) ||
	    (!create && !assemble) ||
	    (create && level == hr_l_empty) ||
	    (assemble && level != hr_l_empty) ||
	    (dev_no == 0)) {
		free(devs);
		goto bad;
	}

	hr_config.level = level;
	hr_config.dev_no = dev_no;
	hr_config.name = name;
	hr_config.devs = devs;

	rc = hr_sess_init(&hr);
	if (rc != EOK) {
		printf("hrctl: hr_sess_init() rc: %s\n", str_error(rc));
		retval = 1;
		goto end;
	}

	if (create) {
		rc = hr_create(hr, &hr_config);
		printf("hrctl: hr_create() rc: %s\n", str_error(rc));
	} else if (assemble) {
		printf("hrctl: assemble not implemented yet\n");
	}

end:
	free(devs);
	hr_sess_destroy(hr);
	return retval;
bad:
	printf("hrctl: bad usage, try hrctl --help\n");
	return 1;
}

/** @}
 */
