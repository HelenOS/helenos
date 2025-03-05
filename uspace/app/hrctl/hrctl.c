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
#include <sif.h>
#include <stdlib.h>
#include <stdio.h>
#include <str.h>
#include <str_error.h>

#define HRCTL_SAMPLE_CONFIG_PATH "/cfg/sample_hr_config.sif"

static void usage(void);
static errno_t fill_config_devs(int, char **, int, hr_config_t *);
static errno_t load_config(const char *, hr_config_t *);

static const char usage_str[] =
    "Usage: hrctl [OPTION]... -n <dev_no> <devices>...\n"
    "\n"
    "Options:\n"
    "  -h, --help                display this help and exit\n"
    "  -C, --create-file=PATH    create an array from file,\n"
    "                            sample file at: " HRCTL_SAMPLE_CONFIG_PATH "\n"
    "  -A, --assemble-file=PATH  create an array from file\n"
    "  -s, --status              display status of active arrays\n"
    "  -H, --hotspare=DEV        add hotspare extent\n"
    "  -D, --destroy             destroy/disassemble an active array\n"
    "  -F, --fail-extent         fail an extent, use with -D and set it before\n"
    "  -c, --create=NAME         create new array\n"
    "  -a, --assemble=NAME       assemble an existing array\n"
    "  -n                        non-zero number of devices\n"
    "  -l, --level=LEVEL         set the RAID level,\n"
    "                            valid values: 0, 1, 4, 5\n"
    "  -0                        striping\n"
    "  -1                        mirroring\n"
    "  -4                        parity on one extent\n"
    "  -5                        distributed parity\n"
    "\n"
    "When specifying name for creation or assembly, the device name\n"
    "is automatically prepended with \"devices/\" prefix.\n"
    "\n"
    "Example usage:\n"
    "  hrctl --create hr0 -0 -n 2 devices/\\hw\\0 devices/\\hw\\1\n"
    "    - creates new mirroring RAID device named /hr0 consisting\n"
    "      of 2 drives\n"
    "  hrctl --assemble hr0 -n 2 devices/\\hw\\0 devices/\\hw\\1\n"
    "    - assembles RAID device named /hr0 consisting of 2 drives,\n"
    "      that were previously in an array\n"
    "  hrctl devices/hr0 --hotspare=devices/disk10\n"
    "    - adds \"devices/disk10\" as hotspare extent\n"
    "  hrctl -F 0 -D devices/hr0\n"
    "    - marks first extent as FAILED\n"
    "Limitations:\n"
    "  - device name must be less than 32 characters in size\n";

static struct option const long_options[] = {
	{ "help", no_argument, 0, 'h' },
	{ "status", no_argument, 0, 's' },
	{ "assemble", required_argument, 0, 'a' },
	{ "create", required_argument, 0, 'c' },
	{ "level", required_argument, 0, 'l' },
	{ "create-file", required_argument, 0, 'C' },
	{ "assemble-file", required_argument, 0, 'A' },
	{ "destroy", required_argument, 0, 'D' },
	{ "fail-extent", required_argument, 0, 'F' },
	{ "hotspare", required_argument, 0, 'H' },
	{ 0, 0, 0, 0 }
};

static void usage(void)
{
	printf("%s", usage_str);
}

static errno_t fill_config_devs(int argc, char **argv, int optind,
    hr_config_t *cfg)
{
	errno_t rc;
	size_t i;

	for (i = 0; i < cfg->dev_no; i++) {
		rc = loc_service_get_id(argv[optind], &cfg->devs[i], 0);
		if (rc == ENOENT) {
			printf("hrctl: no device \"%s\", marking as missing\n",
			    argv[optind]);
			cfg->devs[i] = 0;
		} else if (rc != EOK) {
			printf("hrctl: error resolving device \"%s\", aborting\n",
			    argv[optind]);
			return EINVAL;
		}

		optind++;
	}

	return EOK;
}

