/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup ofw
 * @{
 */

/**
 * @file
 * @brief Tool for printing the OpenFirmware device tree
 */

#include <errno.h>
#include <ofw.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

#define NAME	"ofw"

#define MAX_NAME_LENGTH 1024

static void syntax_print(void);
static errno_t ofw_print_subtree(const char *, bool);
static errno_t ofw_print_properties(const char *);

int main(int argc, char **argv)
{
	errno_t rc;
	const char *path = "/";
	bool verbose = false;

	--argc;
	++argv;

	while (argc > 0 && argv[0][0] == '-') {
		if (str_cmp(argv[0], "-p") == 0) {
			--argc;
			++argv;
			if (argc < 1) {
				printf("Option argument missing.\n");
				return 1;
			}

			path = argv[0];
			--argc;
			++argv;
		} else if (str_cmp(argv[0], "-v") == 0) {
			--argc;
			++argv;

			verbose = true;
		} else {
			syntax_print();
			return 1;
		}
	}

	if (argc != 0) {
		syntax_print();
		return 1;
	}

	rc = ofw_print_subtree(path, verbose);
	if (rc != EOK)
		return 1;

	return 0;
}

static void syntax_print(void)
{
	printf("syntax: %s [<options>]\n", NAME);
	printf("options:\n"
	    "\t-v        Verbose mode (print properties and their values)\n"
	    "\t-p <path> Only print devices under <path>\n");
}

/** List OpenFirmware device nodes under a specific node.
 *
 * @param path Path of node where to start printing
 * @param verbose If @c true, print properties
 * @return EOK on success or an error code
 */
static errno_t ofw_print_subtree(const char *path, bool verbose)
{
	char *subpath;
	ofw_child_it_t it;
	errno_t rc;

	printf("%s\n", path);

	if (verbose) {
		rc = ofw_print_properties(path);
		if (rc != EOK)
			return rc;
	}

	rc = ofw_child_it_first(&it, path);
	while (!ofw_child_it_end(&it)) {
		rc = ofw_child_it_get_path(&it, &subpath);
		if (rc != EOK)
			goto error;

		rc = ofw_print_subtree(subpath, verbose);
		if (rc != EOK)
			goto error;

		free(subpath);
		subpath = NULL;

		ofw_child_it_next(&it);
	}

	ofw_child_it_fini(&it);
	return EOK;
error:
	if (subpath != NULL)
		free(subpath);
	ofw_child_it_fini(&it);
	return rc;
}

static errno_t ofw_print_properties(const char *ofwpath)
{
	const char *propname;
	const uint8_t *propval;
	size_t val_sz;
	ofw_prop_it_t it;
	errno_t rc;
	size_t i;

	rc = ofw_prop_it_first(&it, ofwpath);
	if (rc != EOK)
		return rc;

	while (!ofw_prop_it_end(&it)) {
		propname = ofw_prop_it_get_name(&it);
		printf("'%s' =", propname);

		propval = ofw_prop_it_get_data(&it, &val_sz);

		for (i = 0; i < val_sz; i++)
			printf(" %02x", propval[i]);

		printf(" ('%*s')\n", (int)val_sz - 1, (char *)propval);
		ofw_prop_it_next(&it);
	}

	ofw_prop_it_fini(&it);
	return EOK;
}

/**
 * @}
 */
