/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup mkext4
 * @{
 */

/**
 * @file
 * @brief Tool for creating new Ext4 file systems.
 */

#include <errno.h>
#include <ext4/filesystem.h>
#include <loc.h>
#include <stdio.h>
#include <str.h>

#define NAME	"mkext4"

static void syntax_print(void);
static errno_t ext4_version_parse(const char *, ext4_cfg_ver_t *);

int main(int argc, char **argv)
{
	errno_t rc;
	char *dev_path;
	service_id_t service_id;
	ext4_cfg_t cfg;
	char *endptr;
	aoff64_t nblocks;
	const char *label = "";
	unsigned int bsize = 4096;

	cfg.version = ext4_def_fs_version;

	if (argc < 2) {
		printf(NAME ": Error, argument missing.\n");
		syntax_print();
		return 1;
	}

	--argc;
	++argv;

	while (*argv && *argv[0] == '-') {
		if (str_cmp(*argv, "--size") == 0) {
			--argc;
			++argv;
			if (*argv == NULL) {
				printf(NAME ": Error, argument missing.\n");
				syntax_print();
				return 1;
			}

			nblocks = strtol(*argv, &endptr, 10);
			if (*endptr != '\0') {
				printf(NAME ": Error, invalid argument.\n");
				syntax_print();
				return 1;
			}

			--argc;
			++argv;
			continue;
		}

		if (str_cmp(*argv, "--bsize") == 0) {
			--argc;
			++argv;
			if (*argv == NULL) {
				printf(NAME ": Error, argument missing.\n");
				syntax_print();
				return 1;
			}

			bsize = strtol(*argv, &endptr, 10);
			if (*endptr != '\0') {
				printf(NAME ": Error, invalid argument.\n");
				syntax_print();
				return 1;
			}

			--argc;
			++argv;
			continue;
		}

		if (str_cmp(*argv, "--type") == 0) {
			--argc;
			++argv;
			if (*argv == NULL) {
				printf(NAME ": Error, argument missing.\n");
				syntax_print();
				return 1;
			}

			rc = ext4_version_parse(*argv, &cfg.version);
			if (rc != EOK) {
				printf(NAME ": Error, invalid argument.\n");
				syntax_print();
				return 1;
			}

			--argc;
			++argv;
			continue;
		}

		if (str_cmp(*argv, "--label") == 0) {
			--argc;
			++argv;
			if (*argv == NULL) {
				printf(NAME ": Error, argument missing.\n");
				syntax_print();
				return 1;
			}

			label = *argv;

			--argc;
			++argv;
			continue;
		}

		if (str_cmp(*argv, "--help") == 0) {
			syntax_print();
			return 0;
		}

		if (str_cmp(*argv, "-") == 0) {
			--argc;
			++argv;
			break;
		} else {
			printf(NAME ": Invalid argument: %s\n", *argv);
			syntax_print();
			return 1;
		}
	}

	if (argc != 1) {
		printf(NAME ": Error, unexpected argument.\n");
		syntax_print();
		return 1;
	}

	dev_path = *argv;
	printf("Device: %s\n", dev_path);

	rc = loc_service_get_id(dev_path, &service_id, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device `%s'.\n", dev_path);
		return 2;
	}

	cfg.volume_name = label;
	cfg.bsize = bsize;
	(void) nblocks;

	rc = ext4_filesystem_create(&cfg, service_id);
	if (rc != EOK) {
		printf(NAME ": Error initializing file system.\n");
		return 3;
	}

	printf("Success.\n");

	return 0;
}

static void syntax_print(void)
{
	printf("syntax: mkext4 [<options>...] <device_name>\n");
	printf("options:\n"
	    "\t--size <sectors> Filesystem size, overrides device size\n"
	    "\t--label <label>  Volume label\n"
	    "\t--type <fstype>  Filesystem type (ext2, ext2old)\n"
	    "\t--bsize <bytes>  Filesystem block size in bytes (default = 4096)\n");
}

static errno_t ext4_version_parse(const char *str, ext4_cfg_ver_t *ver)
{
	if (str_cmp(str, "ext2old") == 0) {
		*ver = extver_ext2_old;
		return EOK;
	}

	if (str_cmp(str, "ext2") == 0) {
		*ver = extver_ext2;
		return EOK;
	}

	return EINVAL;
}

/**
 * @}
 */