static errno_t load_config(const char *path, hr_config_t *cfg)
{
	errno_t rc;
	size_t i;
	sif_doc_t *doc = NULL;
	sif_node_t *narrays;
	sif_node_t *rnode;
	sif_node_t *narray;
	sif_node_t *nextent;
	const char *ntype;
	const char *devname;
	const char *level_str;
	const char *dev_no_str;
	const char *extent_devname;

	rc = sif_load(path, &doc);
	if (rc != EOK)
		goto error;

	rnode = sif_get_root(doc);

	narrays = sif_node_first_child(rnode);
	ntype = sif_node_get_type(narrays);
	if (str_cmp(ntype, "arrays") != 0) {
		rc = EIO;
		goto error;
	}

	narray = sif_node_first_child(narrays);
	ntype = sif_node_get_type(narray);
	if (str_cmp(ntype, "array") != 0) {
		rc = EIO;
		goto error;
	}

	devname = sif_node_get_attr(narray, "devname");
	if (devname == NULL) {
		rc = EIO;
		goto error;
	}
	str_cpy(cfg->devname, sizeof(cfg->devname), devname);

	level_str = sif_node_get_attr(narray, "level");
	if (level_str == NULL)
		cfg->level = HR_LVL_UNKNOWN;
	else
		cfg->level = strtol(level_str, NULL, 10);

	dev_no_str = sif_node_get_attr(narray, "n");
	if (dev_no_str == NULL) {
		rc = EIO;
		goto error;
	}
	cfg->dev_no = strtol(dev_no_str, NULL, 10);

	nextent = sif_node_first_child(narray);
	for (i = 0; i < cfg->dev_no; i++) {
		if (nextent == NULL) {
			rc = EINVAL;
			goto error;
		}

		ntype = sif_node_get_type(nextent);
		if (str_cmp(ntype, "extent") != 0) {
			rc = EIO;
			goto error;
		}

		extent_devname = sif_node_get_attr(nextent, "devname");
		if (extent_devname == NULL) {
			rc = EIO;
			goto error;
		}

		rc = loc_service_get_id(extent_devname, &cfg->devs[i], 0);
		if (rc == ENOENT) {
			printf("hrctl: no device \"%s\", marking as missing\n",
			    extent_devname);
			cfg->devs[i] = 0;
			rc = EOK;
		} else if (rc != EOK) {
			printf("hrctl: error resolving device \"%s\", aborting\n",
			    extent_devname);
			return EINVAL;
		}

		nextent = sif_node_next_child(nextent);
	}

error:
	if (doc != NULL)
		sif_delete(doc);
	return rc;
}

