/*
 * Copyright (c) 2017 Jiri Svoboda
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

/** @addtogroup pkg
 * @{
 */
/** @file Package installer
 *
 * Utility to simplify installation of coastline packages
 */

#include <errno.h>
#include <macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <task.h>

#define NAME "pkg"

static void print_syntax(void)
{
	fprintf(stderr, "syntax: " NAME " install <package-name>\n");
}

static errno_t cmd_runl(const char *path, ...)
{
	va_list ap;
	const char *arg;
	int cnt = 0;

	va_start(ap, path);
	do {
		arg = va_arg(ap, const char *);
		cnt++;
	} while (arg != NULL);
	va_end(ap);

	va_start(ap, path);
	task_id_t id;
	task_wait_t wait;
	errno_t rc = task_spawn(&id, &wait, path, cnt, ap);
	va_end(ap);

	if (rc != EOK) {
		printf("Error spawning %s (%s)\n", path, str_error(rc));
		return rc;
	}

	if (!id) {
		printf("Error spawning %s (invalid task ID)\n", path);
		return EINVAL;
	}

	task_exit_t texit;
	int retval;
	rc = task_wait(&wait, &texit, &retval);
	if (rc != EOK) {
		printf("Error waiting for %s (%s)\n", path, str_error(rc));
		return rc;
	}

	if (texit != TASK_EXIT_NORMAL) {
		printf("Command %s unexpectedly terminated\n", path);
		return EINVAL;
	}

	if (retval != 0) {
		printf("Command %s returned non-zero exit code %d)\n",
		    path, retval);
	}

	return retval == 0 ? EOK : EPARTY;
}

static errno_t pkg_install(int argc, char *argv[])
{
	char *pkg_name;
	char *src_uri;
	char *fname;
	char *fnunpack;
	errno_t rc;
	int ret;

	if (argc != 3) {
		print_syntax();
		return EINVAL;
	}

	pkg_name = argv[2];

	ret = asprintf(&src_uri, "http://ci-ipv4.helenos.org/latest/" STRING(UARCH)
	    "/%s-for-helenos-" STRING(UARCH) ".tar.gz",
	    pkg_name);
	if (ret < 0) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	ret = asprintf(&fname, "/tmp/%s-for-helenos-" STRING(UARCH)
	    ".tar.gz", pkg_name);
	if (ret < 0) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	ret = asprintf(&fnunpack, "/tmp/%s-for-helenos-" STRING(UARCH) ".tar",
	    pkg_name);
	if (ret < 0) {
		printf("Out of memory.\n");
		return ENOMEM;
	}
	/* XXX error cleanup */

	printf("Downloading '%s'.\n", src_uri);

	rc = cmd_runl("/app/download", "/app/download", "-o", fname,
	    src_uri, NULL);
	if (rc != EOK) {
		printf("Error downloading package archive.\n");
		return rc;
	}

	printf("Extracting package\n");

	rc = cmd_runl("/app/gunzip", "/app/gunzip", fname, fnunpack, NULL);
	if (rc != EOK) {
		printf("Error uncompressing package archive.\n");
		return rc;
	}

	if (remove(fname) != 0) {
		printf("Error deleting package archive.\n");
		return rc;
	}

	rc = cmd_runl("/app/untar", "/app/untar", fnunpack, NULL);
	if (rc != EOK) {
		printf("Error extracting package archive.\n");
		return rc;
	}

	if (remove(fnunpack) != 0) {
		printf("Error deleting package archive.\n");
		return rc;
	}

	printf("Package '%s' installed.\n", pkg_name);

	return EOK;
}

int main(int argc, char *argv[])
{
	char *cmd;
	errno_t rc;

	if (argc < 2) {
		fprintf(stderr, "Arguments missing.\n");
		print_syntax();
		return 1;
	}

	cmd = argv[1];

	if (str_cmp(cmd, "install") == 0) {
		rc = pkg_install(argc, argv);
	} else {
		fprintf(stderr, "Unknown command.\n");
		print_syntax();
		return 1;
	}

	if (rc != EOK)
		return 1;

	return 0;
}

/** @}
 */
