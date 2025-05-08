/*
 * Copyright (c) 2025 Miroslav Cimerman
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

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <hr.h>
#include <sif.h>
#include <stdlib.h>
#include <stdio.h>
#include <str.h>
#include <str_error.h>

/* #define HRCTL_SAMPLE_CONFIG_PATH "/cfg/sample_hr_config.sif" */

#define NAME "hrctl"

static void	usage(void);
static errno_t	fill_config_devs(int, char **, hr_config_t *);
static errno_t get_vol_configs_from_sif(const char *, hr_config_t **, size_t *);

static const char usage_str[] =
    "Usage: hrctl [OPTION]...\n"
    "\n"
    "Options:\n"
    "  -h, --help                                Display this message and exit.\n"
    "\n"
    "  -c, --create                              Create a volume, options:\n"
    "      name {-l , --level level} device...   manual device specification, or\n"
    "      -f configuration.sif                  create from configuration file.\n"
    "\n"
    "  -a, --assemble                            Assemble volume(s), options:\n"
    "      [device...]                           manual device specification, or\n"
    "      [-f configuration.sif]                assemble from configuration file, or\n"
    "                                            no option is automatic assembly.\n"
    "\n"
    "  -d, --disassemble                         Deactivate/disassemble, options:\n"
    "      [volume]                              specific volume, or\n"
    "                                            all volumes with no specified option.\n"
    "\n"
    "  -m, --modify volume                       Modify a volume, options:\n"
    "          -f, --fail index                  fail an extent (DANGEROUS), or\n"
    "          -h, --hotspare device             add hotspare.\n"
    "\n"
    "  -s, --state                               Display state of active volumes.\n"
    "\n"
    "Example usage:\n"
    "\t\thrctl --create hr0 --level 5 disk1 disk2 disk3\n"
    "\t\thrctl -c hr0 -l 5 disk1 disk2 disk3\n"
    "\t\thrctl -c -f cfg.sif\n"
    "\t\thrctl --assemble disk1 disk2 disk3\n"
    "\t\thrctl -a\n"
    "\t\thrctl -d devices/hr0\n"
    "\t\thrctl -d\n"
    "\t\thrctl --modify devices/hr0 --fail 0\n"
    "\t\thrctl --modify devices/hr0 --hotspare disk4\n"
    "\t\thrctl -s\n"
    "\n"
    "Notes:\n"
    "  Volume service names are automatically prepended with \"devices/\" prefix.\n"
    "  Simulating an extent failure with -m volume -f index is dangerous. It marks\n"
    "  metadata as dirty in other healthy extents, and therefore invalidates\n"
    "  the specified extent.\n"
    "  Nested levels have to be created manually, or from config file, but need to\n"
    "  be specified as separate volumes.\n"
    "\n"
    "Limitations:\n"
    "\t- volume name must be shorter than 32 characters\n"
    "\t- automatic assembly and disassembly on nested volumes is UNDEFINED!\n";

static void usage(void)
{
	printf("%s", usage_str);
}

static errno_t fill_config_devs(int argc, char **argv, hr_config_t *cfg)
{
	errno_t rc;
	size_t i;

	for (i = 0; i < HR_MAX_EXTENTS && optind < argc; i++) {
		rc = loc_service_get_id(argv[optind], &cfg->devs[i], 0);
		if (rc == ENOENT) {
			printf(NAME ": device \"%s\" not found, aborting\n",
			    argv[optind]);
			return ENOENT;
		} else if (rc != EOK) {
			printf(NAME ": error resolving device \"%s\", aborting\n",
			    argv[optind]);
			return EINVAL;
		}
		optind++;
	}

	if (optind < argc) {
		printf(NAME ": too many devices specified, max = %u\n",
		    HR_MAX_EXTENTS);
		return ELIMIT;
	}

	cfg->dev_no = i;

	return EOK;
}