int main(int argc, char **argv)
{
	errno_t rc;
	int retval, c;
	bool create, assemble;
	long fail_extent = -1;
	hr_t *hr = NULL;
	hr_config_t *cfg;

	cfg = calloc(1, sizeof(hr_config_t));
	if (cfg == NULL)
		return 1;

	retval = 0;
	cfg->level = HR_LVL_UNKNOWN;
	cfg->dev_no = 0;
	create = assemble = false;

	if (argc < 2) {
		goto bad;
	}

	c = 0;
	optreset = 1;
	optind = 0;

	while (c != -1) {
		c = getopt_long(argc, argv, "hsC:c:A:a:l:0145Ln:D:F:H:",
		    long_options, NULL);
		switch (c) {
		case 's':
			free(cfg);
			rc = hr_print_status();
			if (rc != EOK)
				return 1;
			return 0;
		case 'C':
			/* only support 1 array inside config for now XXX */
			rc = load_config(optarg, cfg);
			if (rc != EOK) {
				printf("hrctl: failed to load config\n");
				free(cfg);
				return 1;
			}
			create = true;
			goto skip;
		case 'c':
			if (str_size(optarg) > sizeof(cfg->devname) - 1) {
				printf("hrctl: device name too long\n");
				free(cfg);
				return 1;
			}
			str_cpy(cfg->devname, sizeof(cfg->devname), optarg);
			create = true;
			break;
		case 'A':
			rc = load_config(optarg, cfg);
			if (rc != EOK) {
				printf("hrctl: failed to load config\n");
				free(cfg);
				return 1;
			}
			assemble = true;
			goto skip;
		case 'a':
			if (str_size(optarg) > sizeof(cfg->devname) - 1) {
				printf("hrctl: device name too long\n");
				free(cfg);
				return 1;
			}
			str_cpy(cfg->devname, sizeof(cfg->devname), optarg);
			assemble = true;
			break;
		case 'D':
			rc = hr_stop(optarg, fail_extent);
			free(cfg);
			if (rc != EOK) {
				if (rc == ENOENT)
					printf("hrctl: service named \"%s\" does not exist\n",
					    optarg);
				return 1;
			}
			return 0;
		case 'F':
			fail_extent = strtol(optarg, NULL, 10);
			break;
		case 'l':
			if (cfg->level != HR_LVL_UNKNOWN)
				goto bad;
			cfg->level = strtol(optarg, NULL, 10);
			break;
		case '0':
			if (cfg->level != HR_LVL_UNKNOWN)
				goto bad;
			cfg->level = HR_LVL_0;
			break;
		case '1':
			if (cfg->level != HR_LVL_UNKNOWN)
				goto bad;
			cfg->level = HR_LVL_1;
			break;
		case '4':
			if (cfg->level != HR_LVL_UNKNOWN)
				goto bad;
			cfg->level = HR_LVL_4;
			break;
		case '5':
			if (cfg->level != HR_LVL_UNKNOWN)
				goto bad;
			cfg->level = HR_LVL_5;
			break;
		case 'n':
			cfg->dev_no = strtol(optarg, NULL, 10);
			if ((int)cfg->dev_no + optind != argc)
				goto bad;
			rc = fill_config_devs(argc, argv, optind, cfg);
			if (rc != EOK) {
				free(cfg);
				return 1;
			}
			goto skip;
		case 'H':
			if (optind != 3 && argc != 4)
				goto bad;

			service_id_t hotspare;
			service_id_t vol_svc_id;

			rc = loc_service_get_id(argv[1], &vol_svc_id, 0);
			if (rc != EOK) {
				printf("hrctl: error resolving volume \"%s\", "
				    "aborting extent addition\n", argv[1]);
				goto bad;
			}

			rc = loc_service_get_id(optarg, &hotspare, 0);
			if (rc != EOK) {
				printf("hrctl: error resolving device \"%s\", "
				    "aborting extent addition\n", optarg);
				goto bad;
			}

			rc = hr_add_hotspare(vol_svc_id, hotspare);
			if (rc != EOK)
				printf("hrctl: hr_add_hotspare() rc: %s\n",
				    str_error(rc));

			free(cfg);
			if (rc != EOK)
				return 1;
			else
				return 0;
		case 'h':
		default:
			usage();
			free(cfg);
			return 0;
		}
	}

skip:
	if ((create && assemble) || (!create && !assemble))
		goto bad;

	if (create && cfg->level == HR_LVL_UNKNOWN) {
		printf("hrctl: invalid level, exiting\n");
		goto bad;
	}

	if (cfg->dev_no > HR_MAX_EXTENTS) {
		printf("hrctl: too many devices, exiting\n");
		goto bad;
	}

	if (cfg->dev_no == 0) {
		printf("hrctl: invalid number of devices, exiting\n");
		goto bad;
	}

	rc = hr_sess_init(&hr);
	if (rc != EOK) {
		printf("hrctl: hr_sess_init() rc: %s\n", str_error(rc));
		retval = 1;
		goto end;
	}

	if (create) {
		rc = hr_create(hr, cfg, false);
		printf("hrctl: hr_create() rc: %s\n", str_error(rc));
	} else if (assemble) {
		rc = hr_create(hr, cfg, true);
		printf("hrctl: hr_assemble() rc: %s\n", str_error(rc));
	}

end:
	free(cfg);
	hr_sess_destroy(hr);
	return retval;
bad:
	free(cfg);
	printf("hrctl: bad usage, try hrctl --help\n");
	return 1;
}

/** @}
 */
