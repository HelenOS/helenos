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
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <sys/typefmt.h>

#define NAME "devctl"

#define MAX_NAME_LENGTH 1024

char name[MAX_NAME_LENGTH];
char drv_name[MAX_NAME_LENGTH];

static int fun_subtree_print(devman_handle_t funh, int lvl)
{
	devman_handle_t devh;
	devman_handle_t *cfuns;
	size_t count, i;
	int rc;
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

static int fun_tree_print(void)
{
	devman_handle_t root_fun;
	int rc;

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

static int fun_online(const char *path)
{
	devman_handle_t funh;
	int rc;

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

static int fun_offline(const char *path)
{
	devman_handle_t funh;
	int rc;

	rc = devman_fun_get_handle(path, &funh, 0);
	if (rc != EOK) {
		printf(NAME ": Error resolving device function '%s' (%s)\n",
		    path, str_error(rc));
		return rc;
	}

	rc = devman_fun_offline(funh);
	if (rc != EOK) {
		printf(NAME ": Failed to offline function '%s'.\n", path);
		return rc;
	}

	return EOK;
}

static void print_syntax(void)
{
	printf("syntax: devctl [(online|offline) <function>]\n");
}

int main(int argc, char *argv[])
{
	int rc;

	if (argc == 1) {
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
	} else {
		printf(NAME ": Invalid argument '%s'.\n", argv[1]);
		print_syntax();
		return 1;
	}

	return 0;
}

/** @}
 */