static errno_t get_vol_configs_from_sif(const char *path, hr_config_t **rcfgs,
    size_t *rcount)
{
	errno_t rc;
	sif_doc_t *doc = NULL;
	sif_node_t *hrconfig_node;
	sif_node_t *root_node;
	sif_node_t *volume_node;
	sif_node_t *nextent;
	const char *ntype;
	const char *devname;
	const char *level_str;
	const char *extent_devname;
	hr_config_t *vol_configs = NULL;

	rc = sif_load(path, &doc);
	if (rc != EOK)
		goto error;

	root_node = sif_get_root(doc);

	hrconfig_node = sif_node_first_child(root_node);
	ntype = sif_node_get_type(hrconfig_node);
	if (str_cmp(ntype, "hrconfig") != 0) {
		rc = EINVAL;
		goto error;
	}

	size_t vol_count = 0;
	volume_node = sif_node_first_child(hrconfig_node);
	while (volume_node) {
		ntype = sif_node_get_type(volume_node);
		if (str_cmp(ntype, "volume") != 0) {
			rc = EINVAL;
			goto error;
		}
		vol_configs = realloc(vol_configs,
		    (vol_count + 1) * sizeof(hr_config_t));
		hr_config_t *cfg = vol_configs + vol_count;

		devname = sif_node_get_attr(volume_node, "devname");
		if (devname == NULL) {
			rc = EINVAL;
			goto error;
		}
		str_cpy(cfg->devname, sizeof(cfg->devname), devname);

		level_str = sif_node_get_attr(volume_node, "level");
		if (level_str == NULL)
			cfg->level = HR_LVL_UNKNOWN;
		else
			cfg->level = strtol(level_str, NULL, 10);

		nextent = sif_node_first_child(volume_node);
		size_t i = 0;
		while (nextent && i < HR_MAX_EXTENTS) {
			ntype = sif_node_get_type(nextent);
			if (str_cmp(ntype, "extent") != 0) {
				rc = EINVAL;
				goto error;
			}

			extent_devname = sif_node_get_attr(nextent, "devname");
			if (extent_devname == NULL) {
				rc = EINVAL;
				goto error;
			}

			rc = loc_service_get_id(extent_devname, &cfg->devs[i], 0);
			if (rc == ENOENT) {
				printf(NAME ": no device \"%s\", marking as missing\n",
				    extent_devname);
				cfg->devs[i] = 0;
				rc = EOK;
			} else if (rc != EOK) {
				printf(NAME ": error resolving device \"%s\", aborting\n",
				    extent_devname);
				goto error;
			}

			nextent = sif_node_next_child(nextent);
			i++;
		}

		if (i > HR_MAX_EXTENTS) {
			printf(NAME ": too many devices specified in volume \"%s\", "
			    "skipping\n", devname);
			memset(&vol_configs[vol_count], 0, sizeof(hr_config_t));
		} else {
			cfg->dev_no = i;
			vol_count++;
		}

		volume_node = sif_node_next_child(volume_node);
	}

	if (rc == EOK) {
		if (rcount)
			*rcount = vol_count;
		if (rcfgs)
			*rcfgs = vol_configs;
	}
error:
	if (doc != NULL)
		sif_delete(doc);
	if (rc != EOK) {
		if (vol_configs)
			free(vol_configs);
	}
	return rc;
}

static int create_from_config(hr_t *hr, const char *config_path)
{
	hr_config_t *vol_configs = NULL;
	size_t vol_count = 0;
	errno_t rc = get_vol_configs_from_sif(config_path, &vol_configs,
	    &vol_count);
	if (rc != EOK) {
		printf(NAME ": config parsing failed\n");
		return EXIT_FAILURE;
	}

	for (size_t i = 0; i < vol_count; i++) {
		rc = hr_create(hr, &vol_configs[i]);
		if (rc != EOK) {
			printf(NAME ": creation of volume \"%s\" failed: %s, "
			    "but continuing\n",
			    vol_configs[i].devname, str_error(rc));
		} else {
			printf(NAME ": volume \"%s\" successfully created\n",
			    vol_configs[i].devname);
		}
	}

	free(vol_configs);
	return EXIT_SUCCESS;
}

