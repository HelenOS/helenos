/*
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup devctl
 * @{
 */
/** @file Control device framework (devman server).
 */

#include <devman.h>
#include <errno.h>
#include <io/table.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>

#define NAME "devctl"

#define MAX_NAME_LENGTH 1024

static char name[MAX_NAME_LENGTH];
static char drv_name[MAX_NAME_LENGTH];
static bool verbose = false;

static const char *drv_state_str(driver_state_t state)
{
	const char *sstate;

	switch (state) {
	case DRIVER_NOT_STARTED:
		sstate = "not started";
		break;
	case DRIVER_STARTING:
		sstate = "starting";
		break;
	case DRIVER_RUNNING:
		sstate = "running";
		break;
	default:
		sstate = "unknown";
	}

	return sstate;
}

static errno_t fun_subtree_print(devman_handle_t funh, int lvl)
{
	devman_handle_t devh;
	devman_handle_t *cfuns;
	size_t count, i;
	unsigned int score;
	errno_t rc;
	int j;

	for (j = 0; j < lvl; j++)
		printf("    ");

	rc = devman_fun_get_name(funh, name, MAX_NAME_LENGTH);
	if (rc != EOK)
		return ELIMIT;

	if (name[0] == '\0')
		str_cpy(name, MAX_NAME_LENGTH, "/");

	rc = devman_fun_get_driver_name(funh, drv_name, MAX_NAME_LENGTH);
	if (rc != EOK && rc != EINVAL)
		return ELIMIT;

	if (rc == EINVAL)
		printf("%s\n", name);
	else
		printf("%s : %s\n", name, drv_name);

	if (verbose) {
		for (i = 0; true; i++) {
			rc = devman_fun_get_match_id(funh, i, name, MAX_NAME_LENGTH,
			    &score);
			if (rc != EOK)
				break;

			for (j = 0; j < lvl; j++)
				printf("    ");

			printf("    %u %s\n", score, name);
		}
	}

	rc = devman_fun_get_child(funh, &devh);
	if (rc == ENOENT)
		return EOK;

	if (rc != EOK) {
		printf(NAME ": Failed getting child device for function "
		    "%s.\n", "xxx");
		return rc;
	}

	rc = devman_dev_get_functions(devh, &cfuns, &count);
	if (rc != EOK) {
		printf(NAME ": Failed getting list of functions for "
		    "device %s.\n", "xxx");
		return rc;
	}

	for (i = 0; i < count; i++)
		fun_subtree_print(cfuns[i], lvl + 1);

	free(cfuns);
	return EOK;
}

static errno_t fun_tree_print(void)
{
	devman_handle_t root_fun;
	errno_t rc;

	rc = devman_fun_get_handle("/", &root_fun, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving root function.\n");
		return EIO;
	}

	rc = fun_subtree_print(root_fun, 0);
	if (rc != EOK)
		return EIO;

	return EOK;
}

static errno_t fun_online(const char *path)
{
	devman_handle_t funh;
	errno_t rc;

	rc = devman_fun_get_handle(path, &funh, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device function '%s' (%s)\n",
		    path, str_error(rc));
		return rc;
	}

	rc = devman_fun_online(funh);
	if (rc != EOK) {
		printf(NAME ": Failed to online function '%s'.\n", path);
		return rc;
	}

	return EOK;
}

static errno_t fun_offline(const char *path)
{
	devman_handle_t funh;
	errno_t rc;

	rc = devman_fun_get_handle(path, &funh, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device function '%s' (%s)\n",
		    path, str_error(rc));
		return rc;
	}

	rc = devman_fun_offline(funh);
	if (rc != EOK) {
		printf(NAME ": Failed to offline function '%s' (%s)\n", path,
		    str_error(rc));
		return rc;
	}

	return EOK;
}

static errno_t drv_list(void)
{
	devman_handle_t *devs;
	devman_handle_t *drvs;
	driver_state_t state;
	const char *sstate;
	size_t ndrvs;
	size_t ndevs;
	size_t i;
	table_t *table = NULL;
	errno_t rc;

	rc = devman_get_drivers(&drvs, &ndrvs);
	if (rc != EOK)
		return rc;

	rc = table_create(&table);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		goto out;
	}

	table_header_row(table);
	table_printf(table, "Driver\t" "Devs\t" "State\n");

	for (i = 0; i < ndrvs; i++) {
		devs = NULL;

		rc = devman_driver_get_name(drvs[i], drv_name, MAX_NAME_LENGTH);
		if (rc != EOK)
			goto skip;
		rc = devman_driver_get_state(drvs[i], &state);
		if (rc != EOK)
			goto skip;
		rc = devman_driver_get_devices(drvs[i], &devs, &ndevs);
		if (rc != EOK)
			goto skip;

		sstate = drv_state_str(state);

		table_printf(table, "%s\t" "%zu\t" "%s\n", drv_name, ndevs, sstate);
	skip:
		free(devs);
	}

	rc = table_print_out(table, stdout);
	if (rc != EOK)
		printf("Error printing driver table.\n");
out:
	free(drvs);
	table_destroy(table);

	return rc;
}