static int create_from_argv(hr_t *hr, int argc, char **argv)
{
	/* we need name + --level + arg + at least one extent */
	if (optind + 3 >= argc) {
		printf(NAME ": not enough arguments\n");
		return EXIT_FAILURE;
	}

	hr_config_t *vol_config = calloc(1, sizeof(hr_config_t));
	if (vol_config == NULL) {
		printf(NAME ": not enough memory\n");
		return EXIT_FAILURE;
	}

	const char *name = argv[optind++];
	if (str_size(name) >= HR_DEVNAME_LEN) {
		printf(NAME ": devname must be less then 32 bytes.\n");
		goto error;
	}

	str_cpy(vol_config->devname, HR_DEVNAME_LEN, name);

	const char *level_opt = argv[optind++];
	if (str_cmp(level_opt, "--level") != 0 &&
	    str_cmp(level_opt, "-l") != 0) {
		printf(NAME ": unknown option \"%s\"\n", level_opt);
		goto error;
	}

	const char *level_str = argv[optind++];
	if (str_size(level_str) != 1 && !isdigit(level_str[0])) {
		printf(NAME ": unknown level \"%s\"\n", level_str);
		goto error;
	}

	vol_config->level = strtol(level_str, NULL, 10);

	errno_t rc = fill_config_devs(argc, argv, vol_config);
	if (rc != EOK)
		goto error;

	rc = hr_create(hr, vol_config);
	if (rc != EOK) {
		printf(NAME ": creation failed: %s\n", str_error(rc));
		goto error;
	} else {
		printf(NAME ": volume \"%s\" successfully created\n",
		    vol_config->devname);
	}

	free(vol_config);
	return EXIT_SUCCESS;
error:
	free(vol_config);
	return EXIT_FAILURE;
}

static int handle_create(hr_t *hr, int argc, char **argv)
{
	int rc;

	if (optind >= argc) {
		printf(NAME ": no arguments to --create\n");
		return EXIT_FAILURE;
	}

	if (str_cmp(argv[optind], "-f") == 0) {
		optind++;
		if (optind >= argc) {
			printf(NAME ": not enough arguments\n");
			return EXIT_FAILURE;
		}

		const char *config_path = argv[optind++];

		if (optind < argc) {
			printf(NAME ": unexpected arguments\n");
			return EXIT_FAILURE;
		}

		rc = create_from_config(hr, config_path);
	} else {
		rc = create_from_argv(hr, argc, argv);
	}

	return rc;
}

static int assemble_from_config(hr_t *hr, const char *config_path)
{
	hr_config_t *vol_configs = NULL;
	size_t vol_count = 0;
	errno_t rc = get_vol_configs_from_sif(config_path, &vol_configs,
	    &vol_count);
	if (rc != EOK) {
		printf(NAME ": config parsing failed\n");
		return EXIT_FAILURE;
	}

	size_t cnt = 0;
	for (size_t i = 0; i < vol_count; i++) {
		size_t tmpcnt = 0;
		(void)hr_assemble(hr, &vol_configs[i], &tmpcnt);
		cnt += tmpcnt;
	}

	printf(NAME ": assembled %zu volumes\n", cnt);

	free(vol_configs);
	return EXIT_SUCCESS;
}

static int assemble_from_argv(hr_t *hr, int argc, char **argv)
{
	hr_config_t *vol_config = calloc(1, sizeof(hr_config_t));
	if (vol_config == NULL) {
		printf(NAME ": not enough memory\n");
		return ENOMEM;
	}

	errno_t rc = fill_config_devs(argc, argv, vol_config);
	if (rc != EOK)
		goto error;

	size_t cnt;
	rc = hr_assemble(hr, vol_config, &cnt);
	if (rc != EOK) {
		printf(NAME ": assmeble failed: %s\n", str_error(rc));
		goto error;
	}

	printf("hrctl: assembled %zu volumes\n", cnt);

	free(vol_config);
	return EXIT_SUCCESS;
error:
	free(vol_config);
	return EXIT_FAILURE;
}

static int handle_assemble(hr_t *hr, int argc, char **argv)
{
	int rc;

	if (optind >= argc) {
		size_t cnt;
		errno_t rc = hr_auto_assemble(hr, &cnt);
		if (rc != EOK) {
			/* XXX: here have own error codes */
			printf("hrctl: auto assemble rc: %s\n", str_error(rc));
			return EXIT_FAILURE;
		}

		printf(NAME ": auto assembled %zu volumes\n", cnt);
		return EXIT_SUCCESS;
	}

	if (str_cmp(argv[optind], "-f") == 0) {
		if (++optind >= argc) {
			printf(NAME ": not enough arguments\n");
			return EXIT_FAILURE;
		}
		const char *config_path = argv[optind++];

		if (optind < argc) {
			printf(NAME ": unexpected arguments\n");
			return EXIT_FAILURE;
		}

		rc = assemble_from_config(hr, config_path);
	} else {
		rc = assemble_from_argv(hr, argc, argv);
	}

	return rc;
}

static int handle_disassemble(hr_t *hr, int argc, char **argv)
{
	if (optind >= argc) {
		errno_t rc = hr_stop_all(hr);
		if (rc != EOK) {
			printf(NAME ": stopping some volumes failed: %s\n",
			    str_error(rc));
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}

	if (optind + 1 < argc) {
		printf(NAME ": only 1 device can be manually specified\n");
		return EXIT_FAILURE;
	}

	const char *devname = argv[optind++];

	errno_t rc = hr_stop(hr, devname);
	if (rc != EOK) {
		printf(NAME ": disassembly of device \"%s\" failed: %s\n",
		    devname, str_error(rc));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int handle_modify(hr_t *hr, int argc, char **argv)
{
	if (optind >= argc) {
		printf(NAME ": no arguments to --modify\n");
		return EXIT_FAILURE;
	}

	const char *volname = argv[optind++];

	/* at least 1 option and its agument */
	if (optind + 1 >= argc) {
		printf(NAME ": not enough arguments\n");
		return EXIT_FAILURE;
	}

	if (optind + 2 < argc) {
		printf(NAME ": unexpected arguments\n");
		return EXIT_FAILURE;
	}

	if (str_cmp(argv[optind], "--fail") == 0 ||
	    str_cmp(argv[optind], "-f") == 0) {
		optind++;
		unsigned long extent = strtol(argv[optind++], NULL, 10);
		errno_t rc = hr_fail_extent(hr, volname, extent);
		if (rc != EOK) {
			printf(NAME ": failing extent failed: %s\n",
			    str_error(rc));
			return EXIT_FAILURE;
		}
	} else if (str_cmp(argv[optind], "--hotspare") == 0 ||
	    str_cmp(argv[optind], "-h") == 0) {
		optind++;
		errno_t rc = hr_add_hotspare(hr, volname, argv[optind++]);
		if (rc != EOK) {
			printf(NAME ": adding hotspare failed: %s\n",
			    str_error(rc));
			return EXIT_FAILURE;
		}
	} else {
		printf(NAME ": unknown argument\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int handle_state(hr_t *hr, int argc, char **argv)
{
	(void)argc;
	(void)argv;

	errno_t rc = hr_print_state(hr);
	if (rc != EOK) {
		printf(NAME ": state printing failed: %s\n", str_error(rc));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	int rc = EXIT_SUCCESS;
	hr_t *hr = NULL;

	if (argc < 2) {
		rc = EXIT_FAILURE;
		goto end;
	}

	rc = hr_sess_init(&hr);
	if (rc != EOK) {
		printf(NAME ": hr server session init failed: %s\n",
		    str_error(rc));
		return EXIT_FAILURE;
	}

	optreset = 1;
	optind = 0;

	struct option const top_level_opts[] = {
		{ "help",		no_argument, 0, 'h' },
		{ "create",		no_argument, 0, 'c' },
		{ "assemble",		no_argument, 0, 'a' },
		{ "disassemble",	no_argument, 0, 'd' },
		{ "modify",		no_argument, 0, 'm' },
		{ "state",		no_argument, 0, 's' },
		{ 0, 0, 0, 0 }
	};

	int c = getopt_long(argc, argv, "hcadms", top_level_opts, NULL);
	switch (c) {
	case 'h':
		usage();
		goto end;
	case 'c':
		rc = handle_create(hr, argc, argv);
		goto end;
	case 'a':
		rc = handle_assemble(hr, argc, argv);
		goto end;
	case 'd':
		rc = handle_disassemble(hr, argc, argv);
		goto end;
	case 'm':
		rc = handle_modify(hr, argc, argv);
		goto end;
	case 's':
		rc = handle_state(hr, argc, argv);
		goto end;
	default:
		goto end;
	}

end:
	hr_sess_destroy(hr);

	if (rc != EXIT_SUCCESS)
		printf(NAME ": use --help to see usage\n");
	return rc;
}

/** @}
 */