static errno_t drv_show(char *drvname)
{
	devman_handle_t *devs;
	devman_handle_t drvh;
	devman_handle_t funh;
	driver_state_t state;
	const char *sstate;
	unsigned int score;
	size_t ndevs;
	size_t i;
	errno_t rc;

	rc = devman_driver_get_handle(drvname, &drvh);
	if (rc != EOK)
		return rc;

	devs = NULL;

	rc = devman_driver_get_name(drvh, drv_name, MAX_NAME_LENGTH);
	if (rc != EOK)
		return rc;

	rc = devman_driver_get_state(drvh, &state);
	if (rc != EOK)
		return rc;

	rc = devman_driver_get_devices(drvh, &devs, &ndevs);
	if (rc != EOK)
		return rc;

	sstate = drv_state_str(state);

	printf("Driver: %s\n", drv_name);
	printf("State: %s\n", sstate);

	printf("Attached devices:\n");

	for (i = 0; i < ndevs; i++) {
		rc = devman_dev_get_parent(devs[i], &funh);
		if (rc != EOK)
			goto error;

		rc = devman_fun_get_path(funh, name, MAX_NAME_LENGTH);
		if (rc != EOK)
			goto error;
		printf("\t%s\n", name);
	}

	printf("Match IDs:\n");

	for (i = 0; true; i++) {
		rc = devman_driver_get_match_id(drvh, i, name, MAX_NAME_LENGTH,
		    &score);
		if (rc != EOK)
			break;

		printf("\t%u %s\n", score, name);
	}

error:
	free(devs);

	return EOK;
}

static errno_t drv_load(const char *drvname)
{
	errno_t rc;
	devman_handle_t drvh;

	rc = devman_driver_get_handle(drvname, &drvh);
	if (rc != EOK) {
		printf("Failed resolving driver '%s': %s.\n", drvname, str_error(rc));
		return rc;
	}

	rc = devman_driver_load(drvh);
	if (rc != EOK) {
		printf("Failed loading driver '%s': %s.\n", drvname, str_error(rc));
		return rc;
	}

	return EOK;
}

static errno_t drv_unload(const char *drvname)
{
	errno_t rc;
	devman_handle_t drvh;

	rc = devman_driver_get_handle(drvname, &drvh);
	if (rc != EOK) {
		printf("Failed resolving driver '%s': %s.\n", drvname, str_error(rc));
		return rc;
	}

	rc = devman_driver_unload(drvh);
	if (rc != EOK) {
		printf("Failed unloading driver '%s': %s.\n", drvname, str_error(rc));
		return rc;
	}

	return EOK;
}

static void print_syntax(void)
{
	printf("syntax:\n");
	printf("\tdevctl\n");
	printf("\tdevctl online <function>]\n");
	printf("\tdevctl offline <function>]\n");
	printf("\tdevctl list-drv\n");
	printf("\tdevctl show-drv <driver-name>\n");
	printf("\tdevctl load-drv <driver-name>\n");
	printf("\tdevctl unload-drv <driver-name>\n");
}

int main(int argc, char *argv[])
{
	errno_t rc;

	if (argc == 1 || argv[1][0] == '-') {
		if (argc > 1) {
			if (str_cmp(argv[1], "-v") == 0) {
				verbose = true;
			} else {
				printf(NAME ": Invalid argument '%s'\n", argv[1]);
				print_syntax();
				return 1;
			}
		}
		rc = fun_tree_print();
		if (rc != EOK)
			return 2;
	} else if (str_cmp(argv[1], "online") == 0) {
		if (argc < 3) {
			printf(NAME ": Argument missing.\n");
			print_syntax();
			return 1;
		}

		rc = fun_online(argv[2]);
		if (rc != EOK) {
			return 2;
		}
	} else if (str_cmp(argv[1], "offline") == 0) {
		if (argc < 3) {
			printf(NAME ": Argument missing.\n");
			print_syntax();
			return 1;
		}

		rc = fun_offline(argv[2]);
		if (rc != EOK) {
			return 2;
		}
	} else if (str_cmp(argv[1], "list-drv") == 0) {
		rc = drv_list();
		if (rc != EOK)
			return 2;
	} else if (str_cmp(argv[1], "show-drv") == 0) {
		if (argc < 3) {
			printf(NAME ": Argument missing.\n");
			print_syntax();
			return 1;
		}

		rc = drv_show(argv[2]);
		if (rc != EOK) {
			return 2;
		}
	} else if (str_cmp(argv[1], "load-drv") == 0) {
		if (argc < 3) {
			printf(NAME ": Argument missing.\n");
			print_syntax();
			return 1;
		}

		rc = drv_load(argv[2]);
		if (rc != EOK)
			return 2;
	} else if (str_cmp(argv[1], "unload-drv") == 0) {
		if (argc < 3) {
			printf(NAME ": Argument missing.\n");
			print_syntax();
			return 1;
		}

		rc = drv_unload(argv[2]);
		if (rc != EOK)
			return 2;
	} else {
		printf(NAME ": Invalid argument '%s'.\n", argv[1]);
		print_syntax();
		return 1;
	}

	return 0;
}

/** @}
 */
